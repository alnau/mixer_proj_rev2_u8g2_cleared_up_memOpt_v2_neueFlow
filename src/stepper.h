#pragma once

#include "constants.h"
#include "libs_header.h"
#include "global_variables.h"
#include "cycle_data.h"

#define CW 0
#define CCW 1

// Математические константы для упрощения математики во время прерываний
#define ALPHA (2 * PI / SPR)                         // 2*pi/spr
#define A_T_x100 ((long)(ALPHA * T1_FREQ * 100))     // (ALPHA / T1_FREQ)*100
#define T1_FREQ_148 ((int)((T1_FREQ * 0.676) / 100)) // деленный на 100 и умноженный на 0.676 (для уменьшения ошибки, см AVR446)
#define A_SQ (long)(ALPHA * 2 * 10000000000)         // ALPHA*2*10000000000
#define A_x20000 (int)(ALPHA * 20000)                // ALPHA*20000

// Speed ramp states
#define STOP 0
#define ACCEL 1
#define DECEL 2
#define RUN 3
#define PAUSE 4


/**
 * @brief Содержит данные, используемые для рассчета параметров рампы в прерываниях
 *  вызываемых таймером
 * 
 *  Содержит данные используемые прерываниями таймера для расчета профиля рампы
 *  Данные записываются ф-иями startMotor() и move(args), Когда мотор работает (прерывания
 *  таймера вызываются) содержимое читается/модифицируется при рассчете тиков до след шага
 */
typedef struct {
    // Отслеживает статус рампы
    volatile uint8_t run_state;
    // Направление вращение
    volatile bool dir : 1;
    // скорость вращения в 0.01 рад/c^
    volatile uint16_t speed;
    // Число тиков до следующего шага двигателя
    volatile uint32_t step_delay;
    // На каком шаге начинаем торможение
    volatile uint32_t decel_start;
    // Число шагов на торможение
    volatile int32_t decel_val;
    // Минимальное число тиков между шагами (определяется скоростью постоянного вращения)
    volatile uint16_t min_delay;
    // Счетчик, используемый при ускорении/торможении
    volatile int32_t accel_count;
    // Буферизация ускорения для повторного рассчета следующих циклов рампы, а также необходимая при расчете торможения по команде пользователя
    volatile uint16_t accel;
    volatile uint16_t decel;
    // Число циклов. 0 => бесконечное повторение
    volatile uint8_t n_cycles;
    // Флаг режима работы (true -> двустороннее вращение, false -> одностороннее вращение)
    volatile bool is_bidir;
    // Время паузы в секундах
    volatile uint8_t t_pause;
} speedRampData;


extern speedRampData srd;



//void md_LoadDataFromMem(/*могут понадобиться входный параметры, например для случая режима программирования*/);
void startMotor();
//void move(int32_t step, uint16_t accel, uint16_t decel, uint16_t speed, uint16_t t_pause, uint16_t n_cycles, bool is_bidir);
void initTimer1(void);
inline void doTheFStep(bool dir);
void initStepper();
uint32_t m_sqrt(uint32_t x);
void printRampData();
void printRampDataEEPROM();
void restoreRampState();
void saveRampState();
uint32_t stoppingRoutine(uint32_t counter);