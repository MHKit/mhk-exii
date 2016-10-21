
/*
  ReadAnalogVoltage
  Reads an analog input on pin 0, converts it to voltage, and prints the result to the serial monitor.
  Graphical representation is available using serial plotter (Tools > Serial Plotter menu)
  Attach the center pin of a potentiometer to pin A0, and the outside pins to +5V and ground.

  This example code is in the public domain.
*/

#include <RunningMedian.h>
#include <Servo.h>

#define MEDIAN_MAX_SIZE     41

Servo servo;
unsigned long t = 0;

unsigned long counter = 0;

#define MYO_IDLE 0
#define MYO_ACTIVE 1

struct myo {
  RunningMedian shortMedian = RunningMedian(41);
  RunningMedian longMedian = RunningMedian(3);
  RunningMedian diffMedian = RunningMedian(21);
  RunningMedian minDiffMedian = RunningMedian(21);

  int state;
  int pin;
  float minimumThreshold;
};

void loopMyo(struct myo *m) {
  m->shortMedian.add(analogRead(m->pin));
  if (counter % 10 == 0) {
    m->longMedian.add(m->shortMedian.getMedian());
  }
  m->diffMedian.add(m->shortMedian.getMedian() - m->longMedian.getMedian());
  m->minDiffMedian.add(m->shortMedian.getMedian() - m->minimumThreshold);

  if (m->state == MYO_IDLE && m->diffMedian.getMedian() > 10) {
    m->state = MYO_ACTIVE;
    m->minimumThreshold = m->longMedian.getMedian();
    m->minDiffMedian.clear();
    m->minDiffMedian.add(10);
  } else if (m->state == MYO_ACTIVE && m->minDiffMedian.getAverage() < 15) {
    m->state = MYO_IDLE;
    //m->minimumThreshold = 0;
  }
}

struct myo m1;
struct myo m2;

RunningMedian servoMedian = RunningMedian(3);

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  pinMode(9, OUTPUT);

  servo.attach(3);

  m1.pin = A0;
  m2.pin = A1;

  t = millis();
  
  Serial.begin(115200);
}

float servoPos = 90;

void loop() {
  loopMyo(&m1);
  loopMyo(&m2);
  /*analogWrite(9, myoState == MYO_ACTIVE ? map(shortMedian.getMedian() - minimumThreshold, 0, 1024, 0, 255) : 0);
  if (myoState == MYO_ACTIVE && millis() - t > 30) {
    t = millis();
    servo.write(map(shortMedian.getMedian() - minimumThreshold, 0, 1024, 0, 180));
  }*/

  if (!(m1.state && m2.state)) { 
    if (m1.state == MYO_ACTIVE) {
      servoPos += m1.minDiffMedian.getMedian() / 400;
    }
  
    if (m2.state == MYO_ACTIVE) {
      servoPos -= m2.minDiffMedian.getMedian() / 400;
    }
  }

  servoPos = min(102, max(35, servoPos));
  servoMedian.add(servoPos);

  if (millis() - t > 15) {
    t = millis();
    servo.write(servoMedian.getAverage());
  }

  //Serial.print(servoMedian.getAverage());
  Serial.print(m1.shortMedian.getMedian());
  Serial.print(", ");
  Serial.print(m1.minimumThreshold);
  Serial.print(", ");
  Serial.print(m2.shortMedian.getMedian());
  Serial.print(", ");
  Serial.print(m2.minimumThreshold);
  Serial.println();
  ++counter;
}

