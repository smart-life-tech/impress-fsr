# FSR Imbalance Detection System

An Arduino-based system that monitors two Force Sensitive Resistors (FSRs) and alerts when there's an imbalance in load distribution between them.

## Overview

This project uses two FSRs to detect load imbalances and activates a buzzer alert when:
- Load is present on one FSR but not the other
- Both FSRs have loads but with significant difference
- Alert duration is 5 seconds with non-blocking timing

## Features

- ✅ Dual FSR monitoring with configurable thresholds
- ✅ Non-blocking 5-second buzzer alerts
- ✅ Real-time serial monitoring and debugging
- ✅ Adjustable sensitivity settings
- ✅ Efficient sampling using `millis()` timing
- ✅ Prevents buzzer re-triggering during active alert

## Hardware Requirements

### Components
- 1x Arduino (Uno, Nano, or compatible)
- 2x Force Sensitive Resistors (FSRs)
- 1x Buzzer (active or passive)
- 2x 10kΩ resistors (pull-down)
- Breadboard and jumper wires

### Wiring Diagram

```
FSR1 Circuit:
FSR1 ---- A0 ---- 10kΩ ---- GND
     |
    5V

FSR2 Circuit:
FSR2 ---- A1 ---- 10kΩ ---- GND
     |
    5V

Buzzer:
Buzzer (+) ---- Pin 8
Buzzer (-) ---- GND
```

## Pin Configuration

| Component | Arduino Pin | Type |
|-----------|-------------|------|
| FSR1      | A0          | Analog Input |
| FSR2      | A1          | Analog Input |
| Buzzer    | 8           | Digital Output |

## Installation

1. **Clone or download** this repository
2. **Open** `impress_fsr.ino` in Arduino IDE
3. **Connect** your hardware according to the wiring diagram
4. **Upload** the code to your Arduino
5. **Open** Serial Monitor (9600 baud) to view real-time data

## Configuration

### Adjustable Parameters

```cpp
// Threshold values
const int LOAD_THRESHOLD = 100;        // Minimum reading for "load present"
const int IMBALANCE_THRESHOLD = 50;    // Minimum difference for imbalance
const unsigned long BUZZER_DURATION = 5000;    // Alert duration (5 seconds)
const unsigned long SAMPLING_INTERVAL = 50;    // Sample rate (50ms)
```

### Tuning Guidelines

| Parameter | Purpose | Typical Range | Notes |
|-----------|---------|---------------|-------|
| `LOAD_THRESHOLD` | Sensitivity for load detection | 50-200 | Lower = more sensitive |
| `IMBALANCE_THRESHOLD` | Difference threshold | 30-100 | Prevents false alarms |
| `SAMPLING_INTERVAL` | How often to check FSRs | 20-100ms | Lower = more responsive |

## Usage

### Normal Operation
1. Power on the Arduino
2. System starts monitoring automatically
3. Serial output shows real-time FSR readings
4. Buzzer activates when imbalance detected

### Serial Monitor Output
```
FSR Imbalance Detection System Started
Monitoring for load imbalance...
FSR1: 45 | FSR2: 52 | Diff: 7
FSR1: 156 | FSR2: 23 | Diff: 133
IMBALANCE: Load detected only on FSR1
BUZZER ACTIVATED - 5 second alert
Buzzer deactivated
```

## Code Structure

### Main Functions

| Function | Purpose |
|----------|---------|
| `setup()` | Initialize pins and serial communication |
| `loop()` | Main program loop with timing control |
| `sampleFSRs()` | Read both FSR values |
| `checkImbalance()` | Detect imbalance conditions |
| `activateBuzzer()` | Start 5-second buzzer alert |
| `manageBuzzer()` | Handle buzzer timing |

### Timing Implementation
- Uses `millis()` for non-blocking timing
- Separate timers for sampling and buzzer control
- Prevents system blocking during alerts

## Troubleshooting

### Common Issues

**Buzzer not working:**
- Check wiring connections
- Verify buzzer polarity
- Test with a simple digitalWrite() test

**False alarms:**
- Increase `LOAD_THRESHOLD` value
- Adjust `IMBALANCE_THRESHOLD`
- Check for electrical noise

**No response to pressure:**
- Decrease `LOAD_THRESHOLD` value
- Verify FSR wiring and pull-down resistors
- Check FSR functionality with multimeter

**Serial output issues:**
- Ensure 9600 baud rate
- Check USB connection
- Verify Arduino power

### Calibration Steps

1. **Open Serial Monitor** (9600 baud)
2. **Apply no pressure** - note baseline readings
3. **Apply light pressure** - adjust `LOAD_THRESHOLD`
4. **Test imbalance scenarios** - fine-tune thresholds
5. **Verify buzzer timing** - should be exactly 5 seconds

## Applications

- **Balance training equipment**
- **Weight distribution monitoring**
- **Posture correction systems**
- **Load balancing alerts**
- **Physical therapy devices**

## Technical Specifications

- **Operating Voltage:** 5V
- **Sampling Rate:** 20Hz (50ms intervals)
- **Alert Duration:** 5 seconds
- **Response Time:** <100ms
- **FSR Range:** 0-1023 (10-bit ADC)

## License

This project is open source. Feel free to modify and distribute.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Support

For issues or questions:
- Check the troubleshooting section
- Review serial monitor output
- Verify hardware connections
- Test individual components

---

**Version:** 1.0  
**Last Updated:** 2024  
**Compatibility:** Arduino Uno, Nano, Pro Mini