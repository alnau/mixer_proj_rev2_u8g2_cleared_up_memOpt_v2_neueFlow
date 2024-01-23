// #pragma once
#ifndef GLOBAL_VARIABLES_H
#define GLOBAL_VARIABLES_H

#include "Arduino.h"

// Объявление глобальных переменных

//==== Флаги ========
extern bool emergency_stop; // флаг аварийного завершения. Нужно будет переписать на eeprom
extern volatile bool pwr_loss;
extern volatile bool is_restoring;
extern volatile bool is_working;
extern bool working_in_programming_mode;
extern bool working_in_manual_mode;
// extern bool data_initialised;

extern bool safe_stop;
extern volatile bool need_to_stop;
//! Флаг, поднимаемый после начала отработки торможения внутри motor_cntr.c.
extern volatile bool is_stopping;
extern bool need_sound;

// extern bool need_refresh_speed_menu;
// extern bool need_refresh_mm;
// extern bool need_refresh_prog_page;
// extern bool need_refresh_settings;
extern bool refresh_screen;
extern bool need_to_load_interface;
//=================

//===================

extern volatile uint8_t prog_num;
extern volatile uint8_t point_num;
extern uint8_t menu_ptr;
// TODO объявить их внутри несущих функций, если их значения не модифицируются извне
// если модифицируются, то, возможно, следует переписать функции, которые делаяют это
// extern uint8_t main_menu_ptr;
// extern uint8_t setup_ptr;

extern uint16_t prev_scroll_time;
extern uint16_t prev_scroll_time_8;

// extern uint8_t curr_page;
// extern uint8_t settings_cursor;

extern const char *const settings_items[] PROGMEM;
extern const char *const main_menu_items[] PROGMEM;
extern const char *const setup_menu_items[] PROGMEM;

#endif
