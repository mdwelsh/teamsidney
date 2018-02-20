/*
  Fade

  This example shows how to fade an LED on pin 9 using the analogWrite()
  function.

  The analogWrite() function uses PWM, so if you want to change the pin you're
  using, be sure to use another PWM capable pin. On most Arduino, the PWM pins
  are identified with a "~" sign, like ~3, ~5, ~6, ~9, ~10 and ~11.

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/Fade
*/

int redPin = 3;
int greenPin = 5;
int bluePin = 6;
int colorPot = 0;
int dimPot = 0;

int red = 0;
int green = 0;
int blue = 0;

// the setup routine runs once when you press reset:
void setup() {
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
}

// the loop routine runs over and over again forever:
void loop() {
  colorPot = analogRead(A0);
  dimPot = analogRead(A1);
  
  if (colorPot < 100) {
    red = constrain(colorPot, 0, 255);
  }
  if (colorPot < 300) {
    green = constrain(colorPot-100, 0, 255);
  }
  if (colorPot < 500) {
    blue = constrain(colorPot-300, 0, 255);
  }

  red = red * (dimPot / 1024.0);
  green = green * (dimPot / 1024.0);
  blue = blue * (dimPot / 1024.0);

  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);

  
}
