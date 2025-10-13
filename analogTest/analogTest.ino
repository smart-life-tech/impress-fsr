/*
 * FSR GPIO Reader for ESP32 S3
 * Actually reads FSR values from GPIO pins to test impress_fsr.ino logic
 *
 * Reads from the same GPIO pins as impress_fsr.ino:
 * FSR1_PIN = 14, FSR2_PIN = 27
 *
 * Serial Commands:
 * 'r' - Read current FSR values once
 * 'c' - Continuous reading mode (toggle on/off)
 * 's' - Show statistics (min/max/avg over last 10 readings)
 * 't' - Test thresholds (show load/imbalance analysis)
 */

// GPIO pins matching impress_fsr.ino
const int FSR1_PIN = A0; // First FSR analog pin
const int FSR2_PIN = A1; // Second FSR analog pin

// Test parameters matching impress_fsr.ino
const int LOAD_THRESHOLD = 100;
const int IMBALANCE_THRESHOLD = 50;
const unsigned long TEST_INTERVAL = 50; // Match sampling interval

// FSR readings
int fsr1Reading = 0;
int fsr2Reading = 0;

// Statistics tracking
const int MAX_HISTORY = 10;
int fsr1History[MAX_HISTORY];
int fsr2History[MAX_HISTORY];
int historyIndex = 0;
bool continuousMode = true;
unsigned long lastReadTime = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println("=== FSR GPIO Reader Started ===");
  Serial.println("Reading from GPIO pins:");
  Serial.print("FSR1_PIN = ");
  Serial.println(FSR1_PIN);
  Serial.print("FSR2_PIN = ");
  Serial.println(FSR2_PIN);
  Serial.println();
  Serial.println("Available commands:");
  Serial.println("r - Read current FSR values once");
  Serial.println("c - Toggle continuous reading mode");
  Serial.println("s - Show statistics (min/max/avg over last 10 readings)");
  Serial.println("t - Test thresholds (show load/imbalance analysis)");
  Serial.println("==================================");

  // Initialize history arrays
  for (int i = 0; i < MAX_HISTORY; i++)
  {
    fsr1History[i] = 0;
    fsr2History[i] = 0;
  }
}

void loop()
{
  unsigned long currentTime = millis();

  // Check for serial commands
  if (Serial.available() > 0)
  {
    char command = Serial.read();
    handleSerialCommand(command);
  }

  // Handle continuous reading mode
  if (continuousMode && currentTime - lastReadTime >= TEST_INTERVAL)
  {
    readFSRValues();
    outputReadings();
    updateHistory();
    lastReadTime = currentTime;
  }
}

void handleSerialCommand(char command)
{
  switch (command)
  {
  case 'r':
    Serial.println("Single reading:");
    readFSRValues();
    outputReadings();
    updateHistory();
    break;
  case 'c':
    continuousMode = !continuousMode;
    if (continuousMode)
    {
      Serial.println("Continuous mode ON");
      lastReadTime = millis();
    }
    else
    {
      Serial.println("Continuous mode OFF");
    }
    break;
  case 's':
    showStatistics();
    break;
  case 't':
    testThresholds();
    break;
  default:
    Serial.println("Unknown command. Use r, c, s, or t");
    break;
  }
}

void readFSRValues()
{
  // Actually read from GPIO pins like impress_fsr.ino does
  fsr1Reading = analogRead(FSR1_PIN);
  fsr2Reading = analogRead(FSR2_PIN);
}

void outputReadings()
{
  int difference = abs(fsr1Reading - fsr2Reading);

  // Output in format similar to impress_fsr.ino
  Serial.print("FSR1: ");
  Serial.print(fsr1Reading);
  Serial.print(" | FSR2: ");
  Serial.print(fsr2Reading);
  Serial.print(" | Diff: ");
  Serial.print(difference);

  // Add analysis
  bool fsr1Load = fsr1Reading > LOAD_THRESHOLD;
  bool fsr2Load = fsr2Reading > LOAD_THRESHOLD;
  bool imbalanced = (fsr1Load && fsr2Load && difference > IMBALANCE_THRESHOLD) ||
                    (fsr1Load && !fsr2Load) ||
                    (!fsr1Load && fsr2Load);

  Serial.print(" | Load1: ");
  Serial.print(fsr1Load ? "YES" : "NO");
  Serial.print(" | Load2: ");
  Serial.print(fsr2Load ? "YES" : "NO");
  Serial.print(" | Imbalanced: ");
  Serial.print(imbalanced ? "YES" : "NO");

  // Expected behavior based on impress_fsr.ino logic
  if (fsr1Load && fsr2Load && difference <= IMBALANCE_THRESHOLD)
  {
    Serial.print(" | Expected: BALANCED (buzzer should stop)");
  }
  else if (imbalanced)
  {
    Serial.print(" | Expected: IMBALANCED (buzzer timer should start)");
  }
  else
  {
    Serial.print(" | Expected: NO LOAD (no buzzer activity)");
  }

  Serial.println();
}

