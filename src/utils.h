

#ifndef UTILS
#define UTILS
// #include "constants.h"
// #include "libs_header.h"

#include <avr/eeprom.h>
#include "u8g2_constructor.h"
#include "constants.h"
#include "libs_header.h"

// возвращает true если eeprom инициализируется впервые
bool check_if_first_init()
{
    uint8_t first_byte_in_EEPROM;
    bool is_it_first_init = false;
    first_byte_in_EEPROM = eeprom_read_byte((uint8_t *)0);
    // на случай если заводские данные отличаются от 255 в каждом байте
    if (first_byte_in_EEPROM != 255)
    {
        is_it_first_init = first_byte_in_EEPROM & 0b00000001;

        return !is_it_first_init;
    }
    else
        return true;
}

// проверяет не нажал ли пользователь на кнопку вверх/вниз и если да, то возвращает
// новое значение указателя, а если нет, то старое. Указатель буферизируется и не меняется
// внутри функции
//  НЕ РЕНДЕРЕР
uint8_t upDown(uint8_t ptr, uint16_t items)
{
    uint8_t tmp = ptr;

    if (u8g2.getMenuEvent() == UP)
    {
        if (ptr - 1 < 0)
        {
            u8g2.drawHLine(10, 0, SCREEN_WIDTH - 20); // если упремся в потолок меню, рисуем полоску сверху
            u8g2.updateDisplay();
            delay(100);
            u8g2.setDrawColor(0);
            u8g2.drawBox(10, 0, SCREEN_WIDTH - 20, 1); // и замажем ее
            u8g2.setDrawColor(1);
            u8g2.updateDisplay();
        }
        tmp = constrain((uint8_t)(ptr - 1), 0, (uint8_t)(items - 1));
    }
    else if (u8g2.getMenuEvent() == DOWN)
    {
        if (ptr + 1 > (uint8_t)(items - 1))
        {
            u8g2.drawHLine(10, SCREEN_HEIGHT - 1, SCREEN_WIDTH - 20); // аналогичная прямая
            u8g2.updateDisplay();
            delay(100);
            u8g2.setDrawColor(0);
            u8g2.drawBox(10, SCREEN_HEIGHT - 2, SCREEN_WIDTH - 15, 4); // и ее удаление
            u8g2.setDrawColor(1);
            u8g2.updateDisplay();
        }
        // need_refresh = true;
        tmp = constrain((uint8_t)(ptr + 1), 0, (uint8_t)(items - 1));
    }
    return tmp;
}

// аналогично up_down толька выводит перескакивание по порядкам в трехзначных числах
//  TODO: ПРАКТИЧЕСКИ АНАЛОГИЧНА ДВУМ ДРУГИМ. ОПТИМИЗИРОВАТЬ
uint8_t leftRight(uint8_t digit)
{

    uint8_t tmp = digit;

    if (u8g2.getMenuEvent() == LEFT)
    {
        tmp = constrain(digit - 1, 1, 3);
    }
    else if (u8g2.getMenuEvent() == RIGHT)
    {
        tmp = constrain(digit + 1, 1, 3);
    }
    return tmp;
}

// TODO: ПРАКТИЧЕСКИ АНАЛОГИЧНА ДВУМ ДРУГИМ. ОПТИМИЗИРОВАТЬ
uint8_t leftRight(uint8_t curr_pos, uint8_t num_items)
{
    uint8_t tmp = curr_pos;

    if (u8g2.getMenuEvent() == LEFT)
    {
        tmp = constrain(curr_pos - 1, 1, num_items - 1);
    }
    else if (u8g2.getMenuEvent() == RIGHT)
    {
        tmp = constrain(curr_pos + 1, 1, num_items - 1);
    }
    return tmp;
}

// замена string::substring
// по идее, это позволит перевести названия менюшек на char* и закинуть их в PROGMEM (см выше) и высвободит 96 байт (минимум. Класс стринг тоже тяжелый)
/*
  //  Использование:
  char* mySub = substring(originalString, start, end);

  if (mySubstring != nullptr) {
      Serial.println(mySubstring);
      delete[] mySubstring; // Don't forget to free the memory
  }
*/
char *substring(const char *str, size_t start, size_t end)
{
    if (str == nullptr || start > end || start >= strlen(str))
    {
        return nullptr;
    }

    size_t substr_length = end - start;
    char *substr = new char[substr_length + 1]; // +1 for the null terminator

    // Use memcpy to copy the substring into the new variable
    memcpy(substr, str + start, substr_length);

    // Don't forget to null-terminate the new string
    substr[substr_length] = '\0';
    return substr;
}

// рассчет порядка числа
uint8_t calculateOrder(uint16_t number)
{
    uint8_t order = 0;
    if (number == 0)
        return 1;
    else
    {
        uint16_t tmp = number;
        while (tmp != 0)
        {
            tmp = tmp / 10;
            order++;
        }
        return order;
    }
}

// TODO: ПРАКТИЧЕСКИ АНАЛОГИЧНА ДВУМ ДРУГИМ. ОПТИМИЗИРОВАТЬ
uint8_t leftRightProg(uint8_t curr_pos, uint8_t num_items)
{
    uint8_t tmp = curr_pos;

    if (u8g2.getMenuEvent() == LEFT)
    {
        tmp = constrain(curr_pos - 1, 0, num_items - 1);
    }
    else if (u8g2.getMenuEvent() == RIGHT)
    {
        tmp = constrain(curr_pos + 1, 0, num_items - 1);
    }
    return tmp;
}

uint8_t upDownProg(uint8_t ptr, uint16_t items)
{
    uint8_t tmp = ptr;

    if (u8g2.getMenuEvent() == UP)
    {
        tmp = constrain((uint8_t)(ptr - 1), 0, (uint8_t)(items - 1));
    }
    else if (u8g2.getMenuEvent() == DOWN)
    {
        tmp = constrain((uint8_t)(ptr + 1), 0, (uint8_t)(items - 1));
    }
    return tmp;
}

uint8_t digitToX(uint8_t digit)
{
    if (digit == 1)
        return SCREEN_WIDTH - 4 * 6;
    else if (digit == 2)
        return SCREEN_WIDTH - 2 * 6;
    else if (digit == 3)
        return SCREEN_WIDTH - 1 * 6;
    else
        return 0;
}

#endif
