#pragma once

#include "libs_header.h"

const uint16_t BAUD_RATE = 9600;

//пины кнопок
const uint8_t FUNC_BTN = 12;
const uint8_t LEFT_BTN =10;
const uint8_t DOWN_BTN = 14;
const uint8_t UP_BTN = 16;
const uint8_t RIGHT_BTN = 22;
const uint8_t ENTER_BTN = 18;


//пины дисплея
const uint8_t SI = 46;
const uint8_t _SCL = 44;
const uint8_t RS = 42;
const uint8_t RSE = 40;
const uint8_t CS = 38;

// Перевод с итераторов на state machine
// Функции высших уровней
const uint8_t SPEED = 0;
const uint8_t MAIN = 1;
const uint8_t CYCLE = 2;
const uint8_t PROGRAM_SELECT = 3;
const uint8_t SETTINGS = 4;

const uint8_t PROGRAM_SETUP = 5;

// Настройка цикла перемешивания, литералы, по которым идет переключение в конечном автомате
// установки режима работы двигателя
const uint8_t SETUP_T_ACCEL = 6;
const uint8_t SETUP_V = 7;
const uint8_t SETUP_T_WORK = 8;
const uint8_t SETUP_T_SLOWDOWN = 9;
const uint8_t SETUP_T_PAUSE = 10;
const uint8_t SETUP_N_CYCLES = 11;
const uint8_t SETUP_SMOOTHNESS = 12;
const uint8_t SETUP_DIR = 13;

// Аналогично для статуса настроек
const uint8_t SETUP_SOUND = 14;
const uint8_t SETUP_SAFE_STOP = 15;
const uint8_t SEND_DATA = 16;
const uint8_t RECIEVE_DATA = 17;

//==== Пины мотора ====
// Пин шага. Переход в HIGH делает шаг
#define STEP_PIN 31
// Пин направления вращения
#define DIR_PIN 27
// Пин активирующий мотор. LOW -> вкл.
#define ENA_PIN 29


//==== константы, связанные с дисплеем ====
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
#define SETTINGS_ITEMS 2

// Максимальное количество времени (сек), которое двигатель может находиться 
// в одном состоянии
#define MAX_TIME 599
// Максимальное число оборотов в минуту. //TODO, эта величина исходит из 
#define MAX_RPM 100     
// Мкаксимальное число при установке числовых значений (скорость, число повторений и тд)
#define MAX_NUM 255


// ==== Параметры контроллера шагового мотора ====
//! Число шагов двигателя на один оборот
#define SPR 800
//! Передаточное число: R(вала мешалки)/R(вала мотора)
#define R_REDUCTION 3
//! Частота прерываний Таймера1. 
//! Максимальная скорость определяется как min(T1_FREQ/SPR/R_REDUCTION, MAX_NUM) (но это не точно, 
// в любом случае, это верхняя граница). Для SPR = 800, R_REDUCTION = 3, SPR = 800 получается ок. 104 об/мин.
// это находится в прямой связи с состоянием битов CS10-CS12 в регистре TCCR1B. На текущий момент 
// стоит делитель на 64. Чисто для справки, (в порядке CS12,CS11,CS10):
// 001 - 1
// 010 - 8
// 011 - 64
// 100 - 256
// 101 - 1024 
#define T1_FREQ 250000
//! Время торможения при остановке со стороны пользователя. В с
#define TIME_TO_STOP 5
//! Ускорение торможения при остановке по команде пользователя. В 0.01 рад/с^2
//USER_DECEL = 100*RPM*2*pi/60/TIME_TO_STOP
#define USER_DECEL R_REDUCTION*(uint16_t)((100*MAX_RPM)/TIME_TO_STOP/(60/(2*PI))) 






//частота обновления текста в режиме прокрутки (мс)
#define SCROLL_PERIOD 400  
// Период обновления значения скорости, выводимого на экран (мс)
#define UPDATE_PERIOD 400
//! Максимальная длина текствой строки меню (используется в режиме прокрутки)
#define TEXT_MAX_LEN (SCREEN_WIDTH - DATA_COL_WIDTH) / 6 - 2
//! Аналогично, но используется в програмном режимережиме 
#define TEXT_MAX_LEN_8 TEXT_MAX_LEN - 2
//! Смещение столбца данных (коррекция)
#define DATA_X_BIAS 7
#define LINE_Y_BIAS -1
//! Время загрузки менюшек в мс
#define LOADING_TIME 1000

// Пин пьезоэлемента (пока не используется)
#define BUZZER 3
// Частота сигнала пьезоэлемента (пока не используется)
#define BUZZER_PITCH 25

//! Пин потери энергии (пока не ипользуется)
#define PWR_LOSS_PIN 2 //INT 0

// Максимальное ускорение торможения
//#define MAX_DECCEL 100