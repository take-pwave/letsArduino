#ifndef  __LED_7SEG_H__
#define  __LED_7SEG_H__

#define  a   (7)
#define  b   (A5)
#define  c   (A4)
#define  d   (A3)
#define  e   (A2)
#define  f   (A1)
#define  g   (A0)

class LED7Seg {
public:  
  LED7Seg() {
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
      digitalWrite(pins[i], ptrn[value][i]); 
    }
  }
  
  void clear() {
    for (int i = 0; i < 7; i++) {
      digitalWrite(pins[i], LOW); 
    }    
  }  

  void setValue(byte val) {
    value = val;
    //draw();
  }
    
private:
  int    sel = 13;
  int    pins[7] = {a, b, c, d, e, f, g};
  byte   value = 0;
  byte   ptrn[16][7] = {
    {1,1,1,1,1,1,0},
    {0,1,1,0,0,0,0},
    {1,1,0,1,1,0,1},
    {1,1,1,1,0,0,1},
    {0,1,1,0,0,1,1},
    {1,0,1,1,0,1,1},
    {1,0,1,1,1,1,1},
    {1,1,1,0,0,0,0},
    {1,1,1,1,1,1,1},
    {1,1,1,0,0,1,1},
    {1,1,1,0,1,1,1},
    {0,0,1,1,1,1,1},
    {1,0,0,1,1,1,0},  
    {0,1,1,1,1,0,1},
    {1,0,0,1,1,1,1},
    {1,0,0,0,1,1,1}
  };
  
};

#endif
