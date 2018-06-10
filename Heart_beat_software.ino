#include "arduinoFFT.h"

//#define DEBUG
//#define FFT_DEBUG
//#define INDEX_DEBUG

#define SAMPLES 32             //Must be a power of 2
#define SAMPLING_FREQUENCY 7000 //Hz, must be less than 10000 due to ADC

#define POWER_THRESHOLD 300
#define MINTIME 75
#define TIMEOUT 300

arduinoFFT FFT = arduinoFFT();

unsigned int sampling_period_us;
unsigned long microseconds;
unsigned long miliseconds;

double vReal[SAMPLES];
double vImag[SAMPLES];

int curState;
int nextState;
int pwm;
int newpwm;

void setup() {
  
#ifdef DEBUG
  Serial.begin(115200);
#endif

  pinMode(9, OUTPUT);
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
  curState = 0;
  nextState = 0;
  pwm = 0;
}

void loop() {

 
  for (int i = 0; i < SAMPLES; i++) {
    microseconds = micros();    //Overflows after around 70 minutes!

    vReal[i] = analogRead(A0);
    vImag[i] = 0;

    while (micros() < (microseconds + sampling_period_us)) {
    }
  }
  
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);

  #ifdef FFT_DEBUG
  for(int i = 0; i < SAMPLES/2; i++){
    Serial.println(vReal[i]);
  }
  #endif

  #ifdef DEBUG
  Serial.println();
  #endif

  int index = SAMPLES * 0.185;
  double pv = vReal[index];

  #ifdef INDEX_DEBUG
  
  Serial.print("First index = ");
  Serial.println(index);
  Serial.print("First max value = ");
  Serial.println(pv);
  
  #endif

  for (int i = index; i < (SAMPLES / 2); i++) {
    if (vReal[i] > pv) {
      index = i;
      pv = vReal[i];
    }
  }
  #ifdef INDEX_DEBUG
  Serial.print("Second index = ");
  Serial.println(index);
  #endif

  switch (curState) {

    case 0:
      if (pv > POWER_THRESHOLD) {
        nextState = 1;
        pwm = 0;
        miliseconds = millis();
      }
      break;

    case 1:
      if (millis() < miliseconds + MINTIME) {
        if (pv < POWER_THRESHOLD) {
          nextState = 0;
        }
        break;
      }
      if (millis() > miliseconds + TIMEOUT) {
        nextState = 0;
      }
      if (pv < POWER_THRESHOLD) {
        nextState = 2;
        miliseconds = millis();
      }

      break;

    case 2:
      if (millis() < miliseconds + MINTIME) {
        if (pv > POWER_THRESHOLD) {
          nextState = 0;
        }
        break;
      }
      if (millis() > miliseconds + TIMEOUT) {
        nextState = 0;
      }
      if (pv > POWER_THRESHOLD) {
        nextState = 3;
        miliseconds = millis();
      }
      break;

    case 3:
      if (millis() < miliseconds + MINTIME) {
        if (pv < POWER_THRESHOLD) {
          nextState = 0;
        }
        break;
      }
      if (millis() > miliseconds + TIMEOUT) {
        nextState = 0;
      }
      if (pv < POWER_THRESHOLD) {
        nextState = 5;
        beat();
        pwm = 180;
      }
      break;

    case 4:
      if (pv > POWER_THRESHOLD) {
        nextState = 5;
      }
      break;

    case 5:
      if (pv > POWER_THRESHOLD) {
        newpwm = (3.175 * index / SAMPLES - 0.587) * 200 + 20;
        pwm = 0.6 * newpwm + 0.4 * pwm;
        if (pwm > 255) {
          pwm = 255;
        }
      }
      else {
        nextState = 6;
      }
      break;

    case 6:
      if (pv > POWER_THRESHOLD) {
        nextState = 7;
        miliseconds = millis();
      }
      break;

    case 7:
      if (millis() < miliseconds + MINTIME) {
        if (pv < POWER_THRESHOLD) {
          nextState = 6;
        }
        break;
      }
      if (millis() > miliseconds + (TIMEOUT - MINTIME) ) {
        nextState = 5;
      }
      if (pv < POWER_THRESHOLD) {
        nextState = 8;
        miliseconds = millis();
      }
      break;

    case 8:
      if (millis() < miliseconds + MINTIME) {
        if (pv > POWER_THRESHOLD) {
          nextState = 6;
        }
        break;
      }
      if (millis() > miliseconds + TIMEOUT) {
        nextState = 5;
      }
      if (pv > POWER_THRESHOLD) {
        nextState = 9;
        miliseconds = millis();
      }
      break;

    case 9:
      if (millis() < miliseconds + MINTIME) {
        if (pv < POWER_THRESHOLD) {
          nextState = 6;
        }
        break;
      }
      if (millis() > miliseconds + TIMEOUT) {
        nextState = 5;
      }
      if (pv < POWER_THRESHOLD) {
        nextState = 0;
        beat();
        pwm = 0;
      }
      break;
  }

#ifdef DEBUG

  Serial.print("CurrentState = ");
  Serial.println(curState);

  Serial.print("Max freq = ");
  Serial.print(index * 1.0 * SAMPLING_FREQUENCY / SAMPLES);
  Serial.print("\t");
  Serial.println(pv);

  Serial.print("pwm = ");
  Serial.println(pwm);
  

  
#endif

  analogWrite(9, pwm);
  curState = nextState;

}

void beat() {
  //analogWrite(9, 0);
  delay(200);
  analogWrite(9, 220);
  delay(200);
  analogWrite(9, 0);
  delay(200);
  analogWrite(9, 220);
  delay(700);
  //analogWrite(9,0);
  //delay(500);
}

