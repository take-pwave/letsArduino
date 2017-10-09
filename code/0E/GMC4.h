#ifndef  __GMC4_H__
#define  __GMC4_H__

#include "LEDArray.h"
#include "LED7Seg.h"
#include "KeyBoard.h"

// Exec mode
#define  PROGRAM             0
#define  RUN_LedOffBeepOff   1
#define  RUN_LedOnBeepOff    2
#define  STEP_LedOffBeepOff  5
#define  STEP_LedOnBeepOn    6
#define  RUN_LedOnBeepOn     7

// Instruction set
#define  KA  0x0
#define  AO  0x1
#define  CH  0x2
#define  CY  0x3
#define  AM  0x4
#define  MA  0x5
#define  MP  0x6
#define  MM  0x7
#define  TIA  0x8
#define  AIA  0x9
#define  TIY  0xA
#define  AIY  0xB
#define  CIA  0xC
#define  CIY  0xD
#define  CAL  0xE
#define  JUMP  0xF
#define  _RSTO  0x0
#define  _SETR  0x1
#define  _RSTR  0x2
#define  _INPT  0x3
#define  _CMPL  0x4
#define  _CHNG  0x5
#define  _SIFT  0x6
#define  _ENDS  0x7
#define  _ERRS  0x8
#define  _SHTS  0x9
#define  _LONS  0xA
#define  _SOUND 0xB
#define  _TIMR  0xC
#define  _DSPR  0xD
#define  _DEMM  0xE
#define  _DEMP  0xF

// 定義途中

#define  BUZZER    6
#define  FREQ      440
#define  DURATION  40

static byte my_prog[0x70];

class GMC4 {
public:
  GMC4(KeyBoard *keyBoard, LEDArray *ledArray, LED7Seg *led7Seg) {
    this->keyBoard = keyBoard;
    this->ledArray = ledArray;
    this->led7Seg  = led7Seg;
    resetAll();
  }
  
  byte resetAll() {
    pc = &storage[0];
    for (int i = 0; i < 0x60; i++) {
      *pc++ = 0xF;
    }
    return reset();
  }
  
  // 命令を１つ実行する
  byte step() {
    byte key;
    byte code;
    byte tmp;
    char val;
    int  addr;
    int  intval;
    unsigned long  until;
    
    if (*mode != PROGRAM && pc >= prog && pc < mem) {
      // いつでもRsetできるようにする
      key = keyBoard->getKey();  
      if (key == 'T') {   // reset
        *mode = PROGRAM;
        longTone();
        return *mode;
      }
      // 命令を処理する
      switch (*pc++) {
        case KA: 
          // keypadのgetKeyは、キーが押されている間リターンしない
          // 最初にkeyを読み込んでいるのでその値をそのまま使用する。
          // key = keyBoard->getKey();  
          code = keyBoard->getCode();   
          if (key != NO_KEY && code >= 0) {
            if (*mode == RUN_LedOnBeepOn) {
               touchKeyTone();
            }
            *Ar = code;
            *flag = 0;
          }
          else {
            *flag = 1;
          }
          if (key == NO_KEY) {
            if (keyBoard->isPressed()) {
              if (*mode == RUN_LedOnBeepOn) {
                 touchKeyTone();
              }
              *Ar = keyBoard->getCode();
              *flag = 0;
            }
            else
              *flag = 1;
          }        
          break;
        case AO:
          led7Seg->setValue(*Ar);
          *flag = 1;
          break;
        case CH:
          swap(Ar, Br);
          swap(Zr, Yr);
          *flag = 1;
          break;
        case CY:
          swap(Ar, Yr);
          *flag = 1;
          break;
        case AM:
          *(mem + *Yr) = *Ar;
          *flag = 1;
          break;
        case MA:
          *Ar = *(mem + *Yr);
          *flag = 1;
          break;
        case MP:
          *Ar += *(mem + *Yr);
          *flag = *Ar > 0xF ? 1 : 0;
          *Ar = *flag ? *Ar & 0xF : *Ar;
          break;
        case MM:
          val = *(mem + *Yr) - *Ar;
          if (val < 0) {
        	  *Ar = val & 0xF;
        	  *flag = 1;
          }
          else {
        	 *Ar = val;
        	 *flag = 0;
          }
          break;
        case TIA:
          *Ar = *pc++;
          *flag = 1;
          break;
        case AIA:
          *Ar += *pc++;
          *flag = *Ar > 0xF ? 1 : 0;
          *Ar = *flag ? *Ar & 0xF : *Ar;
          break;
        case TIY:
          *Yr = *pc++;
          *flag = 1;
          break;
        case AIY:
          *Yr += *pc++;
          *flag = *Yr > 0xF ? 1 : 0;
          *Yr = *flag ? *Yr & 0xF : *Yr;
          break;
        case CIA:
          *flag = *Ar == *pc++ ? 0 : 1;
          break;
        case CIY:
          *flag = *Yr == *pc++ ? 0 : 1;
          break;
        case CAL:
          switch (*pc++) {
            case _RSTO:
              led7Seg->clear();
              *flag = 1;
              break;
            case _SETR:
              tmp = ledArray->getAddress();
              tmp |= 1 << (*Yr);
              ledArray->setAddress(tmp);
              *flag = 1;
              break;
            case _RSTR:
              tmp = ledArray->getAddress();
              tmp &= ~(1 << (*Yr));
              ledArray->setAddress(tmp);
              *flag = 1;
              break;
            case _CMPL:
              *Ar = (~*Ar) & 0x0F;
              *flag = 1;
              break;
            case _CHNG:
              swap(Ar, ATr);
              swap(Br, BTr);
              swap(Yr, YTr);
              swap(Zr, ZTr);
              *flag = 1;
              break;
            case _SIFT:
              *flag = *Ar % 2 == 0 ? 1 : 0;
              *Ar = *Ar >> 1;
              break;      
            case _ENDS:
              finishTone();
              *flag = 1;
              break;      
            case _ERRS:
              errorTone();
              break;
            case _SHTS:
              touchKeyTone();
              break;
            case _LONS:
              longTone();
              *flag = 1;
              break;
            case _SOUND:
              sound(*Ar);
              *flag = 1;
              break;
            case _TIMR:
              intval = (*Ar + 1) * 100;
              until = millis() + intval;
              while(millis() < until)
                draw();
              *flag = 1;
              break;
            case _DSPR:
              intval = ((int)((storage[0x5F]&0x07)*16)) + ((int)storage[0x5E]&0x0F);
              ledArray->setAddress(intval&0x7F);
              *flag = 1;
              break;
            case _DEMM:
              val = ((char)*(mem + *Yr)) - ((char)*Ar);
              if (val < 0) {
            	  val += 10;
            	  *(mem + *Yr) = val;
            	  *Yr -= 1;
            	  *(mem + *Yr) -= 1;
              }
              else {
            	  *(mem + *Yr) = val;
            	  *Yr -= 1;
              }
              *flag = 1;
              break;
            case _DEMP:
              val = *(mem + *Yr) += *Ar;
              if (val >= 10) {
            	*(mem + *Yr) = val % 10;
            	*Yr -= 1;
            	*(mem + *Yr) += val / 10;
              }
              else
                *Yr -= 1;
              *flag = 1;
              break;
           }
           break;
         case JUMP:
          if (*pc == 0xF) { 
            //errorTone();
            *mode = PROGRAM;
            break;
          }
          addr = (*pc++)*16;
          addr += *pc++;
          if (addr > 0x50) {
            errorTone();
            *mode = PROGRAM;       
            break;
          }          
          if (*flag == 1)
            pc = &storage[addr];    
          *flag = 1;      
          break;
        default:
          errorTone();   
          *mode = PROGRAM;       
      }
    }
    return *mode;
  }
  
