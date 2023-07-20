// Don't forget to make sure SERIAL_RX_BUFFER_SIZE is set to at least 128 in HardwareSerial.h for collecting buell runtime data!
// Credit to Federico Dossena for the bulk of the PWM / timer setup code -> https://fdossena.com/?p=ArduinoFanControl/i.md

#define FAN_PWM_PIN 10

#include <LiquidCrystal_PCF8574.h>
#include <Wire.h>

LiquidCrystal_PCF8574 lcd(0x27);

int timedOutCounter = 0;
int bytePosition = 0;
int responseSize = 99; // response data size

byte rtdHeader[9];
byte rtdResponse[99];

// Configure Timer 1 (pins 9 and 10) to output 25kHz PWM. This is necessary for most PC
 // 4-pin fans to work correctly, since that's what the spec calls for.
void setupTimer1(){
    pinMode(FAN_PWM_PIN, OUTPUT);

    // Set PWM frequency to about 25khz on pins 9 and 10
    // (timer 1 mode 10, no prescale, count to 320)
    TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);
    TCCR1B = (1 << CS10) | (1 << WGM13);
    ICR1 = 320;
    OCR1A = 0;
    OCR1B = 0;
}

// Equivalent of analogWrite on pin 10
void setPWM1B(float f){
    f=f<0?0:f>1?1:f; // shorthand if block that binds f to 0 <= f <= 1
    OCR1B = (uint16_t)(320*f);
}

void setup() {
  rtdHeader[0]=0x01; //SOH
  rtdHeader[1]=0x00; //Emittend
  rtdHeader[2]=0x42; //Recipient
  rtdHeader[3]=0x02; //Data Size
  rtdHeader[4]=0xFF; //EOH
  rtdHeader[5]=0x02; //SOT
  rtdHeader[6]=0x43; //Data 1 -> 0x56 = Get version, 0x43 = Get runttime data
  rtdHeader[7]=0x03; //EOT
  rtdHeader[8]=0xFD; //Checksum

  lcd.begin(16, 2);
  lcd.setBacklight(255);
  lcd.clear();

  // PWM init
  setupTimer1();

  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  // Kick off ECM communication
  sendRTH();
}

void sendRTH() {
  Serial.write((char*) rtdHeader, 9);
  delay(250); // gives the ECM time to write the reply
}

void evalTemp() {
//  unsigned eTemp = (rtdResponse[31] << 8 | rtdResponse[30]) * 0.18 - 40; // Fahrenheit value

  // Using int here since we can occasionally get negative values if there's a problem with
   // the temp sensor, and we want that to display correctly.
  int eTemp = (rtdResponse[31] << 8 | rtdResponse[30]) * 0.1 - 40; // Celsius value
  lcd.setCursor(0,0);
  lcd.print("T:");

  if (eTemp >= 0 && eTemp < 100)
    lcd.print(" "); // formatting hack

  lcd.print(eTemp);
}

void evalLoad() { // TPS
  unsigned eLoad = rtdResponse[27]; // 1-byte value, engine load as percent * 2.55 (0-255)
  lcd.setCursor(7,0);
  lcd.print("L:");

  if (eLoad < 100)
    lcd.print(" "); // formatting hack

  lcd.print(eLoad);
}

void evalO2() {
  unsigned eO2 = (rtdResponse[35] << 8 | rtdResponse[34]) * 0.4888; // O2 voltage * 100
  lcd.setCursor(7,1);
  lcd.print("O:");
  lcd.print(eO2);
}

void evalVolts() {
  unsigned eVolts = (rtdResponse[29] << 8 | rtdResponse[28]); // volts * 100, i.e. 1250
  lcd.setCursor(0,1);
  lcd.print("V:");

  if (eVolts < 1000) // minor hack to keep the digits in the right place
    lcd.print(" ");

  lcd.print(eVolts);
}

void setFan() {
  unsigned eTemp = (rtdResponse[31] << 8 | rtdResponse[30]) * 0.1 - 40; // Celsius value

  // Yes, I know switch-case is a thing, I was too lazy to look up the syntax

  // Fan curve, start at around 100C to prevent fuel from heating up excessively
  if (eTemp > 99 && eTemp <= 120) {
    setPWM1B(0.4f); // 40%
    lcd.setCursor(15, 1);
    lcd.print("*");
  } else if (eTemp > 120 && eTemp <= 130) {
    setPWM1B(0.47f);
    lcd.setCursor(15, 1);
    lcd.print("*");
  } else if (eTemp > 130 && eTemp <= 140) {
    setPWM1B(0.54f);
    lcd.setCursor(15, 1);
    lcd.print("*");
  } else if (eTemp > 140 && eTemp <= 150) {
    setPWM1B(0.61f);
    lcd.setCursor(15, 1);
    lcd.print("*");
  } else if (eTemp > 150 && eTemp <= 160) { // Target operating temp
    setPWM1B(0.68f);
    lcd.setCursor(15, 1);
    lcd.print("*");
  } else if (eTemp > 160 && eTemp <= 170) {
    setPWM1B(0.75f);
    lcd.setCursor(15, 1);
    lcd.print("*");
  } else if (eTemp > 170 && eTemp <= 180) {
    setPWM1B(0.82f);
    lcd.setCursor(15, 1);
    lcd.print("*");
  } else if (eTemp > 180 && eTemp <= 190) {
    setPWM1B(0.89f);
    lcd.setCursor(15, 1);
    lcd.print("*");
  } else if (eTemp > 190) { // Upper limit
    setPWM1B(1.0f);
    lcd.setCursor(15, 1);
    lcd.print("*");
  } else {
    setPWM1B(0.0f); // 0% / off
    lcd.setCursor(15, 1);
    lcd.print(" ");
  }
}

void retryRequest() { // retry logic
  if (timedOutCounter < 3) {
    timedOutCounter++;

    // print a timeout indicator
    lcd.setCursor(15, 1);
    lcd.print("X");
    delay(500);
    lcd.setCursor(15, 1);
    lcd.print(" ");
    delay(500);
  } else {
    timedOutCounter = 0;
    bytePosition = 0;
    sendRTH();
  }
}

void loop() { // arduino runtime loop
  if (Serial.available()) {
    rtdResponse[bytePosition++] = Serial.read();
  } else {
    if (bytePosition == responseSize) {
      bytePosition = 0; // reset to start

      evalTemp(); // print temp
      evalLoad(); // print tps / load
      evalVolts(); // print battery voltage
      evalO2(); // print O2 sensor voltage
      setFan(); // turn fan on or off depending on temp
      sendRTH(); // send another runtime data request
    } else {
      retryRequest();
    }
  }
}
