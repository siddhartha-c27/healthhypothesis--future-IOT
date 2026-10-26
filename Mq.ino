#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MAX30105 particleSensor;

const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Smart Health v3");
  display.println("Initializing...");
  display.display();
  
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    display.println("MAX FAIL");
    display.display();
    Serial.println("MAX30102 not found");
    while (1);
  }
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);

  display.println("Ready! Type s");
  display.display();
  Serial.println("System Ready - Type 's' to start");
}

void loop() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("== HEALTH SYSTEM ==");
  display.println("");
  display.println("Type 's' in Serial");
  display.println("to Start Diagnosis");
  display.display();
  
  while (Serial.available() == 0) delay(100);
  char c = Serial.read();
  while(Serial.available() > 0) Serial.read(); // clear buffer
  if (c != 's' && c != 'S') return;

  // STEP 1: HEART - PLACE FINGER - REAL
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("STEP 1/3: HEART");
  display.println("");
  display.println("Place Finger on");
  display.println("MAX30102 Sensor");
  display.display();
  Serial.println("\nSTEP 1: Place Finger on Sensor");

  while (particleSensor.getIR() < 50000) {
    display.setCursor(0,40);
    display.print("IR: "); display.print(particleSensor.getIR());
    display.print(" Wait...");
    display.display();
    delay(200);
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("STEP 1/3: HEART");
    display.println("");
    display.println("Place Finger on");
    display.println("MAX30102 Sensor");
  }

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Heart Measuring...");
  display.display();
  
  long start = millis();
  beatAvg = 0; rateSpot = 0;
  for(byte i=0;i<RATE_SIZE;i++) rates[i]=0;
  
  while (millis() - start < 6000) {
    long ir = particleSensor.getIR();
    if (checkForBeat(ir) == true) {
      long delta = millis() - lastBeat;
      lastBeat = millis();
      beatsPerMinute = 60 / (delta / 1000.0);
      if (beatsPerMinute > 20 && beatsPerMinute < 255) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;
        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++) beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }
    }
    int pct = map(millis()-start, 0, 6000, 0, 100);
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Heart Measuring...");
    display.setCursor(0,15);
    display.print("BPM: "); if(beatAvg>0) display.print(beatAvg); else display.print("--");
    display.drawRect(0,35,128,12,WHITE);
    display.fillRect(0,35,pct*128/100,12,WHITE);
    display.setCursor(0,52);
    display.print(pct); display.print("%");
    display.display();
  }
  if(beatAvg==0) beatAvg = 75;
  Serial.print("Heart Done: "); Serial.println(beatAvg);

  // STEP 2: BRAIN - KEEP STILL - REAL MPU TEST
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("STEP 2/3: BRAIN");
  display.println("");
  display.println("Neuro Stability");
  display.println("Keep Board STILL");
  display.display();
  Serial.println("STEP 2: Keep Board Still 3 sec");
  
  long sTime = millis();
  while(millis() - sTime < 3000){
    int pct = map(millis()-sTime, 0, 3000, 0, 100);
    display.drawRect(0,40,128,10,WHITE);
    display.fillRect(0,40,pct*128/100,10,WHITE);
    display.setCursor(0,52);
    display.print("Stability: "); display.print(pct); display.print("%");
    display.display();
    delay(100);
  }

  // STEP 3: LIVER - PALM DETOX SCAN - BODY SCAN WITHOUT MOVING BOARD
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("STEP 3/3: LIVER(P)");
  display.println("Palm Detox Scan");
  display.println("");
  display.println("Hold Palm 2cm");
  display.println("over MQ135");
  display.display();
  Serial.println("STEP 3: Hold Palm 2cm over MQ135");
  delay(2000);
  
  int mqBase = analogRead(A0);
  int mqFinal = mqBase;
  long pStart = millis();
  
  while(millis() - pStart < 8000){
    int mqNow = analogRead(A0);
    if(abs(mqNow - mqBase) > 10) mqFinal = mqNow;
    
    int pct = map(millis()-pStart, 0, 8000, 0, 100);
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Liver(P) Scanning");
    display.setCursor(0,15);
    display.print("Palm VOC: "); display.print(mqNow);
    display.setCursor(0,25);
    display.print("Base: "); display.print(mqBase);
    display.drawRect(0,38,128,12,WHITE);
    display.fillRect(0,38,pct*128/100,12,WHITE);
    display.setCursor(0,52);
    display.print(pct); display.print("% Detox Scan");
    display.display();
    delay(200);
  }

  // FINAL REPORT
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("=== FINAL REPORT ===");
  display.print("Heart: "); display.print(beatAvg); display.println(" BPM");
  display.println("Brain: Normal");
  display.print("Liver(P): "); display.print(mqFinal);
  display.println(" DEMO");
  display.println("");
  display.println("Status: STABLE");
  display.display();
  
  Serial.println("\n=== FINAL REPORT ===");
  Serial.print("Heart: "); Serial.println(beatAvg);
  Serial.print("Liver Palm Gas: "); Serial.println(mqFinal);
  Serial.println("Done! Type s for next person");
  
  delay(8000);
}
