// Don't forget to make sure SERIAL_RX_BUFFER_SIZE is set to at least 128 in HardwareSerial.h for collecting buell runtime data!

#define FAN_PWM_PIN 9

#include <LiquidCrystal_PCF8574.h>
#include <Wire.h>

LiquidCrystal_PCF8574 lcd(0x27);

int timedOutCounter = 0;
int bytePosition = 0;
int responseSize = 99; // response data size

byte rtdHeader[9];
byte rtdResponse[99];

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

  // Turn off fan initially
  analogWrite(FAN_PWM_PIN, 0);

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  sendRTH();
}

void sendRTH() {
  Serial.write((char*) rtdHeader, 9);
  delay(250); // gives the ECM time to start replying
}

void evalTemp() {
//  unsigned eTemp = (rtdResponse[31] << 8 | rtdResponse[30]) * 0.18 - 40; // Fahrenheit value
  unsigned eTemp = (rtdResponse[31] << 8 | rtdResponse[30]) * 0.1 - 40; // Celsius value
  lcd.setCursor(0,0);
  lcd.print("T:");

  if (eTemp < 100)
    lcd.print(" "); // formatting hack

  lcd.print(eTemp);
}

void evalO2() {
  unsigned eO2 = (rtdResponse[35] << 8 | rtdResponse[34]) * 0.4888; // O2 voltage * 100 
  lcd.setCursor(7,1);
  lcd.print("O:");
  lcd.print(eO2);
}

void evalFuel() {
  unsigned eFuelFront = (rtdResponse[22] << 8 | rtdResponse[21]) * 0.0133; // Fuel Pulsewidth in ms * 10
  unsigned eFuelRear = (rtdResponse[24] << 8 | rtdResponse[23]) * 0.0133; // Fuel Pulsewidth in ms * 10

  lcd.setCursor(7,0);

  if (eFuelFront < 100)
    lcd.print(" ");

  lcd.print(eFuelFront);

  lcd.setCursor(11,0);

  if (eFuelRear < 100)
    lcd.print(" ");
  
  lcd.print(eFuelRear);
}

void evalVolts() {
  unsigned voltsRaw = (rtdResponse[29] << 8 | rtdResponse[28]); // volts * 100, i.e. 1250
  lcd.setCursor(0,1);
  lcd.print("B:");

  if (voltsRaw < 1000) // minor hack to keep the digits in the right place
    lcd.print(" ");

  lcd.print(voltsRaw);
}

void setFan() {
  unsigned eTemp = (rtdResponse[31] << 8 | rtdResponse[30]) * 0.1 - 40; // Celsius value

  if (eTemp > 179) {
    analogWrite(FAN_PWM_PIN, 255); // 100% / fully on
    lcd.setCursor(15, 1);
    lcd.print("*");
  } else {
    analogWrite(FAN_PWM_PIN, 0); // 0% / off
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

void loop() { // run over and over
  if (Serial.available()) {
    rtdResponse[bytePosition++] = Serial.read();
  } else {
    if (bytePosition == responseSize) {
      bytePosition = 0; // reset to start

      evalTemp(); // print temp
      evalVolts(); // print battery voltage
      evalO2(); // print O2 sensor voltage
      evalFuel(); // print fuel pulsewidths
      setFan(); // turn fan on or off depending on temp
      delay(250);
      sendRTH(); // send another runtime data request
    } else {
      retryRequest();
    }
  }
}
