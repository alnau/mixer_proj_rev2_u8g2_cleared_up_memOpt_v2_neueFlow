#ifndef PROG_UTILS_H
#define PROG_UTILS_H

#include "prog_data.h"
#include "global_variables.h"
#include "constants.h"
#include "libs_header.h"
#include "utils.h"
#include "io_constructor.h"


// TODO: ПРАКТИЧЕСКИ АНАЛОГИЧНА ДВУМ ДРУГИМ. ОПТИМИЗИРОВАТЬ
uint8_t leftRightProg(uint8_t curr_pos, uint8_t num_items)
{
    uint8_t tmp = curr_pos;

    left.tick();
    right.tick();

    if (left.isClicked())   //if (u8g2.getMenuEvent() == LEFT)
    {
        tmp = constrain(curr_pos - 1, 0, num_items - 1);
    }
    else if (right.isClicked()) // if (u8g2.getMenuEvent() == RIGHT)
    {
        tmp = constrain(curr_pos + 1, 0, num_items - 1);
    }
    return tmp;
}

uint8_t upDownProg(uint8_t ptr, uint16_t items)
{
    uint8_t tmp = ptr;

    up.tick();
    down.tick();

    if (up.isClicked())//if (u8g2.getMenuEvent() == UP)
    {
        tmp = constrain((uint8_t)(ptr - 1), 0, (uint8_t)(items - 1));
    }
    else if (down.isClicked())  //else if (u8g2.getMenuEvent() == DOWN)
    {
        tmp = constrain((uint8_t)(ptr + 1), 0, (uint8_t)(items - 1));
    }
    return tmp;
}

uint8_t digitToX(uint8_t digit)
{

    // -24 = 1
    // -12 = 2
    //  -6 = 3 
    const uint8_t LUT[3] = {SCREEN_WIDTH - 4 * 6 , SCREEN_WIDTH - 2 * 6, SCREEN_WIDTH - 1 * 6}; 
    return LUT[digit - 1];

    // if (digit == 1)
    //     return SCREEN_WIDTH - 4 * 6;
    // else if (digit == 2)
    //     return SCREEN_WIDTH - 2 * 6;
    // else if (digit == 3)
    //     return SCREEN_WIDTH - 1 * 6;
    // else
    //     return 0;
    
}


// альтеренативная функция для меню программирования
uint8_t scrollText_8(char *text, uint8_t cursor, uint8_t counter)
{

	uint8_t len = (strlen(text)) / 2 -1;
	if (len >= TEXT_MAX_LEN_8)
	{
		if ((uint16_t)millis() - prev_scroll_time_8 >= 1000 / SCROLL_FREQ)
		{
			// char* tmp[strlen(text)+1];

			uint8_t i = counter % len;
			u8g2.setDrawColor(0);
			u8g2.drawBox(8, (cursor - 1 + 3) * 8, SCREEN_WIDTH - DATA_COL_WIDTH - DATA_X_BIAS - 8, 7);
			u8g2.setDrawColor(1);
			u8g2.setCursor(8, (cursor - 1 + 3) * 8 + 8);

			char* tmp_substr = substring(text, i, i + TEXT_MAX_LEN_8);
			u8g2.print(tmp_substr);
			delete[] tmp_substr;

			// delay(1000 / SCROLL_FREQ);
			prev_scroll_time_8 = (uint16_t)millis();
			u8g2.updateDisplay();
			return counter + 1;
		}
		else
			return counter;
	}
	else if (len < TEXT_MAX_LEN_8)
	{
		u8g2.setDrawColor(0);
		u8g2.drawBox(8, (cursor - 1 + 3) * 8, SCREEN_WIDTH - DATA_COL_WIDTH - DATA_X_BIAS - 8, 7);
		u8g2.setDrawColor(1);
		u8g2.setCursor(8, (cursor - 1 + 3) * 8 + 8);
		u8g2.print(text);
	}
}

