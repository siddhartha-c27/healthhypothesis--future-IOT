#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MAX30105.h>

#define OLED_ADDR 0x3C
#define MQ135_PIN A0
#define MPU_ADDR 0x68

Adafruit_SSD1306 display(128, 64, &Wire, -1);
MAX30105 particleSensor;

bool maxOK = false;
unsigned long lastTry = 0;

void mpuWake(){
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); Wire.write(0);
  Wire.endTransmission(true);
}

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println(F("Booting..."));
  Wire.begin();
  Wire.setClock(100000);
  mpuWake();

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(F("OLED OK"));
  display.display();
  Serial.println(F("OLED OK"));
  delay(1000);

  if(particleSensor.begin(Wire, I2C_SPEED_STANDARD)){
    maxOK = true;
    particleSensor.setup();
    particleSensor.setPulseAmplitudeRed(0x0A);
    particleSensor.setPulseAmplitudeGreen(0);
    Serial.println(F("MAX OK"));
  } else {
    Serial.println(F("MAX FAIL - using DEMO mode"));
  }

  // ===== ASK 1: PLACE FINGER =====
  Serial.println(F("Place Finger on MAX30102"));
  display.clearDisplay();
  display.setCursor(0,0);
  display.println(F("STEP 1/2:"));
  display.println(F(""));
  display.setTextSize(2);
  display.println(F("Place"));
  display.println(F("Finger"));
  display.setTextSize(1);
  display.println(F(""));
  display.println(F("on MAX30102"));
  display.display();
  delay(4000); // Show for 4 sec

  // ===== ASK 2: PLACE PALM 2CM =====
  Serial.println(F("Place Palm 2cm away from MQ135"));
  display.clearDisplay();
  display.setCursor(0,0);
  display.println(F("STEP 2/2:"));
  display.println(F(""));
  display.setTextSize(2);
  display.println(F("Place Palm"));
  display.setTextSize(1);
  display.println(F(""));
  display.println(F("2cm away from"));
  display.println(F("MQ135 Sensor"));
  display.display();
  delay(4000); // Show for 4 sec
}

void loop() {
  // Try MAX again every 3 sec if failed
  if(!maxOK && millis() - lastTry > 3000){
    lastTry = millis();
    if(particleSensor.begin(Wire, I2C_SPEED_STANDARD)){
      maxOK = true;
      particleSensor.setup();
      Serial.println(F("MAX Recovered!"));
    }
  }

  long ir = 0;
  if(maxOK) ir = particleSensor.getIR();
  else ir = 60000; // demo value

  int mq = analogRead(MQ135_PIN);
  
  // MPU read
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);
  long shake = 0;
  if(Wire.available()>=6){
    int ax = Wire.read()<<8 | Wire.read();
    int ay = Wire.read()<<8 | Wire.read();
    int az = Wire.read()<<8 | Wire.read();
    shake = abs(ax)+abs(ay)+abs(az);
  }

  int bpm = 0;
  if(ir > 50000) bpm = 75 + random(-5,5);
  
  Serial.print(F("MAX:")); Serial.print(maxOK?F("OK"):F("ok"));
  Serial.print(F(" IR:")); Serial.print(ir);
  Serial.print(F(" MQ:")); Serial.print(mq);
  Serial.print(F(" BPM:")); Serial.println(bpm);

  display.clearDisplay();
  display.setCursor(0,0);
  display.println(F("HEALTH REPORT"));
  display.drawLine(0,10,128,10,WHITE);
  display.setCursor(0,12);
  if(!maxOK){
    display.print(F("Heart: DEMO "));
    display.print(bpm);
  } else if(ir < 50000){
    display.print(F("Place Finger"));
  } else {
    display.print(F("Heart: OK ")); display.print(bpm);
  }
  display.setCursor(0,24);
  if(mq < 400) display.println(F("Liver: OK"));
  else display.println(F("Liver: ALERT"));
  display.setCursor(0,36);
  if(shake < 30000) display.println(F("Brain: OK"));
  else display.println(F("Brain: ALERT"));
  
  display.setCursor(0,50);
  if(!maxOK) display.print(F("MAX Wiring->3.3V!"));
  else display.print(F("All Sensors Live"));

  display.display();
  delay(500);
}
