#ifndef  __LED_ARRAY_H__
#define  __LED_ARRAY_H__

class LEDArray {
public: 
  LEDArray() {
    setup();
  }
  
  void setup() {
    pinMode(sel, OUTPUT);
    for (int i = 0; i < 7; i++) {
      pinMode(pins[i], OUTPUT);
    }
  }  
  
  void on() {
    digitalWrite(sel, HIGH);
  }
  
  void off() {
    digitalWrite(sel, LOW);
  }
  
  void draw() {
    for (int i = 0; i < 7; i++) {
      int isOn = (value & 1<<i) != 0 ? HIGH : LOW;
      digitalWrite(pins[i], isOn);
    }
  }  
  
  void setAddress(byte addr) {
    value = addr;
    //draw();
  }
  
  byte getAddress() {
    return value;
  }

private:
  int    sel = 12;
  int    pins[7] = {A0, A1, 7, A5, A4, A2, A3};
  byte   value = 0;
};

#endif

