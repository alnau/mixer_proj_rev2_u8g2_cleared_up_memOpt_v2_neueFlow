#ifndef BUTTON_FLAGS_H
#define BUTTON_FLAGS_H

#include "Arduino.h"

class flag {

private:
  uint8_t _flag_content = 0;
public:
  flag() {
    _flag_content = 0;
  }
  bool read(uint8_t idx) {
    #if idx>7
    #error "Invalid bit number"
    #endif
    return _flag_content & (1<<idx);
  }
  void set(uint8_t idx) {
    #if idx>7
    #error "Invalid bit number"
    #endif
    _flag_content |= 1 << idx;
  }
  void reset(uint8_t idx) {
    #if idx>7
    #error "Invalid bit number"
    #endif
    _flag_content &= ~(1 << idx);
  }
  void flip(uint8_t idx) {
    #if idx>7
    #error "Invalid bit number"
    #endif
    read(idx) ? reset(idx) : set(idx);
  }
  void write(uint8_t idx, uint8_t state) {
    #if idx>7
    #error "Invalid bit number"
    #endif
    bitWrite(_flag_content, idx, state);
  }
  void resetAll() {
    _flag_content = 0;
  }
};
#endif