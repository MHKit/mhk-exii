
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

unsigned long counter = 0;

struct finger {
  Servo servo;
  float min;
  float max;

  float angle;
  RunningMedian median = RunningMedian(3);

  unsigned long lastUpdate;
};

void setupFinger(struct finger *f, int pin, float min, float max) {
  f->servo.attach(pin);
  f->min = min;
  f->max = max;
  f->angle = min + (max - min) / 2;
  f->lastUpdate = millis();
}

void loopFinger(struct finger *f) {
  f->angle = min(f->max, max(f->min, f->angle));
  f->median.add(f->angle);

  if (millis() - f->lastUpdate > 15) {
    f->lastUpdate = millis();
    f->servo.write(f->median.getAverage());
  }
}

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

void setupMyo(struct myo *m, int pin) {
  m->pin = pin;
}

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

void resetMyo(struct myo *m) {
  m->state = MYO_IDLE;
  m->minimumThreshold = 0;
  m->shortMedian.clear();
  m->longMedian.clear();
  m->diffMedian.clear();
  m->minDiffMedian.clear();
}

struct myo m1;
struct myo m2;

struct finger thumb;
struct finger index;
struct finger middle;

void setup() {
  pinMode(7, INPUT);
  digitalWrite(7, HIGH);
  
  setupMyo(&m1, A0);
  setupMyo(&m2, A6);

  setupFinger(&thumb, 6, 60, 158);
  setupFinger(&index, 3, 27, 150);
  setupFinger(&middle, 5, 35, 150);

  Serial.begin(115200);
}

void loop() {
  if (digitalRead(7) == LOW) {
    resetMyo(&m1);
    resetMyo(&m2);
  }
  
  loopMyo(&m1);
  loopMyo(&m2);

  if (!(m1.state && m2.state)) { 
    if (m1.state == MYO_ACTIVE) {
      index.angle += m1.minDiffMedian.getMedian() / 700;
      middle.angle = map(index.angle, index.min, index.max, middle.max, middle.min);
    }
  
    if (m2.state == MYO_ACTIVE) {
      index.angle -= m2.minDiffMedian.getMedian() / 700;
      middle.angle = map(index.angle, index.min, index.max, middle.max, middle.min);
    }
  }

  loopFinger(&thumb);
  loopFinger(&index);
  loopFinger(&middle);

  Serial.print(m1.shortMedian.getMedian());
  Serial.print(", ");
  Serial.print(m1.minimumThreshold);
  Serial.print(", ");
  Serial.print(m2.shortMedian.getMedian());
  Serial.print(", ");
  Serial.print(m2.minimumThreshold);
  //Serial.print(index.median.getMedian());
  Serial.println();
  ++counter;
}

