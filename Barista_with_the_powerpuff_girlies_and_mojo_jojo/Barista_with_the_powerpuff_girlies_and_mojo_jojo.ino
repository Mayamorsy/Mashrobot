// This code is designed for the ESP32-S3 using the Arduino framework.

// --- PIN DEFINITIONS ---
const int MIC_PIN = 4;   // MAX9814 OUT pin connected to GPIO 4 (an ADC pin)
const int LED_PIN = 10;  // LED connected to GPIO 10 (via a current-limiting resistor)

// --- CONFIGURATION CONSTANTS ---
// Threshold for detecting sound (in ADC units).
// This is the minimum peak-to-peak amplitude required to trigger the LED.
// You will likely need to adjust this value based on your microphone's sensitivity
// and the noise level in your environment.
// Start with 50 and increase if it's too sensitive, decrease if it's not sensitive enough.
const int SOUND_THRESHOLD = 2000;

// Time window (in milliseconds) to sample the microphone signal.
// 50ms is enough to capture a few cycles of typical speech frequencies.
const unsigned long SAMPLE_WINDOW = 50; 

// Time (in milliseconds) the LED remains ON after sound is detected.
const int LED_ON_DURATION = 500; 

// --- STATE VARIABLES ---
unsigned long startTime = 0;

void setup() {
  // Initialize Serial communication for debugging and reading values
  Serial.begin(115200);
  delay(1000);
  Serial.println("Sound Activated LED initialized.");
  
  // Set the LED pin as an output
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Start with the LED off
}

void loop() {
  // Read the sound level
  int amplitude = getPeakToPeakAmplitude();

  // Print the amplitude for debugging and threshold tuning
  Serial.print("Amplitude: ");
  Serial.println(amplitude);

  // Check if the amplitude exceeds the threshold
  if (amplitude > SOUND_THRESHOLD) {
    // Sound detected!
    Serial.println("Loud sound detected! Turning LED ON.");
    digitalWrite(LED_PIN, HIGH); // Turn the LED ON

    // Keep the LED on for the specified duration
    delay(LED_ON_DURATION);
  }

  // Ensure the LED is turned OFF before the next sampling loop
  digitalWrite(LED_PIN, LOW);
  
  // Wait a short moment before checking again to prevent overwhelming the serial monitor
  delay(10); 
}

/**
 * Reads the microphone's analog signal over a set time window 
 * and calculates the peak-to-peak amplitude.
 * This is the difference between the loudest (max) and quietest (min)
 * voltage readings during the sample period.
 */
int getPeakToPeakAmplitude() {
  unsigned long startMillis = millis(); // Start timer
  
  // Initial reading to establish a baseline. The ESP32 ADC resolution is 12-bit (0-4095).
  int maxSignal = analogRead(MIC_PIN);
  int minSignal = maxSignal;

  // Read samples for the specified time window
  while (millis() - startMillis < SAMPLE_WINDOW) {
    int sample = analogRead(MIC_PIN);
    
    // Find the maximum and minimum values during the sample window
    if (sample > maxSignal) {
      maxSignal = sample;
    }
    if (sample < minSignal) {
      minSignal = sample;
    }
    
    // Small delay to allow other tasks or smooth sampling (optional, but good practice)
    delay(1); 
  }
  
  // Peak-to-peak amplitude is the difference between the max and min readings
  int peakToPeak = maxSignal - minSignal;

  return peakToPeak;
}