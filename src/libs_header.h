#ifndef LIBS_HEADER_H
#define LIBS_HEADER_H


#include <U8g2lib.h>
//#include "tiny_button.h"
#include <EncButton.h>

#include <Arduino.h>
#include <avr/eeprom.h>

//#define IS_DEBUG

#ifdef IS_DEBUG
  #define debug(x) Serial.print(x)
  #define debugln(x)  Serial.println(x)
#else
  #define debug(x)
  #define debugln(x)
#endif

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

// #include <avr/io.h>
// #include <avr/interrupt.h>
// #include <avr/eeprom.h>
#endif