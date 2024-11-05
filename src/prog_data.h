#ifndef PROG_DATA_H
#define PROG_DATA_H

#include "libs_header.h"
#include "constants.h"
// #include "utils.h"

class prog_data {

	/* ! \brief: позволяет считывать и обрабатывать данные программ, занесенных в EEPROM
	 * при первом запуске инициализирует значения всех параметров 16 программ =100
	 * Единовременно работа проводится только с буфером всего одной программы и
	 * заполнение буффера происходит посредством вызова функции load_data_to_buff
	 * в случае, если произошли изменения во внешних переменных, то буфер можно обновить
	 * используя update_buff. Как только пользователь выйдет из меню, необходимо
	 * вызвать функцию update_mem, которая запишет новые данные в EEPROM
	 */

private:
	uint16_t _start_byte;
	int16_t _prog_info[5]; // данные отдельно взятой точки из программы
	//_prog_info[0] - скорость вращения с постоянной скоростью
	//_prog_info[1] - время ускорения
	//_prog_info[2] - время вращения с постоянной скоростью
	//_prog_info[3] - время замедления
	//_prog_info[4] - время паузы

	// uint16_t required_speed; //скорость на текущей точке
	// uint16_t time_to_accelerate;  //время за которое должно произойти ускорение от предыдущей до текущей точки

	uint16_t _address_of_point(uint8_t prog_num, uint8_t point_num); // возвращает адресс начала конкретной точки с номером point_num лежащей внутри программы с номером prog_num
	void _init_EEPROM();

public:
	prog_data(uint16_t start_byte);						  // конструктор, инициализирующий все числа как 100
	void update_mem(uint8_t prog_num, uint8_t point_num); // prog_num = 1,2...,16
	void update_buff(int16_t vel, int16_t accel_t, int16_t work_t, int16_t decel_t, int16_t pause_t);
	void load_data_to_buff(uint8_t prog_num, uint8_t point_num);
	void printBuffData();
	int16_t get_vel();
	int16_t get_accel_t();
	int16_t get_work_t();
	int16_t get_decel_t();
	int16_t get_pause_t();
};

extern prog_data PROG_DATA;
#endif