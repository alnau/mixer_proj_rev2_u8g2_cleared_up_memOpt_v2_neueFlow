#include "prog_data.h"

// конструктор, инициализирующий все числа как 100
prog_data::prog_data(uint16_t start_byte) {
	_start_byte = start_byte;
	_init_EEPROM();
}

// возвращает адресс начала конкретной точки с номером point_num лежащей внутри программы с номером prog_num
uint16_t prog_data::_address_of_point(uint8_t prog_num, uint8_t point_num) {
	return _start_byte + (5 * prog_num * NUM_OF_POINTS + 5 * point_num) * SIZE_OF_DATA;
}

// проверяет является ли этот запуск первым
bool _check_if_first_init() {
	uint8_t first_byte_in_EEPROM; // первый байт EEPROM
	bool is_it_first_init = false;
	first_byte_in_EEPROM = eeprom_read_byte((uint8_t *)0x00); // считали
	// на случай если заводские данные отличаются от 255 в каждом байте
	if (first_byte_in_EEPROM != 255) {
		// если он не равен 0b11111111 (стандартное значение с фабрики), то это почти наверняка означает что прибор уже был инициализирован
		is_it_first_init = first_byte_in_EEPROM & 0b00000001; // на всякий случай проверим, что в байт INIT записана единица
		return !is_it_first_init;
	}
	else
		return true; // если в байте 0x00 записано 255 = 0b11111111 то это первая инициализация
}

// инициализация памяти, связанной с програмными данными
void prog_data::_init_EEPROM() {
	if (_check_if_first_init()) {
		// если первый запуск
		for (int k = 0; k < NUM_OF_PROGS; k++) {
			for (int l = 0; l < NUM_OF_POINTS; l++) {
				for (int m = 0; m < 5; m++) {
					int16_t tmp = 100;
					int16_t addr = _start_byte + (5 * k * NUM_OF_POINTS + 5 * l + m) * SIZE_OF_DATA;
					eeprom_write_word((uint16_t *)addr, tmp); // заполним все ячейки стандартным значением 100
				}
			}
		}
		// delay(100);

	debugln(F("EEPROM DATA was initialised"));

	}
}

// запишем буфер с данными в соответствующую программу и точку
// prog_num = 1,2...,16
void prog_data::prog_data::update_mem(uint8_t prog_num, uint8_t point_num) {

	debugln(F("Writing to EEPROM... "));

	int addr = _address_of_point(prog_num, point_num); // посчитаем адрес начала хранения данных, связанных с конкретно точкой
	for (int i = 0; i < 5; i++) {
		eeprom_update_word((uint16_t *)(addr + i * sizeof(int16_t)), _prog_info[i]); // и запишем слово (если есть изменения) для всех пяти значений буффера
	}
	debugln(F("Done"));
}

// записывает новые значения в буффер (источник данных извне)
void prog_data::update_buff(int16_t vel, int16_t accel_t, int16_t work_t, int16_t decel_t, int16_t pause_t) {
	_prog_info[0] = vel;	 // скорость постоянного вращения
	_prog_info[1] = accel_t; // время ускорения
	_prog_info[2] = work_t;	 // время вращения с постоянной скоростью
	_prog_info[3] = decel_t; // время замедления
	_prog_info[4] = pause_t; // время паузы
}

// подгружает новые данные по точке из EEPROM
void prog_data::load_data_to_buff(uint8_t prog_num, uint8_t point_num) {
	if (prog_num >= 0 && prog_num < NUM_OF_PROGS && point_num >= 0 && point_num < NUM_OF_POINTS) {
		int addr = _address_of_point(prog_num, point_num);
		eeprom_read_block(&_prog_info, (void *)addr, sizeof(_prog_info));
	}
	else
		debugln(F("Error! Check program number and point number"));
}

// выводит текущее содержимое буффера в Serial monitor
void prog_data::printBuffData() {
#ifdef IS_DEBUG
	for (uint8_t i = 0; i < 5; i++)
		debugln(_prog_info[i]);
#endif
}

// НАбор геттеров
int16_t prog_data::get_vel() {
	return _prog_info[0];
}
int16_t prog_data::get_accel_t() {
	return _prog_info[1];
}
int16_t prog_data::get_work_t() {
	return _prog_info[2];
}
int16_t prog_data::get_decel_t() {
	return _prog_info[3];
}
int16_t prog_data::get_pause_t() {
	return _prog_info[4];
}

prog_data PROG_DATA(PROG_FIRST_BYTE);
	