void updateHistory()
{
  fsr1History[historyIndex] = fsr1Reading;
  fsr2History[historyIndex] = fsr2Reading;
  historyIndex = (historyIndex + 1) % MAX_HISTORY;
}

void showStatistics()
{
  Serial.println("=== Statistics (last 10 readings) ===");

  // Calculate FSR1 stats
  int fsr1Min = fsr1History[0];
  int fsr1Max = fsr1History[0];
  long fsr1Sum = 0;

  for (int i = 0; i < MAX_HISTORY; i++)
  {
    if (fsr1History[i] < fsr1Min)
      fsr1Min = fsr1History[i];
    if (fsr1History[i] > fsr1Max)
      fsr1Max = fsr1History[i];
    fsr1Sum += fsr1History[i];
  }

  // Calculate FSR2 stats
  int fsr2Min = fsr2History[0];
  int fsr2Max = fsr2History[0];
  long fsr2Sum = 0;

  for (int i = 0; i < MAX_HISTORY; i++)
  {
    if (fsr2History[i] < fsr2Min)
      fsr2Min = fsr2History[i];
    if (fsr2History[i] > fsr2Max)
      fsr2Max = fsr2History[i];
    fsr2Sum += fsr2History[i];
  }

  Serial.print("FSR1: Min=");
  Serial.print(fsr1Min);
  Serial.print(" Max=");
  Serial.print(fsr1Max);
  Serial.print(" Avg=");
  Serial.println(fsr1Sum / MAX_HISTORY);

  Serial.print("FSR2: Min=");
  Serial.print(fsr2Min);
  Serial.print(" Max=");
  Serial.print(fsr2Max);
  Serial.print(" Avg=");
  Serial.println(fsr2Sum / MAX_HISTORY);

  Serial.println("==================================");
}

void testThresholds()
{
  Serial.println("=== Threshold Analysis ===");

  bool fsr1Load = fsr1Reading > LOAD_THRESHOLD;
  bool fsr2Load = fsr2Reading > LOAD_THRESHOLD;
  int difference = abs(fsr1Reading - fsr2Reading);
  bool imbalanced = (fsr1Load && fsr2Load && difference > IMBALANCE_THRESHOLD) ||
                    (fsr1Load && !fsr2Load) ||
                    (!fsr1Load && fsr2Load);

  Serial.print("Current readings: FSR1=");
  Serial.print(fsr1Reading);
  Serial.print(" FSR2=");
  Serial.println(fsr2Reading);

  Serial.print("LOAD_THRESHOLD (");
  Serial.print(LOAD_THRESHOLD);
  Serial.print("): FSR1=");
  Serial.print(fsr1Load ? "ABOVE" : "BELOW");
  Serial.print(" FSR2=");
  Serial.println(fsr2Load ? "ABOVE" : "BELOW");

  Serial.print("IMBALANCE_THRESHOLD (");
  Serial.print(IMBALANCE_THRESHOLD);
  Serial.print("): Difference=");
  Serial.print(difference);
  Serial.print(" Imbalanced=");
  Serial.println(imbalanced ? "YES" : "NO");

  Serial.print("Expected behavior: ");
  if (fsr1Load && fsr2Load && difference <= IMBALANCE_THRESHOLD)
  {
    Serial.println("BALANCED - buzzer should stop");
  }
  else if (imbalanced)
  {
    Serial.println("IMBALANCED - buzzer timer should start");
  }
  else
  {
    Serial.println("NO LOAD - no buzzer activity");
  }

  Serial.println("=========================");
}
