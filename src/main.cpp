
// FSR Posture Monitoring System with Firebase Analytics
// Monitors sitting posture, detects imbalances, logs data to Firebase for health insights
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <FirebaseESP32.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <BluetoothSerial.h>

// Firebase configuration (replace with your actual values)
#define FIREBASE_HOST "your-project.firebaseio.com" // Your Firebase project URL
#define FIREBASE_AUTH "your-database-secret"        // Your Firebase database secret
FirebaseData firebaseData;

// WiFi credentials (replace with your actual values)
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"

// NTP for timestamps
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // Update every minute

// Bluetooth Serial for fallback communication
BluetoothSerial SerialBT;

// Connection mode
enum ConnectionMode
{
    WIFI_MODE,
    BT_MODE
};
ConnectionMode currentMode = WIFI_MODE;

// Pin definitions
const int FSR1_PIN = 34;   // Seat FSR analog pin
const int FSR2_PIN = 35;   // Back FSR analog pin
const int BUZZER_PIN = 32; // Buzzer digital pin

// Threshold values
const int LOAD_THRESHOLD = 100;     // Minimum reading to consider as "load present"
const int IMBALANCE_THRESHOLD = 50; // Minimum difference to trigger imbalance

// Timing variables
const unsigned long BUZZER_DURATION = 5000;           // 5 seconds in milliseconds
const unsigned long SAMPLING_INTERVAL = 50;           // Sample every 50ms
const unsigned long FIREBASE_UPDATE_INTERVAL = 60000; // Send data every 60 seconds
const unsigned long WIFI_CHECK_INTERVAL = 30000;      // Check WiFi every 30 seconds

unsigned long lastSampleTime = 0;
unsigned long buzzerStartTime = 0;
unsigned long lastFirebaseUpdate = 0;
unsigned long lastWiFiCheck = 0;
bool buzzerActive = false;

// FSR readings
int fsr1Reading = 0;
int fsr2Reading = 0;

// Analytics variables
unsigned long sittingStartTime = 0;
unsigned long totalSittingTime = 0;
unsigned long imbalanceStartTime = 0;
unsigned long totalImbalanceTime = 0;
int imbalanceCount = 0;
int currentPostureScore = 0; // 0-100, higher is better
float avgSeatPressure = 0.0;
float avgBackPressure = 0.0;
int sampleCount = 0;
String lastRecommendation = "";

const int ledcChannel = 0;  // PWM channel
const int resolution = 8;   // 8-bit resolution
#define LEDC_RESOLUTION 8   // bits
#define LEDC_FREQUENCY 1000 // Hz
void setupWiFi();
void setupBluetooth();
void sendDataToFirebase();
void updateAnalytics();
void sampleFSRs();
void checkImbalance();
void activateBuzzer();
void manageBuzzer(unsigned long currentTime);
void checkConnectionMode();
void sendDataViaBluetooth();

void setup()
{
    // Initialize pins
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    // Initialize serial for debugging
    Serial.begin(9600);
    Serial.println("FSR Posture Monitoring System with Firebase Started");

    // Try WiFi connection first
    setupWiFi();

    // Initialize buzzer
    ledcAttach(BUZZER_PIN, LEDC_FREQUENCY, LEDC_RESOLUTION);
    ledcWriteTone(BUZZER_PIN, 128); // Play 1kHz tone
    delay(1000);                    // Duration
    ledcWriteTone(BUZZER_PIN, 0);   // Stop tone

    // Initialize sitting start time
    sittingStartTime = millis();
    Serial.println("Monitoring posture...");
}

void setupWiFi()
{
    // Try to connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    { // 10 seconds timeout
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi connected");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        currentMode = WIFI_MODE;

        // Initialize Firebase
        Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
        Firebase.reconnectWiFi(true);

        // Initialize NTP
        timeClient.begin();
        timeClient.update();
        Serial.println("NTP initialized");
    }
    else
    {
        Serial.println("\nWiFi connection failed. Switching to Bluetooth mode.");
        currentMode = BT_MODE;
        setupBluetooth();
    }
}

void setupBluetooth()
{
    SerialBT.begin("FSR_Posture_Monitor"); // Bluetooth device name
    Serial.println("Bluetooth initialized. Device name: FSR_Posture_Monitor");
}

void loop()
{
    unsigned long currentTime = millis();

    // Sample FSRs at regular intervals
    if (currentTime - lastSampleTime >= SAMPLING_INTERVAL)
    {
        sampleFSRs();
        checkImbalance();
        lastSampleTime = currentTime;
    }

    // Handle buzzer timing
    manageBuzzer(currentTime);

    // Check WiFi connection periodically and switch modes if needed
    if (currentTime - lastWiFiCheck >= WIFI_CHECK_INTERVAL)
    {
        checkConnectionMode();
        lastWiFiCheck = currentTime;
    }

    // Send data based on connection mode
    if (currentMode == WIFI_MODE)
    {
        if (currentTime - lastFirebaseUpdate >= FIREBASE_UPDATE_INTERVAL)
        {
            updateAnalytics();
            sendDataToFirebase();
            lastFirebaseUpdate = currentTime;
        }
    }
    else if (currentMode == BT_MODE)
    {
        if (currentTime - lastFirebaseUpdate >= FIREBASE_UPDATE_INTERVAL)
        {
            updateAnalytics();
            sendDataViaBluetooth();
            lastFirebaseUpdate = currentTime;
        }
    }
}

