// FSR Imbalance Detection System
// Activates buzzer when load is present on one FSR but not the other

// Pin definitions
const int FSR1_PIN = 34;        // First FSR analog pin
const int FSR2_PIN = 35;        // Second FSR analog pin
const int BUZZER_PIN = 32;       // Buzzer digital pin

// Threshold values
const int LOAD_THRESHOLD = 100;  // Minimum reading to consider as "load present"
const int IMBALANCE_THRESHOLD = 50; // Minimum difference to trigger imbalance

// Timing variables
const unsigned long BUZZER_DURATION = 5000; // 5 seconds in milliseconds
const unsigned long SAMPLING_INTERVAL = 50;  // Sample every 50ms

unsigned long lastSampleTime = 0;
unsigned long buzzerStartTime = 0;
bool buzzerActive = false;

// FSR readings
int fsr1Reading = 0;
int fsr2Reading = 0;

void setup() {
  // Initialize pins
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Initialize serial for debugging
  Serial.begin(9600);
  Serial.println("FSR Imbalance Detection System Started");
  Serial.println("Monitoring for load imbalance...");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Sample FSRs at regular intervals
  if (currentTime - lastSampleTime >= SAMPLING_INTERVAL) {
    sampleFSRs();
    checkImbalance();
    lastSampleTime = currentTime;
  }
  
  // Handle buzzer timing
  manageBuzzer(currentTime);
}

void sampleFSRs() {
  // Read both FSR values
  fsr1Reading = analogRead(FSR1_PIN);
  fsr2Reading = analogRead(FSR2_PIN);
  
  // Debug output
  Serial.print("FSR1: ");
  Serial.print(fsr1Reading);
  Serial.print(" | FSR2: ");
  Serial.print(fsr2Reading);
  Serial.print(" | Diff: ");
  Serial.println(abs(fsr1Reading - fsr2Reading));
}

void checkImbalance() {
  // Determine if each FSR has a significant load
  bool fsr1HasLoad = (fsr1Reading > LOAD_THRESHOLD);
  bool fsr2HasLoad = (fsr2Reading > LOAD_THRESHOLD);
  
  // Check for imbalance conditions:
  // 1. Load on FSR1 but not FSR2
  // 2. Load on FSR2 but not FSR1
  // 3. Both have loads but significant difference
  
  bool imbalanceDetected = false;
  
  if (fsr1HasLoad && !fsr2HasLoad) {
    // Load only on FSR1
    imbalanceDetected = true;
    Serial.println("IMBALANCE: Load detected only on FSR1");
  }
  else if (!fsr1HasLoad && fsr2HasLoad) {
    // Load only on FSR2
    imbalanceDetected = true;
    Serial.println("IMBALANCE: Load detected only on FSR2");
  }
  else if (fsr1HasLoad && fsr2HasLoad) {
    // Both have loads, check for significant difference
    int difference = abs(fsr1Reading - fsr2Reading);
    if (difference > IMBALANCE_THRESHOLD) {
      imbalanceDetected = true;
      Serial.println("IMBALANCE: Significant load difference detected");
    }
  }
  
  // Activate buzzer if imbalance detected and buzzer not already active
  if (imbalanceDetected && !buzzerActive) {
    activateBuzzer();
  }
}

void activateBuzzer() {
  buzzerActive = true;
  buzzerStartTime = millis();
  digitalWrite(BUZZER_PIN, HIGH);
  Serial.println("BUZZER ACTIVATED - 5 second alert");
}

void manageBuzzer(unsigned long currentTime) {
  // Turn off buzzer after 5 seconds
  if (buzzerActive && (currentTime - buzzerStartTime >= BUZZER_DURATION)) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
    Serial.println("Buzzer deactivated");
  }
}