// отрисовка курсора в менющке программирования
//  РЕНДЕРЕР
void printProgPtr(uint8_t ptr)
{
	u8g2.setDrawColor(0);
	u8g2.drawBox(0, 24, 4, SCREEN_HEIGHT - 24);
	u8g2.drawBox(78 + 12, 4 + 8, 4 * 6, 1);
	u8g2.setDrawColor(1);
	if (ptr == 0)
	{
		u8g2.drawHLine(78 + 12, 4 + 8, 4 * 6);
	}
	else
	{
		u8g2.setCursor(0, ptr + 3 - 1 + 8);
		u8g2.print(F(">"));
	}
}

void printProgNumbers(int16_t data, uint8_t ptr, bool is_setup, uint8_t digit)
{
	bool is_negative = (data < 0) ? true : false;
	uint16_t abs_data = abs(data);
	uint8_t order = calculateOrder(data);
	u8g2.setDrawColor(0);
	u8g2.drawBox(SCREEN_WIDTH - 3 * 6 - DATA_X_BIAS, (ptr + 1) * 8 + LINE_Y_BIAS, 18 + DATA_X_BIAS, 1);
	u8g2.setDrawColor(1);
	if (is_negative)
	{
		u8g2.setCursor(SCREEN_WIDTH - 4 * 6 - DATA_X_BIAS, ptr * 8 + 8);
		u8g2.print(F("-"));
	}
	else
	{
		u8g2.setCursor(SCREEN_WIDTH - 3 * 6 - DATA_X_BIAS, ptr * 8 + 8);
	}
	if (order < 3)
	{
		for (int i = 1; i <= 3 - order; i++)
			u8g2.print(F("0"));
	}
	u8g2.print(abs_data);
	if (is_setup)
	{
		u8g2.drawHLine(SCREEN_WIDTH - (3 - digit + 1) * 6 - DATA_X_BIAS, (ptr + 1) * 8 + LINE_Y_BIAS, 4);
	}
}

uint16_t setupProgNumbers(int16_t data, uint8_t ptr)
{
	uint8_t digit = 1;
	int16_t tmp = data;
	uint8_t tmp_digit = digit;
	uint8_t order = calculateOrder(data);

	printProgNumbers(tmp, ptr, true, digit);
	u8g2.updateDisplay();

	while (1)
	{
		enter.tick();
		if (enter.isClicked()) //if (u8g2.getMenuEvent() == ENTER)
		{
			printProgNumbers(tmp, ptr, false, ptr);
			u8g2.updateDisplay();
			data = tmp;
			return tmp;
		}

		tmp_digit = digit;
		digit = leftRight(digit);
		if (tmp_digit != digit)
		{
			printProgNumbers(tmp, ptr, true, digit);
			u8g2.updateDisplay();
		}

		printProgNumbers(tmp, ptr, true, digit);
		up.tick();
		down.tick();
		if (up.isClicked()) //if (u8g2.getMenuEvent() == UP)
		{
			// если сотни
			if (digit == 1)
			{
				tmp = constrain(tmp + 100, -999, 999);
			}
			// десятки десятки
			if (digit == 2)
			{
				tmp = constrain(tmp + 10, -999, 999);
			}
			// и единицы
			if (digit == 3)
			{
				tmp = constrain(tmp + 1, -999, 999);
			}
			printProgNumbers(tmp, ptr, true, digit);
			u8g2.updateDisplay();
		}
		else if (down.isClicked()) //else if (u8g2.getMenuEvent() == DOWN)
		{
			// если сотни
			if (digit == 1)
			{
				tmp = constrain(tmp - 100, -999, 999);
			}
			if (digit == 2)
			{
				tmp = constrain(tmp - 10, -999, 999);
			}
			if (digit == 3)
			{
				tmp = constrain(tmp - 1, -999, 999);
			}
			printProgNumbers(tmp, ptr, true, digit);
			u8g2.updateDisplay();
		}
	}
}