void checkConnectionMode()
{
    if (currentMode == WIFI_MODE)
    {
        // Check if WiFi is still connected
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("WiFi connection lost. Switching to Bluetooth mode.");
            currentMode = BT_MODE;
            setupBluetooth();
        }
    }
    else if (currentMode == BT_MODE)
    {
        // Try to reconnect to WiFi
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 10)
        { // 5 seconds timeout
            delay(500);
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("WiFi reconnected. Switching to WiFi mode.");
            currentMode = WIFI_MODE;

            // Reinitialize Firebase and NTP
            Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
            Firebase.reconnectWiFi(true);
            timeClient.begin();
            timeClient.update();
            Serial.println("Firebase and NTP reinitialized");
        }
    }
}

void sampleFSRs()
{
    // Read both FSR values
    fsr1Reading = analogRead(FSR1_PIN);
    fsr2Reading = analogRead(FSR2_PIN);

    // Update analytics
    sampleCount++;
    avgSeatPressure = (avgSeatPressure * (sampleCount - 1) + fsr1Reading) / sampleCount;
    avgBackPressure = (avgBackPressure * (sampleCount - 1) + fsr2Reading) / sampleCount;

    // Calculate posture score (higher is better, max 100)
    // Score based on balance and pressure distribution
    int difference = abs(fsr1Reading - fsr2Reading);
    float balanceScore = max(0, 100 - (difference * 2));             // Penalize imbalance
    float pressureScore = min(100, (fsr1Reading + fsr2Reading) / 4); // Reward even pressure
    currentPostureScore = (balanceScore + pressureScore) / 2;

    // Debug output
    Serial.print("FSR1: ");
    Serial.print(fsr1Reading);
    Serial.print(" | FSR2: ");
    Serial.print(fsr2Reading);
    Serial.print(" | Diff: ");
    Serial.print(abs(fsr1Reading - fsr2Reading));
    Serial.print(" | Score: ");
    Serial.println(currentPostureScore);
}

void checkImbalance()
{
    // Determine if each FSR has a significant load
    bool fsr1HasLoad = (fsr1Reading > LOAD_THRESHOLD);
    bool fsr2HasLoad = (fsr2Reading > LOAD_THRESHOLD);

    // Check for imbalance conditions:
    // 1. Load on FSR1 but not FSR2
    // 2. Load on FSR2 but not FSR1
    // 3. Both have loads but significant difference

    bool imbalanceDetected = false;

    if (fsr1HasLoad && !fsr2HasLoad)
    {
        // Load only on FSR1
        imbalanceDetected = true;
        Serial.println("IMBALANCE: Load detected only on FSR1");
    }
    else if (!fsr1HasLoad && fsr2HasLoad)
    {
        // Load only on FSR2
        imbalanceDetected = true;
        Serial.println("IMBALANCE: Load detected only on FSR2");
    }
    else if (fsr1HasLoad && fsr2HasLoad)
    {
        // Both have loads, check for significant difference
        int difference = abs(fsr1Reading - fsr2Reading);
        if (difference > IMBALANCE_THRESHOLD)
        {
            imbalanceDetected = true;
            Serial.println("IMBALANCE: Significant load difference detected");
        }
    }

    // Track imbalance timing
    if (imbalanceDetected && imbalanceStartTime == 0)
    {
        imbalanceStartTime = millis();
        imbalanceCount++;
    }
    else if (!imbalanceDetected && imbalanceStartTime != 0)
    {
        totalImbalanceTime += millis() - imbalanceStartTime;
        imbalanceStartTime = 0;
    }

    // Activate buzzer if imbalance detected and buzzer not already active
    if (imbalanceDetected && !buzzerActive)
    {
        activateBuzzer();
    }
}

void activateBuzzer()
{
    buzzerActive = true;
    buzzerStartTime = millis();
    // digitalWrite(BUZZER_PIN, HIGH);
    Serial.println("BUZZER ACTIVATED - 5 second alert");
    ledcWriteTone(BUZZER_PIN, 200); // Play 1kHz tone
}

void manageBuzzer(unsigned long currentTime)
{
    // Turn off buzzer after 5 seconds
    if (buzzerActive && (currentTime - buzzerStartTime >= BUZZER_DURATION))
    {
        // digitalWrite(BUZZER_PIN, LOW);
        buzzerActive = false;
        Serial.println("Buzzer deactivated");
        ledcWriteTone(BUZZER_PIN, 0);
    }
}

