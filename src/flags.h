#ifndef BUTTON_FLAGS_H
#define BUTTON_FLAGS_H

#include "Arduino.h"

class flag {

// TODO: переведи все на static
private:
  uint8_t _flag_content = 0;
public:
  flag() {
    _flag_content = 0;
  }
  bool read(uint8_t idx) {

    return _flag_content & (1<<idx);
  }
  void set(uint8_t idx) {

    _flag_content |= 1 << idx;
  }
  void reset(uint8_t idx) {
    _flag_content &= ~(1 << idx);
  }
  void flip(uint8_t idx) {

    read(idx) ? reset(idx) : set(idx);
  }
  void write(uint8_t idx, uint8_t state) {

    bitWrite(_flag_content, idx, state);
  }
  void resetAll() {
    _flag_content = 0;
  }
};
#endif