#pragma once

#define BAUD_RATE 57600

//пины кнопок
#define FUNC_BTN 12
#define LEFT_BTN 10
#define DOWN_BTN 14
#define UP_BTN 16
#define RIGHT_BTN 22
#define ENTER_BTN 18

#define FUNC U8X8_MSG_GPIO_MENU_HOME
#define ENTER U8X8_MSG_GPIO_MENU_SELECT
#define LEFT U8X8_MSG_GPIO_MENU_PREV
#define RIGHT U8X8_MSG_GPIO_MENU_NEXT
#define UP  U8X8_MSG_GPIO_MENU_UP
#define DOWN U8X8_MSG_GPIO_MENU_DOWN  

//пины дисплея
#define SI 46
#define _SCL 44
#define RS 42
#define RSE 40
#define CS 38

#define SPEED 0
#define MAIN 1
#define CYCLE 2
#define SETTINGS 3
#define PROGRAM_SELECT 4
#define PROGRAM_SETUP 5

//==== Пины мотора ====
// Пин шага. Переход в HIGH делает шаг
#define STEP_PIN 31
// Пин направления вращения
#define DIR_PIN 29
// Пин активирующий мотор. LOW -> вкл.
#define ENA_PIN 27


//==== константы связанные с дисплеем ====
// ширина в пикс
#define SCREEN_WIDTH 128 
// высота в пикс
#define SCREEN_HEIGHT 64 
//ширина столбца с данными (4 символа + 2 пикселя справа для бегунка)
#define DATA_COL_WIDTH 26 
//радиус скругленной рамки
#define FRAME_RADIUS 7    

//==== константы связанные с структурой данных EEPROM ====
//размер данных хранимых в EEPROM в байтах
#define SIZE_OF_DATA (uint8_t)2
// байт, с которого начинает храниться информация о режиме 
#define START_BYTE (uint8_t)10 
// байт, с которого начинает храниться информация о программируемом режиме
#define PROG_FIRST_BYTE (uint16_t)(START_BYTE + 6*SIZE_OF_DATA + 1 + 1)
// Число точек в режиме программирования
#define NUM_OF_POINTS (uint8_t)16
//Число программ
#define NUM_OF_PROGS (uint8_t)8

#define RAMP_FIRST_BYTE (uint16_t)(PROG_FIRST_BYTE + NUM_OF_POINTS * NUM_OF_PROGS * 5 * SIZE_OF_DATA + 1)

//==== константы менюшек ====
//Число элементов главного меню
#define MM_ITEMS 3
// Число эл-тов в режиме программирования
#define PROG_ITEMS 8
// Число эл-тов в режиме ручного запуска
#define SETUP_ITEMS 8
// Число эл-тов в меню настроек
#define SETTINGS_ITEMS 4



// ==== Параметры контроллера шагового мотора ====
//! Число шагов двигателя на один оборот
#define SPR 800
//! Частота прерываний Таймера1
#define T1_FREQ 250000
//! Время торможения при остановке со стороны пользователя. В с
#define TIME_TO_STOP 5
//! Ускорение торможения при остановке по команде пользователя. В 0.01 рад/с^2
//USER_DECEL = 100*RPM*2*pi/60/TIME_TO_STOP
#define USER_DECEL (uint16_t)((100*MAX_RPM)/TIME_TO_STOP/(60/(2*PI))) //9.549 = 60/(2*PI)

#define MAX_TIME 300
#define MAX_RPM 200
#define MAX_NUM MAX_RPM


//частота обновления текста в режиме прокрутки
#define SCROLL_FREQ 1
#define SCROLL_PERIOD 333  
//! Максимальная длина текствой строки меню (используется в режиме прокрутки)
#define TEXT_MAX_LEN (SCREEN_WIDTH - DATA_COL_WIDTH) / 6 - 7
//! Аналогично, но используется в програмном режимережиме 
#define TEXT_MAX_LEN_8 TEXT_MAX_LEN - 2
//! Смещение столбца данных (коррекция)
#define DATA_X_BIAS 7
#define LINE_Y_BIAS -1
//! Время загрузки менюшек в мс
#define LOADING_TIME 1000

#define BUZZER 3
#define BUZZER_PITCH 25

//! Пин потери энергии
#define PWR_LOSS 2 //INT 0


#define MAX_DECCEL 100