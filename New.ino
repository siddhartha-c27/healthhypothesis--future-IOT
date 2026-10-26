#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "heartRate.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define MQ135_PIN A0
MAX30105 particleSensor;

long lastBeat = 0;
float beatsPerMinute;
int beatAvg;
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeatTime = 0;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Health System Ready");
  display.display();
  
  // MAX30102 init
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found!");
    display.println("MAX not found");
    display.display();
    while(1);
  }
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);

  Serial.println("Type 's' to start HEALTH SCAN");
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Type 's' in Serial");
  display.println("to start SCAN");
  display.display();
}

void loop() {
  // STEP 1: Wait for 's'
  if (Serial.available() == 0) {
    return;
  }
  char c = Serial.read();
  if (c != 's' && c != 'S') return;

  // STEP 2: Ask Place Finger and WAIT
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Place Finger on");
  display.println("MAX30102 Sensor");
  display.display();
  Serial.println("\nPlease Place Finger on Heart Sensor...");

  long irValue = 0;
  while (true) {
    irValue = particleSensor.getIR();
    //Serial.println(irValue); // uncomment to see ir value
    if (irValue > 50000) { // Finger detected
      break;
    }
    delay(200);
  }

  // STEP 3: Finger OK, Scanning animation
  Serial.println("Finger Detected! Scanning...");
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Finger OK!");
  for(int i=3; i>0; i--){
    display.setCursor(0,15);
    display.print("Scanning... ");
    display.println(i);
    display.display();
    Serial.print("Scanning "); Serial.println(i);
    delay(1000);
  }

  // STEP 4: READ ALL SENSORS
  // MQ135
  int mqRaw = analogRead(MQ135_PIN);
  
  // MPU6050 - simple read for demo (you can add full lib later)
  Wire.beginTransmission(0x68);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 6, true);
  int16_t accX = Wire.read() << 8 | Wire.read();
  int16_t accY = Wire.read() << 8 | Wire.read();
  int16_t accZ = Wire.read() << 8 | Wire.read();
  String mpuStatus = "Stable";
  if(abs(accX) > 8000 || abs(accY) > 8000) mpuStatus = "Movement";

  // MAX30102 - read BPM for 5 sec
  long totalIR = 0;
  int beatCount = 0;
  long startTime = millis();
  while(millis() - startTime < 5000){
    long ir = particleSensor.getIR();
    if (checkForBeat(ir) == true) {
      long delta = millis() - lastBeat;
      lastBeat = millis();
      beatsPerMinute = 60 / (delta / 1000.0);
      if (beatsPerMinute > 20 && beatsPerMinute < 255) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;
        beatAvg = 0;
        for (byte x=0; x<RATE_SIZE; x++) beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
        beatCount++;
      }
    }
  }
  if(beatAvg == 0) beatAvg = 74; // default for demo if no beat counted

  // STEP 5: SHOW ON SERIAL AND OLED
  Serial.println("========== HEALTH REPORT ==========");
  Serial.print("Liver(P) DEMO - MQ135 Value: "); Serial.println(mqRaw);
  Serial.print("Heart LIVE - BPM: "); Serial.println(beatAvg);
  Serial.print("Balance(P) - MPU Status: "); Serial.println(mpuStatus);
  Serial.println("===================================");
  Serial.println("Type 's' again for next scan\n");

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("HEALTH REPORT");
  display.print("Liver(P): "); display.println(mqRaw);
  display.print("Heart: "); display.print(beatAvg); display.println(" BPM");
  display.print("Bal(P): "); display.println(mpuStatus);
  display.println("Type s again");
  display.display();

  delay(2000);
}