void printProgTime(uint16_t T, uint8_t ptr, bool is_setup, uint8_t digit)
{
	u8g2.setDrawColor(0);
	u8g2.drawBox(SCREEN_WIDTH - 4 * 6 - DATA_X_BIAS, 8 * (ptr + 1) + LINE_Y_BIAS, 22 + DATA_X_BIAS, 1); // сотрем старые черты, если потребуется
	u8g2.setDrawColor(1);
	u8g2.setCursor(SCREEN_WIDTH - 4 * 6 - DATA_X_BIAS, 8 * ptr + 8);
	u8g2.print(T / 60);
	u8g2.print(F(":"));
	u8g2.print((T % 60) / 10);
	u8g2.print((T % 60) % 10);
	if (is_setup)
	{
		u8g2.drawHLine(digitToX(digit) - DATA_X_BIAS, 8 * (ptr + 1) + LINE_Y_BIAS, 4);
	}
}

uint16_t setupProgTime(int16_t T, uint8_t ptr)
{
	/// const uint16_t MAX_TIME = 599;
	uint8_t digit = 1;
	uint8_t tmp_digit = digit;

	uint16_t tmp = T;
	printProgTime(tmp, ptr, true, digit);
	u8g2.updateDisplay();

	while (1)
	{
		enter.tick();
		if (enter.isClicked())	//if (u8g2.getMenuEvent() == ENTER)
		{
			printProgTime(tmp, ptr, false, 0);
			u8g2.updateDisplay();
			return tmp;
		}
		tmp_digit = digit;
		digit = leftRight(digit);
		if (digit != tmp_digit)
		{
			printProgTime(tmp, ptr, true, digit);
			u8g2.updateDisplay();
		}

		up.tick();
		down.tick();
		
		if (up.isClicked())	//if (u8g2.getMenuEvent() == UP)
		{

			// если минуты
			if (digit == 1)
			{
				tmp = constrain(tmp + 60, 0, MAX_TIME);
			}
			// десятки секунд
			if (digit == 2)
			{
				tmp = constrain(tmp + 10, 0, MAX_TIME);
			}
			// и секунды
			if (digit == 3)
			{
				tmp = constrain(tmp + 1, 0, MAX_TIME);
			}
			printProgTime(tmp, ptr, true, digit);
			u8g2.updateDisplay();
		}
		else if (down.isClicked())	//else if (u8g2.getMenuEvent() == DOWN)
		{
			// printTime(tmp, ptr, true, digit);
			// если минуты
			if (digit == 1)
			{
				tmp = constrain(tmp - 60, 0, MAX_TIME);
			}
			if (digit == 2)
			{
				tmp = constrain(tmp - 10, 0, MAX_TIME);
			}
			if (digit == 3)
			{
				tmp = constrain(tmp - 1, 0, MAX_TIME);
			}
			printProgTime(tmp, ptr, true, digit);
			u8g2.updateDisplay();
		}
	}
}

void printProgrammingPages(uint8_t page_n)
{
	for (int i = 0; i < 4; i++)
	{
		u8g2.setCursor(8, 4 + 16 * i + 8);
		u8g2.print(F("Программа "));
		u8g2.print(page_n * 4 + i + 1);
	}
}

void printProgrammingHeader(uint8_t program_ptr, uint8_t curr_point_num)
{
	u8g2.clear();
	u8g2.setCursor(0, 8);
	u8g2.print(F("Программа: "));
	u8g2.print(program_ptr + 1);
	u8g2.print(F("\n\rТочка: "));
	u8g2.print(curr_point_num + 1);
	u8g2.print(F("/"));
	u8g2.print(NUM_OF_POINTS);
	u8g2.drawHLine(0, 18, SCREEN_WIDTH - 1);
	u8g2.drawVLine(75, 0, 18);

	u8g2.setCursor(78 + 12, 4 + 8);
	if (!is_working)
		u8g2.print(F("Пуск"));
	else
		u8g2.print(F("Стоп"));
}

void printProgrammingSetupItems()
{
	for (int i = 0; i < 5; i++)
	{
		u8g2.setCursor(8, 3 + i + 8);

		PGM_P pstr = pgm_read_word(setup_menu_items + i);
		char buffer[strlen_P(pstr) + 1];
		strcpy_P(buffer, pstr);

		char* tmp_substr = substring(buffer, i, i + TEXT_MAX_LEN_8);
		u8g2.print(tmp_substr);
		delete[] tmp_substr;
	}
}
void refreshProgrammingSetupItems(uint8_t num)
{
	u8g2.setCursor(8, 3 + num - 1 + 8);

	PGM_P pstr = pgm_read_word(setup_menu_items + num - 1);
	char buffer[strlen_P(pstr) + 1];
	strcpy_P(buffer, pstr);

	char* tmp_substr = substring(buffer, i, i + TEXT_MAX_LEN_8);
	u8g2.print(tmp_substr);
	delete[] tmp_substr;
}

