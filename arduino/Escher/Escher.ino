#include <vector>

#include <Wire.h>
#include <AccelStepper.h>
#include <Adafruit_MotorShield.h>

#define MAX_SPEED 50.0
#define ACCELERATION 500.0

Adafruit_MotorShield AFMS = Adafruit_MotorShield();

Adafruit_StepperMotor *myStepper1 = AFMS.getStepper(200, 1);
Adafruit_StepperMotor *myStepper2 = AFMS.getStepper(200, 2);
void forwardstep1() {  
  myStepper1->onestep(FORWARD, SINGLE);
}
void backwardstep1() {  
  myStepper1->onestep(BACKWARD, SINGLE);
}
void forwardstep2() {  
  myStepper2->onestep(FORWARD, DOUBLE);
}
void backwardstep2() {  
  myStepper2->onestep(BACKWARD, DOUBLE);
}

class Escher {
  public:
  Escher(AccelStepper& xstepper, AccelStepper& ystepper) : _xstepper(xstepper), _ystepper(ystepper) {
    _xstepper.setMaxSpeed(MAX_SPEED);
    _xstepper.setAcceleration(ACCELERATION);
    _ystepper.setMaxSpeed(MAX_SPEED);
    _ystepper.setAcceleration(ACCELERATION);
  }

  void moveTo(int x, int y) {
    Serial.printf("moveTo (%d, %d)\n", x, y);
    _xstepper.moveTo(x);
    _ystepper.moveTo(y);
  }

  void push(int x, int y) {
    Serial.printf("push (%d, %d)\n", x, y);
    std::pair<int, int> coord(x, y);
    _pending.push_back(coord);
  }

  void run() {
    if (_xstepper.distanceToGo() == 0 && _ystepper.distanceToGo() == 0 && !_pending.empty()) {
      std::pair<int, int> front = _pending.front();
      Serial.printf("Next waypoint: (%d, %d)\n", front.first, front.second);
      _xstepper.moveTo(front.first);
      _ystepper.moveTo(front.second);
      _pending.erase(_pending.begin());
    }

    _xstepper.run();
    _ystepper.run();
  }

  private:
  AccelStepper& _xstepper;
  AccelStepper& _ystepper;
  std::vector<std::pair<int, int>> _pending;
  
};

AccelStepper stepper1(forwardstep1, backwardstep1);
AccelStepper stepper2(forwardstep2, backwardstep2);
Escher escher(stepper1, stepper2);

void setup() {  
  AFMS.begin(); // Start the bottom shield

  escher.push(100, 0);
  escher.push(100, 100);
  escher.push(0, 100);
  escher.push(0, 0);
}

void loop() {
  escher.run();
}
