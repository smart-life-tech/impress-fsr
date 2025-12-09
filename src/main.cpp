// FSR Posture Monitoring System with Firebase Analytics
// Monitors sitting posture, detects imbalances, logs data to Firebase for health insights
#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <NTPClient.h>
#include <WiFiUdp.h>


// Firebase configuration (replace with your actual values)
#define FIREBASE_HOST "smart-chair-31447-default-rtdb.firebaseio.com/" 
#define FIREBASE_AUTH "Y4foCcBVznqKlCtc0FaDt0Qa8x79N5BgrjcPAcpK"        
FirebaseData firebaseData;

// Device unique identifier
String deviceID = "";

// WiFi credentials (replace with your actual values)
#define WIFI_SSID "Linda"
#define WIFI_PASSWORD "12345678"

// NTP for timestamps (GMT+0, update every minute)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

 

// Connection mode
enum ConnectionMode
{
    WIFI_MODE,
    BT_MODE
};
ConnectionMode currentMode = WIFI_MODE;

// Pin definitions
const int FSR1_PIN = 14;   // Seat FSR analog pin
const int FSR2_PIN = 27;   // Back FSR analog pin
const int BUZZER_PIN = 25; // Buzzer digital pin

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

// Function declarations
void setupWiFi();
 
void sendDataToFirebase();
void updateAnalytics();
void sampleFSRs();
void checkImbalance();
void activateBuzzer();
void manageBuzzer(unsigned long currentTime);
void checkConnectionMode();
 
void generateRecommendation();
String getFormattedDateTime();
void cleanupOldEntries(String basePath, int maxEntries);
String generateDeviceID();

void setup()
{
    // Initialize pins
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    // Initialize serial for debugging
    Serial.begin(9600);
    Serial.println("FSR Posture Monitoring System with Firebase Started");

    // Generate unique device ID from MAC address
    deviceID = generateDeviceID();
    Serial.print("Device ID: ");
    Serial.println(deviceID);

    // Try WiFi connection first
    setupWiFi();

    // Initialize buzzer PWM
    // ledcAttach(BUZZER_PIN, LEDC_FREQUENCY, LEDC_RESOLUTION);
    //ledcAttachPin(BUZZER_PIN, LEDC_FREQUENCY);
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
            
            lastFirebaseUpdate = currentTime;
        }
    }
}

void checkConnectionMode()
{
    if (currentMode == WIFI_MODE)
    {
        // Check if WiFi is still connected
     
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
    // Update NTP time and get full date-time
    timeClient.update();
    String timestamp = getFormattedDateTime();

    // Prepare JSON data with device ID
    String jsonData = "{";
    jsonData += "\"deviceID\":\"" + deviceID + "\",";
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
    
    FirebaseJson json;
    json.setJsonData(jsonData);
    
    // Push data to device-specific path (creates unique key for each entry)
    String dataPath = "/devices/" + deviceID + "/postureData";
    if (Firebase.pushJSON(firebaseData, dataPath, json))
    {
        Serial.println("Data sent to Firebase successfully");
        Serial.println("Timestamp: " + timestamp);
        
        // Clean up old entries to keep only last 100
        cleanupOldEntries(dataPath, 100);
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
        eventData += "\"deviceID\":\"" + deviceID + "\",";
        eventData += "\"timestamp\":\"" + timestamp + "\",";
        eventData += "\"event\":\"imbalance_detected\",";
        eventData += "\"fsr1\":" + String(fsr1Reading) + ",";
        eventData += "\"fsr2\":" + String(fsr2Reading) + ",";
        eventData += "\"postureScore\":" + String(currentPostureScore);
        eventData += "}";

        FirebaseJson eventJson;
        eventJson.setJsonData(eventData);
        
        String eventPath = "/devices/" + deviceID + "/events";
        if (Firebase.pushJSON(firebaseData, eventPath, eventJson))
        {
            Serial.println("Event logged to Firebase");
            
            // Clean up old events to keep only last 100
            cleanupOldEntries(eventPath, 100);
        }
        else
        {
            Serial.println("Failed to log event");
        }
    }
}

 
// Generate unique device ID from MAC address
String generateDeviceID()
{
    uint8_t mac[6];
    WiFi.macAddress(mac);
    String macStr = "";
    for (int i = 0; i < 6; i++)
    {
        if (mac[i] < 16)
            macStr += "0";
        macStr += String(mac[i], HEX);
    }
    macStr.toUpperCase();
    return "FSR_" + macStr;
}

// Get formatted date-time string (YYYY-MM-DD HH:MM:SS)
String getFormattedDateTime()
{
    unsigned long epochTime = timeClient.getEpochTime();
    
    // Calculate date components
    int currentYear = 1970;
    int currentMonth = 1;
    int currentDay = 1;
    
    unsigned long days = epochTime / 86400;
    unsigned long remainingSeconds = epochTime % 86400;
    
    int hours = remainingSeconds / 3600;
    remainingSeconds %= 3600;
    int minutes = remainingSeconds / 60;
    int seconds = remainingSeconds % 60;
    
    // Simple date calculation (approximate, good enough for logging)
    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    while (days > 0)
    {
        int daysInYear = 365;
        // Check for leap year
        if ((currentYear % 4 == 0 && currentYear % 100 != 0) || (currentYear % 400 == 0))
        {
            daysInYear = 366;
            daysInMonth[1] = 29;
        }
        else
        {
            daysInMonth[1] = 28;
        }
        
        if (days >= daysInYear)
        {
            days -= daysInYear;
            currentYear++;
        }
        else
        {
            break;
        }
    }
    
    while (days > 0)
    {
        if (days >= daysInMonth[currentMonth - 1])
        {
            days -= daysInMonth[currentMonth - 1];
            currentMonth++;
            if (currentMonth > 12)
            {
                currentMonth = 1;
                currentYear++;
            }
        }
        else
        {
            currentDay += days;
            days = 0;
        }
    }
    
    // Format as YYYY-MM-DD HH:MM:SS
    String dateTime = String(currentYear) + "-";
    if (currentMonth < 10) dateTime += "0";
    dateTime += String(currentMonth) + "-";
    if (currentDay < 10) dateTime += "0";
    dateTime += String(currentDay) + " ";
    if (hours < 10) dateTime += "0";
    dateTime += String(hours) + ":";
    if (minutes < 10) dateTime += "0";
    dateTime += String(minutes) + ":";
    if (seconds < 10) dateTime += "0";
    dateTime += String(seconds);
    
    return dateTime;
}

// Clean up old entries to maintain only the last maxEntries
void cleanupOldEntries(String basePath, int maxEntries)
{
    // Query to get all entries
    QueryFilter query;
    
    if (Firebase.getJSON(firebaseData, basePath))
    {
        FirebaseJson &json = firebaseData.jsonObject();
        size_t count = json.iteratorBegin();
        
        // If we have more than maxEntries, delete the oldest ones
        if (count > maxEntries)
        {
            String key, value;
            int type = 0;
            int entriesToDelete = count - maxEntries;
            int deleted = 0;
            
            // Iterate through entries and delete oldest ones
            for (size_t i = 0; i < count && deleted < entriesToDelete; i++)
            {
                json.iteratorGet(i, type, key, value);
                String deletePath = basePath + "/" + key;
                
                if (Firebase.deleteNode(firebaseData, deletePath))
                {
                    deleted++;
                    Serial.println("Deleted old entry: " + key);
                }
            }
            
            json.iteratorEnd();
            Serial.println("Cleanup complete. Deleted " + String(deleted) + " old entries.");
        }
        else
        {
            json.iteratorEnd();
        }
    }
}