void printProgrammingData(int16_t p_vel, int16_t p_t_accel, int16_t p_t_work, int16_t p_t_decel, int16_t p_t_pause)
{
	printProgTime(p_t_accel, 3, false, 0);
	printProgNumbers(p_vel, 4, false, 0);
	printProgTime(p_t_work, 5, false, 0);
	printProgTime(p_t_decel, 6, false, 0);
	printProgTime(p_t_pause, 7, false, 0);
}

// если двигатель был остановлен, то он спросит нужно ли запустить его и выставит бул is_working и working_in_programming_mode
// также в случае успеха обновляет данные EEPROM
bool start_program(uint8_t prog_ptr, uint8_t curr_point_num)
{
	// фан факт: функция setCursorXY добавляет текста неебаться большую черную рамку. Полчаса проебано
	//  TODO если выживет, то перевести на userInterfaceMessage
	bool yes_is_selected = 1;
	u8g2.setDrawColor(0);
	u8g2.drawBox(16, 12, SCREEN_WIDTH - 32, SCREEN_HEIGHT - 26);
	u8g2.setDrawColor(1);
	u8g2.setCursor(16 + 6, 24);
	if (!is_working)
		u8g2.print(F("Запустить?"));
	else
		u8g2.print(F("Остановить?"));
	u8g2.setCursor(16 + 6 + 6, 40);
	u8g2.print(F("да"));
	u8g2.setCursor(16 + 6 + 6, 48);
	u8g2.print(F("нет"));
	u8g2.drawFrame(16, 12, SCREEN_WIDTH - 32, SCREEN_HEIGHT - 26);
	u8g2.setCursor(16 + 6, 40);
	u8g2.print(F(">"));
	u8g2.updateDisplay();

	while (1)
	{
		enter.tick();
		if (enter.isClicked()) //if (u8g2.getMenuEvent() == ENTER)
		{
			// очистить экран
			need_refresh_prog_page = true;
			if (!is_working)
			{
				if (yes_is_selected)
				{
					prog_num = prog_ptr;
					// need_recalc_accel = true;
					// PROG_DATA.printBuffData();
					working_in_programming_mode = true;
					// fill_array_of_counters();
				}
				return yes_is_selected; // если двигатель покоился, то возвращаем текущий статус
			}
			else
			{
				if (yes_is_selected)
				{
					working_in_programming_mode = false;
				}
				return !yes_is_selected; // если пользователь думает остановить двигатель, то инвертируем выбор
			}
		}

		up.tick();
		down.tick();
		if (down.isClicked() or down.isClicked())	//if ((u8g2.getMenuEvent() == UP) or (u8g2.getMenuEvent() == DOWN))
		{

			yes_is_selected = !yes_is_selected; // обновить выбор
			u8g2.setDrawColor(0);
			u8g2.drawBox(16 + 6, 24, 5, 8);
			u8g2.setDrawColor(1);
			if (yes_is_selected)
			{
				// если выбрано да
				u8g2.setCursor(16 + 6, 40);
				u8g2.print(F(">"));
			}
			else
			{
				u8g2.setCursor(16 + 6, 48);
				u8g2.print(F(">"));
			}
			u8g2.updateDisplay();
		}
	}
}

