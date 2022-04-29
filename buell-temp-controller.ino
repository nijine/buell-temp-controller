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
  delay(500); // gives the ECM time to start replying
}

void evalTemp() {
//  unsigned eTemp = (rtdResponse[31] << 8 | rtdResponse[30]) * 0.18 - 40; // Fahrenheit value
  unsigned eTemp = (rtdResponse[31] << 8 | rtdResponse[30]) * 0.1 - 40; // Celsius value
  lcd.setCursor(0,0);
  lcd.print("Temp: ");
  lcd.print(eTemp);
  lcd.print(" *C "); // extra space is a "clearing" hack for the LCD, in case our temp changes over to a lower order-of-magnitude
}

void evalVolts() {
  unsigned voltsRaw = (rtdResponse[29] << 8 | rtdResponse[28]); // volts * 100, i.e. 1250
  int right = voltsRaw % 100;
  int left = (voltsRaw - right) / 100;
  lcd.setCursor(0,1);
  lcd.print("Batt: ");

//  if (left < 10) // minor hack to keep the digits in the right place
//    lcd.print(" "); // its omitted here because if our battery voltage was under 10v, we'd have bigger problems than formatting
  lcd.print(left);
  lcd.print(".");

  if (right < 10) // same as above, formatting hack for right-of-the-decimal
    lcd.print("0");
  lcd.print(right);
  lcd.print(" V"); 
}

void setFan() {
  unsigned eTemp = (rtdResponse[31] << 8 | rtdResponse[30]) * 0.1 - 40; // Celsius value

  if (eTemp > 180)
    analogWrite(FAN_PWM_PIN, 255); // 100% / fully on
  else
    analogWrite(FAN_PWM_PIN, 0); // 0% / off
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
      setFan(); // turn fan on or off depending on temp
      delay(4500); // no need to refresh this any faster
      sendRTH(); // send another runtime data request
    } else {
      retryRequest();
    }
  }
}
