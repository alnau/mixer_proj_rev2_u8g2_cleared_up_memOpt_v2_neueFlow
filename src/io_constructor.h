#ifndef IO_CONSTRUCTOR_H
#define IO_CONSTRUCTOR_H

#include "libs_header.h"
#include "constants.h"

//==== Экран ======
#ifdef WOKWI
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, 21,20);
//Для NANO 11 (MOSI), 12 (MISO), 13 (SCK)
#elif __AVR_ATmega328P__
U8G2_ST7565_ERC12864_F_4W_HW_SPI u8g2(U8G2_R0, CS, RS/*, RSEI*/);
#else 
U8G2_ST7565_ERC12864_F_4W_SW_SPI u8g2(U8G2_R0, _SCL, SI, CS, RS, RSE); 

#endif

//U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, 21,20);


Button up(UP_BTN);
Button down(DOWN_BTN);
Button right(RIGHT_BTN);
Button left(LEFT_BTN);
Button enter(ENTER_BTN);
Button func(FUNC_BTN);

#endif