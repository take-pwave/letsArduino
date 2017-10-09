#ifndef  __KEYBOARD_H__
#define  __KEYBOARD_H__

#define  ROWS  4
#define  COLS  5
class KeyBoard {
public:
  int getKey() {
    char key = keypad.getKey();
    if (key != NO_KEY) {
      code = -1;
      if ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'F')) 
        code = key <= '9' ? key - '0' : key - 'A' + 10;
      lastkey = key;
    }
      
    return key;
  }
  
  boolean isPressed() {
    return keypad.getState() == HOLD;
  }
  
  int  getCode() {
    return code;
  }
  
  int  lastKey() {
    return lastkey;
  }
  
private:
  char  lastkey = NO_KEY;
  byte  code = -1;
  char keys[ROWS][COLS] = {
    { 'C', 'D', 'E', 'F', 'S'},
    { '8', '9', 'A', 'B', 'I'},
    { '4', '5', '6', '7', 'R'},
    { '0', '1', '2', '3', 'T'}
  };
  byte rowPins[ROWS] = {11, 10, 9, 8};
  byte colPins[COLS] = {5, 4, 3, 2, 1};
  Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
};

#endif
