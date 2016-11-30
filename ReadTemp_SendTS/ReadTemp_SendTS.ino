#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <OneWire.h>
#include <ESP8266wifi.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
OneWire  ds(12);  // on pin 10 (a 4.7K resistor is necessary)

#include <SoftwareSerial.h> //Software Serial is needed to interface with ESP8266 because the hardware serial 
//is being used for monitoring Serial line with computer during trouble shooting.
SoftwareSerial ESP8266(10, 11); //RX,TX

#define IP "184.106.153.149" // thingspeak.com IP address .. 184.106.153.149 ; see their website for more detail
String GET = "GET /update?key=X6OILVIV03M3276J&field"; // you 16-digit API key
String field1 = "1=";
String field2 = "2=";
String field3 = "3=";
String field4 = "4=";

byte i;
byte present = 0;
byte type_s;
byte data[12];
byte addr[8];
float celsius, fahrenheit;
byte counting = 0;


void setup() {
  pinMode(12, INPUT); //DS18B20
  pinMode(13, OUTPUT); //LED pin

  lcd.begin();
  lcd.setCursor(0, 0);
  lcd.print("LCDisplay");
  lcd.setCursor(0, 1);
  lcd.print("Hello World");
  delay(2000);
  lcd.clear();

  Serial.begin(9600);
  ESP8266.begin(9600); // this is to start serial communication with the ESP via Software Serial.
  ESP8266.println("AT+RST"); // this check the ESP8266.
  Serial.println("AT+RST");
  delay(2000);
  ESP8266.println("AT");
  Serial.println("AT");
  delay(2000);
  if (ESP8266.find("OK")) {
    Serial.println("OK");
    Serial.println("Connected");
  } else 
    Serial.println("Lost Connection to ESP8266");
}


// the loop routine runs over and over again forever:
void loop(void) {

  digitalWrite(13, HIGH); //LED on when program work :))

  if ( !ds.search(addr)) {
    //Serial.println("No more addresses.");
    //Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end

  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }
  //  Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }

  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;

  counting = counting + 1;

  Serial.print("  Count ");
  Serial.print(counting);
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius., ");

  digitalWrite(13, LOW);
  Serial.println();

  lcd.setCursor(5, 0);
  lcd.print(celsius);
  lcd.setCursor(4, 1);
  lcd.print("Celsius");

  update((String)celsius, field1);

  delay(10000);   //delay program in millisecond, here i delay 10sec
}


void update(String value, String field) {

  Serial.println("in the update loop");
  ESP8266.println("AT+RST"); // it seems that mine works better if I reset it everytime before I do CIPSTART
  Serial.println("AT+RST");
  delay(5000);
  String cmd = "AT+CIPSTART=\"TCP\",\""; //standard code. see https://github.com/espressif/esp8266_at/wiki //
  cmd += IP; // += is concatenating the previous cmd string with the content of IP
  cmd += "\",80"; // same thing here it just keep adding stuff to the cmd string content

  ESP8266.println(cmd);//type in the string in cmd into the ESP8266
  Serial.println(cmd);
  delay(5000);

  if (ESP8266.find("Error")) {
    Serial.println("AT+CIPSTART Error");
    return;
  }
  cmd = GET;
  cmd += field;
  cmd += value;
  cmd += "\r\n\r\n"; // it seems that this part is important, to input enough \r\n, mine does not work if I only have one pair.
  ESP8266.print("AT+CIPSEND=");
  ESP8266.println(cmd.length());
  Serial.print("AT+CIPSEND=");
  Serial.println(cmd.length());
  delay(10000);

  if (ESP8266.find(">")) { // if ESP8266 return the > prompt back from the CIPSEND command, you are set, if not something is wrong
    ESP8266.print(cmd); //type in the GET command string and done
    ESP8266.print("AT+CIPSEND");
    //ESP8266.print("AT+CIPCLOSE");
    Serial.print(">");
    Serial.print(cmd);
    Serial.println(" Update Data to Thinkspeak...");
    delay(5000);
    Serial.println("> OK!!!");


  }
  else {
    ESP8266.print("AT+CIPCLOSE");
    //Serial.println("AT+CIPCLOSE"); //this command somehow does not work and always return errors so I just comment it out
    Serial.println("AT+CIPSEND error");
    delay(1000);
  }

}