void updateAnalytics()
{
    // Update total sitting time
    totalSittingTime = millis() - sittingStartTime;

    // Generate health recommendations based on analytics
    generateRecommendation();
}

void generateRecommendation()
{
    String recommendation = "";

    if (totalSittingTime > 3600000)
    { // More than 1 hour sitting
        recommendation += "You've been sitting for over an hour. Consider taking a short walk. ";
    }

    if (totalImbalanceTime > 600000)
    { // More than 10 minutes imbalance
        recommendation += "Prolonged posture imbalance detected. Adjust your sitting position for better spinal alignment. ";
    }

    if (currentPostureScore < 50)
    {
        recommendation += "Current posture score is low. Try to distribute weight evenly between seat and back. ";
    }

    if (avgSeatPressure < 150 || avgBackPressure < 150)
    {
        recommendation += "Low pressure detected - ensure you're sitting properly on the sensors. ";
    }

    if (imbalanceCount > 10)
    {
        recommendation += "Multiple imbalances detected. Consider ergonomic adjustments to your workspace. ";
    }

    lastRecommendation = recommendation;
}

void sendDataToFirebase()
{
    // Update NTP time
    timeClient.update();
    String timestamp = timeClient.getFormattedTime();

    // Prepare JSON data
    String jsonData = "{";
    jsonData += "\"timestamp\":\"" + timestamp + "\",";
    jsonData += "\"totalSittingTime\":" + String(totalSittingTime / 1000) + ","; // in seconds
    jsonData += "\"totalImbalanceTime\":" + String(totalImbalanceTime / 1000) + ",";
    jsonData += "\"imbalanceCount\":" + String(imbalanceCount) + ",";
    jsonData += "\"currentPostureScore\":" + String(currentPostureScore) + ",";
    jsonData += "\"avgSeatPressure\":" + String(avgSeatPressure, 2) + ",";
    jsonData += "\"avgBackPressure\":" + String(avgBackPressure, 2) + ",";
    jsonData += "\"lastFSR1\":" + String(fsr1Reading) + ",";
    jsonData += "\"lastFSR2\":" + String(fsr2Reading) + ",";
    jsonData += "\"recommendation\":\"" + lastRecommendation + "\"";
    jsonData += "}";

    // Send to Firebase
    if (Firebase.setJSON(firebaseData, "/postureData", jsonData))
    {
        Serial.println("Data sent to Firebase successfully");
    }
    else
    {
        Serial.println("Failed to send data to Firebase");
        Serial.println(firebaseData.errorReason());
    }

    // Also send event log if imbalance occurred recently
    if (imbalanceCount > 0)
    {
        String eventData = "{";
        eventData += "\"timestamp\":\"" + timestamp + "\",";
        eventData += "\"event\":\"imbalance_detected\",";
        eventData += "\"fsr1\":" + String(fsr1Reading) + ",";
        eventData += "\"fsr2\":" + String(fsr2Reading) + ",";
        eventData += "\"postureScore\":" + String(currentPostureScore);
        eventData += "}";

        String eventPath = "/events/" + String(millis());
        if (Firebase.setJSON(firebaseData, eventPath, eventData))
        {
            Serial.println("Event logged to Firebase");
        }
        else
        {
            Serial.println("Failed to log event");
        }
    }
}

void sendDataViaBluetooth()
{
    // Prepare data string for Bluetooth transmission
    String dataString = "POSTURE_DATA:";
    dataString += "totalSittingTime=" + String(totalSittingTime / 1000) + ",";
    dataString += "totalImbalanceTime=" + String(totalImbalanceTime / 1000) + ",";
    dataString += "imbalanceCount=" + String(imbalanceCount) + ",";
    dataString += "currentPostureScore=" + String(currentPostureScore) + ",";
    dataString += "avgSeatPressure=" + String(avgSeatPressure, 2) + ",";
    dataString += "avgBackPressure=" + String(avgBackPressure, 2) + ",";
    dataString += "lastFSR1=" + String(fsr1Reading) + ",";
    dataString += "lastFSR2=" + String(fsr2Reading) + ",";
    dataString += "recommendation=" + lastRecommendation;
    dataString += "\n";

    // Send via Bluetooth Serial
    SerialBT.println(dataString);
    Serial.println("Data sent via Bluetooth: " + dataString);

    // Also send event log if imbalance occurred recently
    if (imbalanceCount > 0)
    {
        String eventString = "EVENT:imbalance_detected,";
        eventString += "fsr1=" + String(fsr1Reading) + ",";
        eventString += "fsr2=" + String(fsr2Reading) + ",";
        eventString += "postureScore=" + String(currentPostureScore);
        eventString += "\n";

        SerialBT.println(eventString);
        Serial.println("Event sent via Bluetooth: " + eventString);
    }
}
