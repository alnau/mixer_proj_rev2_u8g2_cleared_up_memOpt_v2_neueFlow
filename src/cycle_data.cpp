#include "cycle_data.h"

cycle_data::cycle_data(uint16_t _start_byte) {
    start_byte = _start_byte;
}

void cycle_data::loadDataFromMem() {
    cycle_data_vars data;
    eeprom_read_block((void *)&data, (void *)start_byte, sizeof(data));

    //uint8_t temp_word = 0;
    // t_accel = eeprom_read_word((uint16_t *)start_byte);
    // v_const = eeprom_read_word((uint16_t *)(start_byte + SIZE_OF_DATA));
    // t_const = eeprom_read_word((uint16_t *)(start_byte + 2 * SIZE_OF_DATA));
    // t_slowdown = eeprom_read_word((uint16_t *)(start_byte + 3 * SIZE_OF_DATA));
    // t_pause = eeprom_read_word((uint16_t *)(start_byte + 4 * SIZE_OF_DATA));
    // num_cycles = eeprom_read_word((uint16_t *)(start_byte + 5 * SIZE_OF_DATA));
    // temp_word = eeprom_read_byte((uint8_t *)(start_byte + 6 * SIZE_OF_DATA));

    t_accel = data.t_accel;
    v_const = data.v_const;
    t_const = data.t_const;
    t_slowdown = data.t_slowdown;
    t_pause = data.t_pause;
    num_cycles = data.num_cycles;
    //temp_word = data.flags;

    is_accel_smooth = data.flags & 0b00000001;
    is_bidirectional = (data.flags >> 1) & 0b00000001;

    // is_accel_smooth = temp_word & 1;
    // is_bidirectional = (temp_word >> 1) & 1;
}

void cycle_data::writeDataToMem() {

    // Serial.print(F("Writing to EEPROM... "));


    cycle_data_vars data = {t_accel, v_const, t_const, t_slowdown, t_pause, num_cycles, (is_bidirectional << 1) | is_accel_smooth};
    eeprom_update_block((const void *)&data, (void *)start_byte, sizeof(data));

    // eeprom_update_word((uint16_t *)start_byte, t_accel);
    // eeprom_update_word((uint16_t *)(start_byte + SIZE_OF_DATA), v_const);
    // eeprom_update_word((uint16_t *)(start_byte + 2 * SIZE_OF_DATA), t_const);
    // eeprom_update_word((uint16_t *)(start_byte + 3 * SIZE_OF_DATA), t_slowdown);
    // eeprom_update_word((uint16_t *)(start_byte + 4 * SIZE_OF_DATA), t_pause);
    // eeprom_update_word((uint16_t *)(start_byte + 5 * SIZE_OF_DATA), num_cycles);
    // eeprom_update_byte((uint8_t *)(start_byte + 6 * SIZE_OF_DATA), temp_word);

    // Serial.println(F("Done"));
}

uint8_t cycle_data::sizeOfCycleData() {
    cycle_data_vars data;
    return sizeof(data);
}

cycle_data CYCLE_DATA(START_BYTE);