#if defined(ARDUINO_ESP32_DEV) || defined(ARDUINO_AVR_MINI)

#include <SPI.h>
#include <LoRa.h>
// Dog specific
String Marco = "SPARKY", Polo = "WOOF";

void setup() {
  // Only use SS and no reset or dio0
  LoRa.setPins(SS, -1, -1);
  // External switch
  pinMode(4, OUTPUT);
}

void loop() {
  // 915 MHz
  LoRa.begin(915E6);
  // Make sure you are getting right size
  while (LoRa.parsePacket() != Marco.length());
  String txt = "";
  // Build the string
  while (LoRa.available()) txt += char(LoRa.read());
  // Are you calling my name?
  if (txt == Marco) {
    // External switch on then off
    digitalWrite(4, 1);
    delay(100);
    digitalWrite(4, 0);
    delay(1000);
    // Reply to the call
    LoRa.beginPacket();
    LoRa.print(Polo);
    LoRa.endPacket();
  }
  LoRa.end();
}

#elif defined(ARDUINO_spresense_ast)

#include <GNSS.h>
#include <SDHCI.h>
#include <File.h>
static SpGnss Gnss;
SpNavData NavData;
SDClass SD;
File LogFile;
// This would come from Helium
String Marco = "SPARKY", Polo = "WOOF";
int VisualContact = 0;

void setup() {
  // Setup LEDs
  for (int j = 0; j < 4; j++) pinMode(_LED_PIN(j), OUTPUT);
  // Communicate with B_L072Z_LRWAN1
  Serial2.begin(1000000);
  delay(3000);
  Gnss.begin();
  // Send every 20 seconds
  Gnss.setInterval(20);
  // USA
  Gnss.select(GPS);
  Gnss.start(COLD_START);
  // To reset B_L072Z_LRWAN1
  pinMode(15, OUTPUT);
  digitalWrite(15, 1);
}

void loop()
{
  if (Gnss.waitUpdate(-1))
  {
    for (int j = 0; j < 4; j++) digitalWrite(_LED_PIN(j), 0);
    digitalWrite(_LED_PIN(0), 1);
    Gnss.getNavData(&NavData);
    // Use only valid data
    if (NavData.posDataExist != 0) {
      digitalWrite(_LED_PIN(1), 1);
      // Reset B_L072Z_LRWAN1
      for (int i = 0; i < 2; i++) {
        digitalWrite(15, !digitalRead(15));
        delay(500);
      }
      // Comma separated data
      String CSV = String(NavData.latitude, 5) + "," + String(NavData.longitude, 5) + "," + String(NavData.altitude, 5) + "," + String(NavData.velocity, 5) + "," + Marco + "," + VisualContact;
      // Save locally
      LogFile = SD.open(String(NavData.time.year) + String((NavData.time.month < 10) ? "0" : "") + String(NavData.time.month) + String((NavData.time.day < 10) ? "0" : "") + String(NavData.time.day) + ".log", FILE_WRITE);
      if (LogFile) {
        // Add timestamp
        LogFile.println(String((NavData.time.hour < 10) ? "0" : "") + String(NavData.time.hour) + String((NavData.time.minute < 10) ? "0" : "") + String(NavData.time.minute) + String((NavData.time.sec < 10) ? "0" : "") + String(NavData.time.sec) + "," + CSV);
        LogFile.close();
      }
      // Send data starting with "|"
      Serial2.print( "|" + CSV );
      digitalWrite(_LED_PIN(2), 1);
      for (int i = 0; i < 120; i++) {
        delay(100);
        String UARTText = "";
        // Build the string
        while (Serial2.available() > 0) UARTText += char(Serial2.read());
        // Is it the expected reply?
        VisualContact = (UARTText == Polo ? 1 : 0);
        VisualContact == 1 ? digitalWrite(_LED_PIN(3), 1) : digitalWrite(_LED_PIN(0), 0);
        if (UARTText != "") break;
      }
    }
  }
}

#elif defined(ARDUINO_STM32L0_B_L072Z_LRWAN1)

#include "LoRaWAN.h"
#include "LoRaRadio.h"
#include "CayenneLPP.h"

const char *devEui = "";
const char *appEui = "";
const char *appKey = "";

void setup()
{
  const int Green = 4;
  const int Red = 5;
  const int Blue = 10;
  // Setup LEDs
  pinMode(Red, OUTPUT);
  pinMode(Green, OUTPUT);
  pinMode(Blue, OUTPUT);
  // Communicate with GPS
  Serial1.begin(1000000);
  // US Region
  LoRaWAN.begin(US915);
  // Helium SubBand
  LoRaWAN.setSubBand(2);
  // Disable Adaptive Data Rate
  LoRaWAN.setADR(false);
  // Set Data Rate 1 - Max Payload 53 Bytes
  LoRaWAN.setDataRate(1);
  // Try to skip activation
  LoRaWAN.setSaveSession(true);
  // Device IDs and Key
  LoRaWAN.joinOTAA(appEui, appKey, devEui);
  // Wait until join
  delay(5000);
  String UARTText = "";
  // Build the string
  while (Serial1.available() > 0) UARTText += char(Serial1.read());
  // We have text
  if (UARTText.startsWith("|")) {
    int Commas[7];
    // Find commas in string
    for (int i = 1; i < 7; i++) Commas[i] = UARTText.indexOf(',', Commas[i - 1] + 1);
    float Lat = UARTText.substring(Commas[1] + 2, Commas[2]).toFloat(), Lon = UARTText.substring(Commas[2] + 1, Commas[3]).toFloat();
    // Convert m to feet & m/s to mph
    float Alt = UARTText.substring(Commas[3] + 1, Commas[4]).toFloat() * 3.28084, Vel = UARTText.substring(Commas[4] + 1, Commas[5]).toFloat() * 2.23694;
    String Marco = UARTText.substring(Commas[5] + 1, Commas[6]);
    int VisualContact = UARTText.substring(Commas[6] + 1).toInt();
    // Board is not busy
    if (LoRaWAN.joined() && !LoRaWAN.busy())
    {
      digitalWrite(Blue, 1);
      CayenneLPP ThisLPP(64);
      ThisLPP.addGPS(0, Lat, Lon, Alt);
      ThisLPP.addAnalogInput( 1, Vel);
      ThisLPP.addDigitalInput(2, VisualContact);
      // Send it bro!
      LoRaWAN.sendPacket(ThisLPP.getBuffer(), ThisLPP.getSize());
      while (LoRaWAN.busy());
      digitalWrite(Blue, 0);
      LoRaRadio.begin(915E6);
      LoRaRadio.beginPacket();
      LoRaRadio.print(Marco);
      LoRaRadio.endPacket();
      // Three attempts to get reply
      for (int i = 0; i < 3; i++) {
        LoRaRadio.receive();
        delay(1000);
        LoRaRadio.parsePacket();
        String txt = "";
        while (LoRaRadio.available()) txt += char(LoRaRadio.read());
        if (txt != "") {
          digitalWrite(Red, !digitalRead(Red));
          Serial1.print(txt);
          break;
        }
      }
    }
  }
}

void loop()
{
}
#else
#error Unsupported board selection.
#endif
