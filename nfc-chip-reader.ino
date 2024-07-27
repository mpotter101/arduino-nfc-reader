// NFC Reader
// 

#define BAUDRATE = 9600;

// Define Pins
#define BLUE 5
#define GREEN 3
#define RED 6

// 
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

  PowerLight = StatusLight {RED, false, COLOR_MAX, 1000, 1000, 0, StatusState::on};
  CardLight = StatusLight {BLUE, false, COLOR_MAX, 1500, 150, 0, StatusState::reading};
  GameLight = StatusLight {GREEN, false, COLOR_MAX, 450, 100, 0, StatusState::off};

  pinMode(PowerLight.pin, OUTPUT);
  pinMode(GameLight.pin, OUTPUT);
  pinMode(CardLight.pin, OUTPUT);
  analogWrite(PowerLight.pin, LOW);
  digitalWrite(GameLight.pin, LOW);
  digitalWrite(CardLight.pin, LOW);
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

  // Update lights based on state
  PowerLight = ManageLight(PowerLight);
  CardLight = ManageLight(CardLight);
  GameLight = ManageLight(GameLight);

  // Prepare for next loop
  delay(UPDATE_DELAY);
}

