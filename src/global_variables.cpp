#include "global_variables.h"

// Инициализация глобальных переменных

//==== Флаги ========
bool emergency_stop = false; // флаг аварийного завершения. Нужно будет переписать на eeprom
volatile bool pwr_loss = false;
volatile bool is_restoring = false;
bool volatile is_working = false;
bool working_in_programming_mode = false;
bool working_in_manual_mode = false;
// bool data_initialised = false;

bool safe_stop = true;
bool volatile need_to_stop = false;
//! Флаг, поднимаемый после начала отработки торможения внутри motor_cntr.c.
bool volatile is_stopping = false;
bool need_sound = true;

// bool need_refresh_speed_menu = false;
// bool need_refresh_mm = false;
// bool need_refresh_prog_page = false;
// bool need_refresh_settings = false;
bool need_to_load_interface = false;
bool refresh_screen = false;
//=================

//===================

volatile uint8_t prog_num = 0;
volatile uint8_t point_num = 0;
uint8_t menu_ptr = 0;

// TODO объявить их внутри несущих функций, если их значения не модифицируются извне
// если модифицируются, то, возможно, следует переписать функции, которые делаяют это
// uint8_t main_menu_ptr = 0;
// uint8_t setup_ptr = 0;

uint16_t prev_scroll_time = 0;
uint16_t prev_scroll_time_8 = 0;

// uint8_t curr_page = 0;
// uint8_t settings_cursor = 0;

const char sett_0[] PROGMEM = "Яркость";
const char sett_1[] PROGMEM = "Звук";
const char sett_2[] PROGMEM = "Плавная остановка";
const char sett_3[] PROGMEM = ""; // Отправить данные
const char sett_4[] PROGMEM = ""; // Получить данные

const char mm_0[] PROGMEM = "Режим";
const char mm_1[] PROGMEM = "Программы";
const char mm_2[] PROGMEM = "Настройки";

const char setup_0[] PROGMEM = "Время ускорения";
const char setup_1[] PROGMEM = "Скорость вращения";
const char setup_2[] PROGMEM = "Время постоянного вращения";
const char setup_3[] PROGMEM = "Время торможения";
const char setup_4[] PROGMEM = "Время паузы";
const char setup_5[] PROGMEM = "Число повторений";
const char setup_6[] PROGMEM = "Ускорение";
const char setup_7[] PROGMEM = "Режим вращения";

const char *const settings_items[] PROGMEM = {sett_0, sett_1, sett_3, sett_4};
const char *const main_menu_items[] PROGMEM = {mm_0, mm_1, mm_2};
const char *const setup_menu_items[] PROGMEM = {setup_0, setup_1, setup_2, setup_3, setup_4, setup_5, setup_6, setup_7};
