

#ifndef UTILS_H
#define UTILS_H
// #include "constants.h"
// #include "libs_header.h"

#include "global_variables.h"

#include <avr/eeprom.h>
#include "io_constructor.h"
#include "constants.h"
#include "libs_header.h"



// возвращает true если eeprom инициализируется впервые
bool check_if_first_init() {
    uint8_t first_byte_in_EEPROM;
    //bool is_it_first_init = false;
    first_byte_in_EEPROM = eeprom_read_byte((uint8_t *)0);
    // на случай если заводские данные отличаются от 255 в каждом байте
    if (first_byte_in_EEPROM != 255) {
        return false;
        // is_it_first_init = first_byte_in_EEPROM & 0b00000001;

        // return !is_it_first_init;
    }
    else {
        return true;

    }
}

// проверяет не нажал ли пользователь на кнопку вверх/вниз и если да, то возвращает
// новое значение указателя, а если нет, то старое. Указатель буферизируется и не меняется
// внутри функции
//  НЕ РЕНДЕРЕР
uint8_t upDown(uint8_t ptr, uint8_t items) {
    uint8_t tmp = ptr;

    //up.tick();
    //down.tick();
    if (up.click()) {
    //if (u8g2.getMenuEvent() == UP)
        debugln(F("Up"));
        if (ptr - 1 < 0) {
            u8g2.drawHLine(10, 0, SCREEN_WIDTH - 20); // если упремся в потолок меню, рисуем полоску сверху
            u8g2.updateDisplay();
            delay(250);
            u8g2.setDrawColor(0);
            u8g2.drawHLine(10, 0, SCREEN_WIDTH - 20);
            //u8g2.drawBox(10, 0, SCREEN_WIDTH - 20, 1); // и замажем ее
            u8g2.setDrawColor(1);
            u8g2.updateDisplay();
        }
        tmp = constrain(ptr - 1, 0, items - 1);
        //refresh_screen = true;
        //return tmp;
    }
    if (down.click()) {
    //else if (u8g2.getMenuEvent() == DOWN)
        debugln(F("down"));
        if (ptr + 1 > (uint8_t)(items - 1)) {
            u8g2.drawHLine(10, SCREEN_HEIGHT - 1, SCREEN_WIDTH - 20); // аналогичная прямая
            u8g2.updateDisplay();
            delay(250);
            u8g2.setDrawColor(0);
            u8g2.drawHLine(10, SCREEN_HEIGHT - 1, SCREEN_WIDTH - 20);
            //u8g2.drawBox(10, SCREEN_HEIGHT - 2, SCREEN_WIDTH - 15, 4); // и ее удаление
            u8g2.setDrawColor(1);
            u8g2.updateDisplay();
        }
        //need_refresh = true;
        //refresh_screen = true;
        tmp = constrain((uint8_t)(ptr + 1), 0, (uint8_t)(items - 1));
        //return tmp;
    }
    return tmp;
}


// обрабатывает перескакивание влево-вправо по порядкам в трехзначных числах Функция полностью аналогична upDown
uint8_t leftRight(uint8_t curr_pos, uint8_t num_items = 4) {
    uint8_t tmp = curr_pos;

    //left.tick();
    //right.tick();

    if (left.click()) {
    //if (u8g2.getMenuEvent() == LEFT)   
        debugln(F("left"));
        tmp = constrain(curr_pos - 1, 1, num_items - 1);
    }
    if (right.click()) {
    //else if (u8g2.getMenuEvent() == RIGHT)
        debugln(F("right"));
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
      delete[] mySubstring; // не забудь очистить память, дурачок (ты помнишь к чему приводит отсутствие этой строчки)
  }
*/
char *substring(const char *str, uint8_t start, uint8_t end) {
    // if (str == nullptr || start > end || start >= strlen(str))
    // {
    //     debugln("substring error");
    //     return nullptr;
    // }
    

    uint8_t substr_length = min(2*end, strlen(str)) - 2*start;

    if (substr_length < 2) 
        return nullptr;
    char *substr = new char[substr_length + 1]; // +1 из-за \n

    // Скопируем строку в новую переменную
    memcpy(substr, str + 2*start, substr_length);

    // Don't forget to null-terminate the new string
    substr[substr_length] = '\0';
    return substr;
}

// рассчет порядка числа
uint8_t calculateOrder(uint8_t number) {   
    // Не индусский код, а оптимизация памяти: "тупая" реализация экономит 42б. хех
    switch (number) {
        case 0: return 0;
        case 1 ... 9: return 1;
        case 10 ... 99: return 2;
        default: return 3;
    }

    // ... поэтому прийдется убрать процедурный подход
    // uint8_t order = 0;
    // if (number == 0)
    //     return 1;
    // else
    // {
    //     uint16_t tmp = number;
    //     while (tmp != 0)
    //     {
    //         tmp = tmp / 10;
    //         order++;
    //     }
    //     return order;
    // }
}

void printWarning() {
  u8g2.clear(); 
  if (menu_ptr == CYCLE) {
    u8g2.setCursor(4, 4 + 8);
    u8g2.print(F("Устройство работает."));

    u8g2.setCursor(4, 4 + 24);
    u8g2.print(F("Изменение настроек"));

    u8g2.setCursor(4, 4 + 40);
    u8g2.print(F("невозможно"));
  }
  else {

    u8g2.setCursor(4, 4 + 8);
    u8g2.print(F("Пока не реализовано."));

    u8g2.setCursor(4, 4 + 24);
    u8g2.print(F("Работа ведется."));
  }
  
  u8g2.drawFrame(63-5 - 2, 60 - 9, 10 + 4, 9+3);
  u8g2.setCursor(63-5, 60);
  u8g2.print(F("Ok"));
  //u8g2.drawButtonUTF8(63, 60, U8G2_BTN_HCENTER | U8G2_BTN_BW1, 0, 2, 1, "Ok");
  u8g2.updateDisplay();
  while (!enter.click()) {
    enter.tick();
  }
  return;    
}

#endif
