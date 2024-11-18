#ifndef CYCLE_DATA_H
#define CYCLE_DATA_H

#include "constants.h"
#include "libs_header.h"

/**
 * @brief заведем базовую структуру для хранения данных цикла
 * 
 * Упрощает подгрузку/сохранение данных из/в EEPROM. Если вкратце, то это нужно лишь для 
 * работы с данными через eeprom_write_block или eeprom_read_block
 */
struct cycle_data_vars {
    uint16_t t_accel;       // время ускорения (с)
    uint8_t v_const;        // скорость вращения с пост. скоростью (об/мин)
    uint16_t t_const;       // время вращения с постоянной скоростью
    uint16_t t_slowdown;    // время торможения (с)
    uint16_t t_pause;       // время паузы (с)
    uint8_t num_cycles;     // число повторений цикла. num_cycles = 0 означает бесконечное повторение 
    uint8_t flags;          //для хранения булевых значений, определяющих линейную/нелинейную рампу и идет ли вращение в две стороны
};

/**
 * @brief синглтон - прослойка для бесшовной передачи данных рампы между eeprom и динамической паматью 
 * 
 */
class cycle_data {
private:
    uint16_t start_byte;    // Байт, с которого начинают храниться данные цикла в EEPROM (см. документацию)

public:
    cycle_data(uint16_t _start_byte);

    uint16_t t_accel;       // время ускорения (с)
    uint8_t v_const;        // скорость вращения с пост. скоростью (об/мин)
    uint16_t t_const;       // время вращения с постоянной скоростью
    uint16_t t_slowdown;    // время торможения (с)
    uint16_t t_pause;       // время паузы (с)
    uint8_t num_cycles;     // число повторений цикла. num_cycles = 0 означает бесконечное повторение 
    bool is_accel_smooth;
    bool is_bidirectional;

    void loadDataFromMem();
    void writeDataToMem();
    uint8_t sizeOfCycleData();
};

extern cycle_data CYCLE_DATA;

#endif