void setupProgram(uint8_t prog_ptr)
{
	uint8_t curr_point_num = point_num;
	uint8_t tmp_point_num = point_num;
	uint8_t item_num = 0;
	uint8_t tmp_num = 0;
	uint8_t counter = 0;
	bool need_update_eeprom = false;
	bool need_refresh_buffer = false;

	PROG_DATA.load_data_to_buff(prog_ptr, curr_point_num); // загрузили данные программы в буффер
	static int16_t p_vel = PROG_DATA.get_vel();
	static int16_t p_t_accel = PROG_DATA.get_accel_t();
	static int16_t p_t_work = PROG_DATA.get_work_t();
	static int16_t p_t_decel = PROG_DATA.get_decel_t();
	static int16_t p_t_pause = PROG_DATA.get_pause_t();

	printProgrammingHeader(prog_ptr, curr_point_num);
	printProgrammingData(p_vel, p_t_accel, p_t_work, p_t_decel, p_t_pause);
	printProgrammingSetupItems();
	printProgPtr(item_num);
	u8g2.updateDisplay();

	while (1)
	{
		func.tick();
		enter.tick();
		
		if (func.isClicked())	//if (u8g2.getMenuEvent() == FUNC)
		{
			u8g2.clear();
			return;
		}
		else if (enter.isClicked())//else if (u8g2.getMenuEvent() == ENTER)
		{
			need_update_eeprom = true;
			switch (item_num)
			{
			case 0:
				// Запуск программы
				PROG_DATA.update_buff(p_vel, p_t_accel, p_t_work, p_t_decel, p_t_pause);
				PROG_DATA.update_mem(prog_ptr, tmp_point_num);
				need_update_eeprom = false;
				is_working = start_program(prog_ptr, curr_point_num);
				break;
			case 1:
				p_t_accel = setupProgTime(p_t_accel, 3);
				break;
			case 2:
				p_vel = setupProgNumbers(p_vel, 4);
				break;
			case 3:
				p_t_work = setupProgTime(p_t_work, 5);
				break;
			case 4:
				p_t_decel = setupProgTime(p_t_decel, 6);
				break;
			case 5:
				p_t_pause = setupProgTime(p_t_pause, 7);
				break;
			}
		}

		// если пользователь пытался включить двигатель, то обновляем менюшку
		if (need_refresh_prog_page)
		{
			printProgrammingHeader(prog_ptr, curr_point_num);
			printProgrammingData(p_vel, p_t_accel, p_t_work, p_t_decel, p_t_pause);
			printProgrammingSetupItems();
			printProgPtr(item_num);
			u8g2.updateDisplay();

			need_refresh_prog_page = false;
		}

		tmp_num = item_num;
		item_num = upDownProg(item_num, 6);
		// если перешли к другому элементу меню
		if (item_num != tmp_num)
		{
			refreshProgrammingSetupItems(tmp_num);
			printProgrammingData(p_vel, p_t_accel, p_t_work, p_t_decel, p_t_pause);
			printProgPtr(item_num);
			counter = 0;
			u8g2.updateDisplay();
		}
		tmp_point_num = curr_point_num;
		curr_point_num = leftRightProg(curr_point_num, NUM_OF_POINTS);
		// если перешли к следующей точке
		if (curr_point_num != tmp_point_num)
		{
			// если вносили изменения, то внесем их в EEPROM
			if (need_update_eeprom)
			{
				PROG_DATA.update_buff(p_vel, p_t_accel, p_t_work, p_t_decel, p_t_pause);
				// PROG_DATA.printBuffData();
				PROG_DATA.update_mem(prog_ptr, tmp_point_num);
				need_update_eeprom = false;
			}
			// загрузим данные новой точки в буффер
			PROG_DATA.load_data_to_buff(prog_ptr, curr_point_num);
			// и обновим переменные для вывода данных
			p_vel = PROG_DATA.get_vel();
			p_t_accel = PROG_DATA.get_accel_t();
			p_t_work = PROG_DATA.get_work_t();
			p_t_decel = PROG_DATA.get_decel_t();
			p_t_pause = PROG_DATA.get_pause_t();

			// item_num = 0;
			counter = 0;
			printProgrammingHeader(prog_ptr, curr_point_num);
			printProgrammingData(p_vel, p_t_accel, p_t_work, p_t_decel, p_t_pause);
			printProgrammingSetupItems();
			printProgPtr(item_num);
			u8g2.updateDisplay();
		}

		if (item_num != 0)
		{
			PGM_P pstr = pgm_read_word(setup_menu_items + item_num - 1);
			char buffer[strlen_P(pstr) + 1];
			strcpy_P(buffer, pstr);

			counter = scrollText_8(buffer, item_num, counter);
		}
	}
}

#endif