#ifndef CYCLE_DATA_H
#define CYCLE_DATA_H

#include "constants.h"
#include "libs_header.h"

struct cycle_data_vars {
    uint16_t t_accel;
    uint8_t v_const;
    uint16_t t_const;
    uint16_t t_slowdown;
    uint16_t t_pause;
    uint8_t num_cycles;
    uint8_t flags;
};

class cycle_data {
private:
    uint16_t start_byte;

public:
    cycle_data(uint16_t _start_byte);

    uint16_t t_accel;
    uint8_t v_const;
    uint16_t t_const;
    uint16_t t_slowdown;
    uint16_t t_pause;
    uint8_t num_cycles;
    bool is_accel_smooth;
    bool is_bidirectional;

    void loadDataFromMem();
    void writeDataToMem();
    uint8_t sizeOfCycleData();
};

extern cycle_data CYCLE_DATA;

#endif