  int getKey() {
    char key = keyBoard->getKey();
    
    if (key != NO_KEY) {
      touchKeyTone();
    }
    return key;
  }
  
  int getAddr() {
    return pc - prog;
  }

  byte getCode() {
    return keyBoard->getCode();
  }
    
  byte reset() {
    // もしかして、オブジェクトのアドレスが変化するみたい！
    prog = &storage[0];
    mem  = &storage[0x50];
    Ar   = &storage[0x6F];
    ATr  = &storage[0x69];
    Br   = &storage[0x6C];
    BTr  = &storage[0x67];
    Yr   = &storage[0x6E];
    YTr  = &storage[0x68];
    Zr   = &storage[0x6D];
    ZTr  = &storage[0x66];
    flag = &storage[0x61];
    mode = &storage[0x62];
    lastKey = &storage[0x63];
    *flag = 0;   
    *mode = PROGRAM;
    pc   = prog;
    return *pc;
  }
  
  byte addrSet(int addr) {
    pc = &storage[addr];
    return *pc;
  }
  
  byte incr(byte val) {
    *pc++ = val;
    return *pc;
  }

  void setMode(byte newMode) {
    *mode = newMode;
  }
  
  byte getMode() {
    return *mode;
  }
  
  void touchKeyTone() {
    tone(BUZZER, FREQ, DURATION);
  }
  
  void longTone() {
    tone(BUZZER, FREQ, 1000);
  }
  
  void errorTone() {
    tone(BUZZER, FREQ, DURATION);
    delay(100);
    tone(BUZZER, FREQ, DURATION);
    delay(100);
    tone(BUZZER, FREQ, DURATION);
    delay(100);    
  }
  
  void finishTone() {
    tone(BUZZER, scales[2], 80);
    delay(80);
    tone(BUZZER, scales[3], 80);
    delay(80);
    tone(BUZZER, scales[4], 80);
    delay(80);
    tone(BUZZER, scales[5], 80);
    delay(80);
    tone(BUZZER, scales[6], 80);
  }

  void sound(byte scale) {
    tone(BUZZER, scales[scale], DURATION);
  }
  
  void swap(byte *q, byte *p) {
    byte tmp = *p;
    *p = *q;
    *q = tmp;
  }

  void draw() {
    led7Seg->off();
    if (*mode == STEP_LedOnBeepOn || *mode == RUN_LedOnBeepOff)
      ledArray->setAddress(getAddr());
    ledArray->draw();
    ledArray->on();
    delay(1);
    ledArray->off();
    led7Seg->draw();
    led7Seg->on();
  }
  
private:   

  byte *storage = my_prog;
  byte *prog = &storage[0];
  byte *mem  = &storage[0x50];
  byte *Ar   = &storage[0x6F];
  byte *ATr  = &storage[0x69];
  byte *Br   = &storage[0x6C];
  byte *BTr  = &storage[0x67];
  byte *Yr   = &storage[0x6E];
  byte *YTr  = &storage[0x68];
  byte *Zr   = &storage[0x6D];
  byte *ZTr  = &storage[0x66];
  byte *pc   = prog;
  byte *flag = &storage[0x61];
  byte *mode = &storage[0x62];
  byte *lastKey = &storage[0x63];
  byte scales[14] = {440, 494, 523, 587, 659, 699, 784, 880, 989, 1047, 1175, 1319, 1397, 1568 };  // 4オクターブのラから6オクターブのソまで
  
  KeyBoard  *keyBoard;
  LED7Seg   *led7Seg;
  LEDArray  *ledArray;
};

#endif
