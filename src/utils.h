

#ifndef UTILS_H
#define UTILS_H
// #include "constants.h"
// #include "libs_header.h"

#include "global_variables.h"

#include <avr/eeprom.h>
#include "io_constructor.h"
#include "constants.h"
#include "libs_header.h"




/**
 * @brief функция, определяющая факт первой инициализации микроконтроллера
 * 
 * Нужна для инициализации регистра настроек и статуса. Просто для бесшовного программирования контроллера в 
 * потоке. Ардуинки отгружаются с EEPROM, заполненными единицами, так что если нулевой байт равен 0b11111111
 * (что невозможно, по построению структуры данных в моей программе), то это указывает на то, что ардуинка девственна
 * @return true - первая инициализация
 * @return false - иначе
 */
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



/**
 * @brief отлавливаем не была ли нажаты кнопки вверх/вниз и отрабатываем событие
 * 
 * Фактически, принимаем текущих номер элемента в меню и прибавляем/вычитаем единицу при возвращении 
 * номера элемента. Если кнопка не была нажата, то возвращает ptr без изменений. 
 * Начальный указатель буферизируется и не меняется
 * В случае, когда пользователь пытается "пробиться" сквозь потолок или пол, выводим на 250мс линию сверху или снизу 
 * экрана соответственно. В этом случае, указатель не меняется
 *  
 * НЕ РЕНДЕРЕР (?)
 * 
 * @param ptr - "указатель" (читай номер) элемента меню
 * @param items - макс число элементов на одном экране
 * @return uint8_t новый номер элемента (ptr +- 1, или ptr в случае выхода  за границы)
 */
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


/**
 * @brief Аналогично UpDown, толькодля отработки лево-право при выставлении численных значений (переход между 
 * порядками единицы <-> десятки <-> сотни)
 * 
 * @param curr_pos (текущий номер порядка 1(сотни)-3(единицы))
 * @param num_items макс. число порядком (1-4)
 * @return uint8_t обовленный номер порядка (0-3) [!TODO: почему-то мне не слишком комфортно смещение]
 */
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

/**
 * @brief замена string::substring
 * 
 * Выделяет подстроку по требуемому началу и концу, незаменимо в скролле.
 * Разработано под кириллицу (2 байта на символ)
 * по идее, это позволит перевести названия менюшек на char* и закинуть 
 * их в PROGMEM (см выше) и высвободит 96 байт (минимум. Класс стринг тоже тяжелый)
 * 
 * @example 
 *           char* mySub = substring(originalString, start, end);
 *           if (mySubstring != nullptr) {
 *               Serial.println(mySubstring);
 *               delete[] mySubstring; // не забудь очистить память, дурачок 
 *               // (надеюсь, ты помнишь чему приводит отсутствие этой строчки)
 *           }
 *
 * @param str исходная строка   
 * @param start номер первого символа подстроки 
 * @param end номер последнего символа подстроки
 * @return char* собствено, результирующая подстрока
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

/**
 * @brief Позволяет рассчитать порядок числа
 * 
 * ВАЖНО: Если не готов к приступу тяжелейшего снобизма, то лучше не смотри на реализацию.
 * Поверь, весь безбожный код имеет на то причины (см комментарии)
 * 
 * @param number исходное число (положительное, меньше 255)
 * @return uint8_t порядок числа 0 (меньше 10), 1 (меньше 100), 2 (меньше 1000), 3 (иначе. Пока не используется)
 */
uint8_t calculateOrder(uint8_t number) {   
    // Не индусский код, а оптимизация памяти: "тупая" реализация экономит 42б. хех
    switch (number) {
        case 0: return 0;
        case 1 ... 9: return 1;
        case 10 ... 99: return 2;
        default: return 3;

    }

    // // Очередная попытка оптимизировать без if и ветвлений.
    // // Вероятно, быстрее, так как программа не может "потерять внимание". 
    // // При этом, занимает на 20б больше ублюдской реализации. И мне очень лень тестировать скорость
    // // Это того не стоит
    //return 0 + 1*(number >=1 and number <10) + 2*(number >= 10 and number <100) + 3*(number >= 100);


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

    // ...как и все его альтернативные версии, давно удаленные за ненадобностью, несмотря на 
    // их изобретательность, да
}


/**
 * @brief Выводит на дисплей предупреждение о том, что устройство работает
 * и изменение настроек режима невозможно (слишком очевидно, да, я знаю)
 * Также блокирует доступ к пока нереализованным функциям
 * 
 * Блокирует управление, пока не нажат Enter (верхняя правая кнопка. По состоянию на 18.11 пин 18)
 * 
 */
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
