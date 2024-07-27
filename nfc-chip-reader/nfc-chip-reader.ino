// NFC Reader
// 

// Libraries
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>

PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

//#define BAUDRATE 9600
#define BAUDRATE 115200

// Define Pins
#define BLUE 10
#define GREEN 9
#define RED 11

// Light control and Update speed
#define COLOR_MAX 10
#define UPDATE_DELAY 10

enum StatusState 
{
  off,
  reading,
  writing,
  on
};

struct StatusLight 
{
  int pin;
  bool isOn;
  int brightness;
  int offDuration;
  int onDuration;
  int blinkTimer;
  StatusState state;
};

StatusLight PowerLight;
StatusLight CardLight;
StatusLight GameLight;

void setup()
{
  Serial.begin(BAUDRATE);
  Serial.println();

  PowerLight = StatusLight {RED, false, COLOR_MAX, 1000, 1000, 0, StatusState::on};
  CardLight = StatusLight {BLUE, false, COLOR_MAX, 1500, 150, 0, StatusState::reading};
  GameLight = StatusLight {GREEN, false, COLOR_MAX, 450, 100, 0, StatusState::off};

  pinMode(PowerLight.pin, OUTPUT);
  pinMode(GameLight.pin, OUTPUT);
  pinMode(CardLight.pin, OUTPUT);
  analogWrite(PowerLight.pin, LOW);
  digitalWrite(GameLight.pin, LOW);
  digitalWrite(CardLight.pin, LOW);

  nfc.begin();
  Serial.println();
  Serial.println("Getting formware version...");
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata)
  {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }

  // Got ok data, print it out!
  Serial.print("Found chip PN5");
  Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); 
  Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata>>8) & 0xFF, DEC);
  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.  nfc.setPassiveActivationRetries(0xFF);    // configure board to read RFID tags  nfc.SAMConfig();      Serial.println("Waiting for an ISO14443A card");

}

bool isReadyToBlink(StatusLight light)
{
  bool ready = false;

  if (light.isOn)
  {
    ready = light.blinkTimer >= light.onDuration;
  }
  else
  {
    ready = light.blinkTimer >= light.offDuration;
  }

  return ready;
}

StatusLight BlinkLight(StatusLight light)
{
  if (light.isOn)
  {
    digitalWrite(light.pin, LOW);
    light.isOn = false;
  }
  else
  {
    analogWrite(light.pin, light.brightness);
    light.isOn = true;
  }

  return light;
}

StatusLight TurnOff(StatusLight light)
{
  digitalWrite(light.pin, LOW);
  light.isOn = false;
  return light;
}

StatusLight TurnOn(StatusLight light)
{
  analogWrite(light.pin, light.brightness);
  light.isOn = true;
  return light;
}

StatusLight ManageLight(StatusLight light)
{
  switch(light.state) 
  {
      case StatusState::off:
        if (light.isOn) { return TurnOff(light); }
      break;

      case StatusState::on:
        if (!light.isOn) { return TurnOn(light); }
      break;

      case StatusState::reading:
      case StatusState::writing:
        light.blinkTimer = light.blinkTimer + UPDATE_DELAY;
        if (isReadyToBlink(light))
        {
          light = BlinkLight(light);
          light.blinkTimer = 0;
        }
      break;
  }

  return light;
}

// main loop
void loop()
{
  // Node JS Interactions
  // ...
  readCard();

  // Update lights based on state
  PowerLight = ManageLight(PowerLight);
  CardLight = ManageLight(CardLight);
  GameLight = ManageLight(GameLight);

  // Prepare for next loop
  delay(UPDATE_DELAY);
}

void readCard()
{
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  // Buffer to store the returned UID
  uint8_t uidLength;
  // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  if (success) 
  {
    Serial.println("Found a card!");
    Serial.print("UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("UID Value: ");
    String hex_value = "";
    for (uint8_t i=0; i < uidLength; i++)
    {
      Serial.print(" 0x");Serial.print(uid[i], HEX);
      //Serial.print(" ");Serial.print(uid[i], HEX);
      hex_value += (String)uid[i];
    }
    Serial.println(", value="+hex_value);
    if(hex_value == "16517722582") 
    {
      Serial.println("This is Key Tag. ");
    }
    else if(hex_value == "230522426") 
    {
      Serial.println("This is Card Tag. ");
    }
    else if(hex_value == "63156295") 
    {
      Serial.println("This is Phone Tag. ");
    }
    else Serial.println("I don't know.");
    Serial.println("");
    // Wait 1 second before continuing    
    delay(1000);
  }
  else
  {
    // PN532 probably timed out waiting for a card    
    Serial.println("Waiting for a card...");
  }

}
