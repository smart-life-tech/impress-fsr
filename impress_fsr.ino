// Pin definitions
const int FSR1_PIN = A0;   // First FSR analog pin
const int FSR2_PIN = A1;   // Second FSR analog pin
const int BUZZER_PIN = 10; // Buzzer digital pin

// Thresholds
const int LOAD_THRESHOLD = 100;
const int IMBALANCE_THRESHOLD = 50;

// Timing
const unsigned long BUZZER_DELAY = 5000;    // Delay before buzzer activates
const unsigned long SAMPLING_INTERVAL = 50; // Sampling interval

unsigned long lastSampleTime = 0;
unsigned long imbalanceStartTime = 0;

bool buzzerActive = false;
bool imbalanceOngoing = false;

// FSR readings
int fsr1Reading = 0;
int fsr2Reading = 0;

const int ledcChannel = 0;
const int resolution = 8;
#define LEDC_RESOLUTION 8
#define LEDC_FREQUENCY 1000

void setup()
{
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.begin(9600);
  Serial.println("FSR Imbalance Detection System Started");

  ledcAttach(BUZZER_PIN, LEDC_FREQUENCY, LEDC_RESOLUTION);
  ledcWriteTone(BUZZER_PIN, 128);
  delay(1000);
  ledcWriteTone(BUZZER_PIN, 0);
}

void loop()
{
  unsigned long currentTime = millis();

  if (currentTime - lastSampleTime >= SAMPLING_INTERVAL)
  {
    sampleFSRs();
    detectImbalance(currentTime);
    lastSampleTime = currentTime;
  }

  manageBuzzer(currentTime);
}

void sampleFSRs()
{
  fsr1Reading = analogRead(FSR1_PIN);
  fsr2Reading = analogRead(FSR2_PIN);

  Serial.print("FSR1: ");
  Serial.print(fsr1Reading);
  Serial.print(" | FSR2: ");
  Serial.print(fsr2Reading);
  Serial.print(" | Diff: ");
  Serial.println(abs(fsr1Reading - fsr2Reading));
}

void detectImbalance(unsigned long currentTime)
{
  bool fsr1Load = fsr1Reading > LOAD_THRESHOLD;
  bool fsr2Load = fsr2Reading > LOAD_THRESHOLD;
  int difference = abs(fsr1Reading - fsr2Reading);

  if (fsr1Load && fsr2Load && difference <= IMBALANCE_THRESHOLD)
  {
    // Both sensors touched evenly -> stop buzzer
    buzzerActive = false;
    imbalanceOngoing = false;
    ledcWriteTone(BUZZER_PIN, 0);
    Serial.println("Balanced pressure on both sensors – buzzer stopped.");
    return;
  }

  // Detect imbalance: one pressed, or large difference
  bool imbalanceDetected =
      (fsr1Load && !fsr2Load) ||
      (!fsr1Load && fsr2Load) ||
      (fsr1Load && fsr2Load && difference > IMBALANCE_THRESHOLD);

  if (imbalanceDetected)
  {
    if (!imbalanceOngoing)
    {
      imbalanceOngoing = true;
      imbalanceStartTime = currentTime;
      Serial.println("Imbalance detected – starting 5 second timer.");
    }
  }
  else
  {
    imbalanceOngoing = false;
    if (buzzerActive)
    {
      buzzerActive = false;
      ledcWriteTone(BUZZER_PIN, 0);
      Serial.println("Imbalance cleared – buzzer stopped.");
    }
  }
}

void manageBuzzer(unsigned long currentTime)
{
  if (imbalanceOngoing && !buzzerActive && (currentTime - imbalanceStartTime >= BUZZER_DELAY))
  {
    buzzerActive = true;
    ledcWriteTone(BUZZER_PIN, 200); // Activate buzzer tone
    Serial.println("BUZZER ACTIVATED after 5 seconds of imbalance.");
  }
}