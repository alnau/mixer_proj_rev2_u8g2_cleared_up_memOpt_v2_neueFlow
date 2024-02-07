
//#define WOKWI

#define IS_DEBUG



#include "constants.h"
#include "libs_header.h"
#include "utils.h"
//#include "prog_utils.h"
#include "cycle_data.h"
#include "stepper.h"
#include "global_variables.h"
#include "io_constructor.h"


//#define FPSTR(pstr) (const __FlashStringHelper*)(pstr)



void printMenuLoadingScreen(const __FlashStringHelper* menu_name);

//prog_data PROG_DATA(PROG_FIRST_BYTE);


//задаем дефолтные значения рабочего режима, записываем их в энергонезависимую память
//а также задаем стандартные "настройки" в первый байт EEPROM
inline void initData() {
  if (check_if_first_init()) {
    //если устройство запускается впервые

    debugln(F("First start"));

    //установили дефолтные настройки
    CYCLE_DATA.t_accel = 10;              //s
    CYCLE_DATA.v_const = 150;             //rev/min
    CYCLE_DATA.t_const = 10;              //s
    CYCLE_DATA.t_slowdown = 2;            //s
    CYCLE_DATA.t_pause = 0;               //s
    CYCLE_DATA.num_cycles = 1;            //number 0-255
    CYCLE_DATA.is_accel_smooth = false;   //false->linear, true->sin^2(pi t/2)
    CYCLE_DATA.is_bidirectional = false;  // false->single direction, true->biderectional

    //и записали их в память EEPROM
    CYCLE_DATA.writeDataToMem();

    const uint8_t default_settings = 0b00010101;

    eeprom_update_byte((uint8_t*)0, default_settings);  //записали что звук включен, не было аварийной остановки и что инициализация была проведена
    //eeprom_update_byte((uint8_t*)1, 127);               //записали дефолтную яркость во второй бит
    eeprom_update_byte((uint8_t*)2, 0);
  } else {
    CYCLE_DATA.loadDataFromMem();  //инначе подгрузили установки режима из памяти
    loadSettigsRegister();         //и другие настройки
  }

  //PROG_DATA.load_data_to_buff(0,0);
}

inline void loadSettigsRegister() {
  uint8_t data_container = 0;
  data_container = eeprom_read_byte((uint8_t*)0);  //xxx(SAFE)(PROG)(SOUND)(EMERG)(INIT)
  //safe_stop = (data_container >> 4) & 0b00000001;
  emergency_stop = (data_container >> 1) & 0b00000001;
  need_sound = (data_container >> 2) & 0b00000001;
}


inline void initDisplay() {

  //u8g2.begin(/* menu_select_pin= */ ENTER_BTN, /* menu_next_pin= */ RIGHT_BTN, /* menu_prev_pin= */ LEFT_BTN, /* menu_up_pin= */ UP_BTN, /* menu_down_pin= */ DOWN_BTN, /* menu_home_pin= */ FUNC_BTN);
  //u8g2.begin(/* menu_select_pin= */ ENTER_BTN, /* menu_next_pin= */ U8X8_PIN_NONE, /* menu_prev_pin= */ U8X8_PIN_NONE, /* menu_up_pin= */ U8X8_PIN_NONE, /* menu_down_pin= */ U8X8_PIN_NONE, /* menu_home_pin= */ FUNC_BTN);
  u8g2.begin();
  u8g2.setContrast(0);
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_haxrcorp4089_t_cyrillic);
  

  printMenuLoadingScreen(F(" Загрузка..."));
  //refresh_screen = true;
}


inline void powerLoss() {
  
  if (is_working) {
    noInterrupts();

    pwr_loss = true;

    uint8_t eeprom_0x00 = eeprom_read_byte(0);
    bitWrite(eeprom_0x00, 1, 1);                            //установили флаг аварийной остановки
    //eeprom_0x00 = setBit(eeprom_0x00, working_in_programming_mode, 4);  //записали из какого режима велась работа
    eeprom_update_byte(0, eeprom_0x00);

    debugln(F("Emergency stop byte is set. Saving ramp state..."));
    interrupts();
    debug(F("Settings EEPROM Data: ")); debugln(eeprom_read_byte(0));

    return;
  } 
  else
    return;
}



void setup() {

#ifdef IS_DEBUG
  Serial.begin(BAUD_RATE);
  while(!Serial) {
    ;
  }
#endif
  //pinMode(BUZZER, OUTPUT);
  initDisplay();
  initData();
  initStepper();
  initTimer1();
  checkEmergencyStop();
  menu_ptr = SPEED;
  need_to_load_interface = true;

  //TODO обходится в 240б, что дорого. В финальной версии нужно 
  //реализовать на более низком уровне
  //attachInterrupt(digitalPinToInterrupt(PWR_LOSS), powerLoss, FALLING);  

  debug(F("Settings EEPROM Data: ")); debugln(eeprom_read_byte(0));
}



//проверяем при инициализации на факт, того что прошлый запуск процесса мешалки законился
//аварийной остановкой и, если так, то сообщаем об этом пользователю
// НЕ РЕНДЕРЕР
void checkEmergencyStop() {
  //emergency_stop = true;
  if (emergency_stop) {
    debugln(F("Inside emergency handler"));
    //По идее, это закинет нас в врерывание ISR(TIMER1_COMPA_vect), где произойдет процесс востановления режима работы
    is_restoring = true;
    OCR1A = 10;
    // Установим таймер с делителем на 64
    TCCR1B |= ((0 << CS12) | (1 << CS11) | (1 << CS10));
    //digitalWrite(ENA_PIN, LOW); //Разбудим двигатель


    // Выведем меню на экран
    //u8g2.clear();
    //u8g2.setCursor(0, 8);
    // TODO: перевести на userInterfaceMessage()
    //u8g2.print(F("Контроллер был \n\r отключен во время \n\r работы \n\r Зажмите Enter чтобы \n\r продолжить"));
    //u8g2. ("Контроллер был", "отключен во время работы", "Зажмите [E] чтобы продолжить", "Ok"); //блокирующее. Будет тяжело мерцать и орать
    //u8g2.updateDisplay();

    //необходимо подгрузить параметры последнего режима и выставить is_working, чтобы работа мешалки продолжалась по прерыванию таймера
    // working_in_programming_mode = (tmp >> 3) & 0b00000001;  // выставили режим, в котором происходит работа                            //восстановили предыдущий процесс

    uint16_t last_update_time = (uint16_t)millis();
    //bool is_lit = false;
    //TODO ВАЖНО ЗДЕСЬ МОЖНО ЗАДАВАТЬ ДЛИНУ СИГНАЛА, ТАК ЧТО МОЖНО УПРОСТИТЬ СИНТАКСИС
    // if (need_sound)
    //   tone(BUZZER, BUZZER_PITCH, 100);

    while (1) {
      //МЕРЦАЙ ЭКРАНОМ И ОРИ
      if ((uint16_t)millis() - last_update_time > 500) {
        // if (is_lit) {
        //   //u8g2.setContrast(70);
        //   is_lit = false;
        // } else {
        //   // if (need_sound) {
        //   //   tone(BUZZER, BUZZER_PITCH, 100);
        //   // }
        //   //u8g2.setContrast(0);
        //   is_lit = true;
        // }
        u8g2.clear();
        u8g2.setCursor(0, 8);
        u8g2.print(F(" Контроллер был \n\r отключен во время \n\r работы \n\r Зажмите Enter чтобы \n\r продолжить"));
        u8g2.updateDisplay();
        last_update_time = (uint16_t)millis();
      }

      if (enter.isClicked()) {
        //u8g2.setContrast(0);
        //noTone(BUZZER);
        emergency_stop = false;
        uint8_t tmp = eeprom_read_byte((uint8_t*)0);
        tmp = tmp & 0b11111101;  //вернули второй бит в состояние 0
        eeprom_update_byte((uint8_t*)0, tmp);
        return;
      }
    }
  }
}


//выводит разделы главного меню
// РЕНДЕРЕР
void printMainMenu() {
  for (uint8_t i = 0; i < MM_ITEMS; i++) {
    u8g2.setCursor(8, 4 + 8 + 16 * i);
    PGM_P pstr = pgm_read_word(main_menu_items + i);
    char buffer[strlen_P(pstr)+1];
    strcpy_P(buffer, pstr);
    u8g2.print(buffer);
  }
}

//осовная логика главного меню
// НЕ РЕНДЕРЕР
void mainMenu() {
  static uint8_t main_menu_ptr;

  if (need_to_load_interface) {
    main_menu_ptr = 0;
    refresh_screen = true;
    u8g2.clear();
    printMainMenu();
    //printPtr(main_menu_ptr);
    need_to_load_interface = false;
  }

  
  //основной процесс в меню
  if (enter.isClicked()) {
    need_to_load_interface = true;
    menu_ptr = 2 + main_menu_ptr;

  }
  else if(func.isClicked()) {
    menu_ptr = SPEED;
    need_to_load_interface = true;
    return;
  }

  uint8_t tmp_ptr = main_menu_ptr;
  main_menu_ptr = upDown(main_menu_ptr, MM_ITEMS);

  if (tmp_ptr != main_menu_ptr) {    
    refresh_screen = true;
  }

  printPtr(main_menu_ptr);
}


//Выводит текущую скорость
//НЕ РЕНДЕРЕР
void printSpeedMenu(uint8_t speed, bool is_working) {
  //scale = 5 => w = 6*4 = 24
  const uint8_t start_row_y = 3;

  uint8_t order = calculateOrder(speed);
  u8g2.clear();
  u8g2.setCursor(5, 18);
  if (is_working and !is_stopping)
    u8g2.print(F("вкл"));
  else
    u8g2.print(F("выкл"));


  u8g2.setCursor(5 + 3 * 8 - order * 8 + 8-10, 8 * start_row_y + 26);

  u8g2.setFont(u8g2_font_inr24_mn);
  u8g2.print(speed);
  u8g2.setFont(u8g2_font_haxrcorp4089_t_cyrillic);
  u8g2.drawRFrame(0, 8 * start_row_y - 4, 126, 36, FRAME_RADIUS);
  u8g2.setCursor(75 + 5 + 8-10, 32 + 8 * start_row_y - 6);
  u8g2.print(F("об/мин"));
  refresh_screen = true;

  //oled.update(); 02.02.24 АХАХ, серьезно?! Оставлю на память [контекст: GyverOLED]
}





void speedMenu() {

  uint8_t speed = 0; 
  static uint8_t prev_speed;
  static uint16_t t_since_last_update;
  const uint16_t SCALER = 30*ALPHA*T1_FREQ/PI; 

  //если только зашли после пред. менюшки
  if (need_to_load_interface) {
    t_since_last_update = (uint16_t)millis();

    if (is_working and (srd.run_state != PAUSE)) {
      // w = ALPHA*f/delay[рад/с]
      // RPM = w/(2*pi)*60 [об/мин]
      speed = (uint8_t)(SCALER/(srd.step_delay));
      
    }

    printSpeedMenu(speed, is_working);
    need_to_load_interface = false;
  }

  
  //обработка меню при вызове
  if ((is_working or is_stopping) or refresh_screen) {
    if (srd.run_state != PAUSE)
      speed = (uint8_t)(SCALER/(srd.step_delay));
    else 
      speed = 0;

    if ((prev_speed != speed) and ((uint16_t)millis() - t_since_last_update > UPDATE_PERIOD)) {
      prev_speed = speed;
      t_since_last_update = (uint16_t)millis();
      printSpeedMenu(speed, is_working);
    }   
  }

  if (enter.isClicked()) {
    if (!need_to_stop and !is_working) {
      //если не работали и нажали Enter, начинаем работу
      is_working = true;
      startMotor();
    }
    else if (is_working and !need_to_stop) {
      //Если двигатель работал и пользователю необходимо его остановить
      need_to_stop = true;  //Установим флаг, который будет обрабатываться в прерывании motor_cntr.c
      refresh_screen = true;
    }
  }
  else if (func.isClicked()) {
    //если пользователь нажал на настройку,выкидываем его в главное меню, при этом продолжая работу в прерываниях
    menu_ptr = MAIN;
    need_to_load_interface = true;

    return;
  }
}


//выводит время в формате M:SS у правого края экрана
// Если выставлен is_setup, то подчеркнет цифру
//с порядковым номером digit
//РЕНДЕРЕР (НО МОЖНО ОПТИМИЗИРОВАТЬ)
void printTime(uint16_t T, uint8_t ptr, bool is_setup = false, uint8_t digit = 0) {

  enum DIGIT {
    MIN = 1,
    SEC1 = 2,
    SEC2 = 3
  };

  uint8_t m = T/60;
  uint8_t s1 = (T % 60) / 10;
  uint8_t s2 = (T % 60) % 10;

  char m_c[2];
  char s1_c[2];
  char s2_c[2];

  itoa(m, m_c, 10);     //переведем инт в строку (десятичное представление)
  itoa(s1, s1_c, 10);
  itoa(s2, s2_c, 10);
  
  const uint8_t x_0 = SCREEN_WIDTH - 4 * 6 - DATA_X_BIAS + 5;
  uint8_t y_0 = 4 + ptr * 16 + 8;

  uint8_t m_width = u8g2.getStrWidth(m_c);
  uint8_t s1_width = u8g2.getStrWidth(s1_c);
  uint8_t s2_width = u8g2.getStrWidth(s2_c);
  uint8_t separator_width = u8g2.getStrWidth(":");


  u8g2.setDrawColor(0);
  u8g2.drawBox(x_0, y_0-8, 20,11);
  u8g2.setDrawColor(1);


  u8g2.setCursor(x_0, y_0);
  u8g2.print(m);
  u8g2.print(F(":"));
  u8g2.print(s1);
  u8g2.print(s2);
  if (is_setup) {
    switch(digit) {
      case MIN:
        u8g2.drawHLine(x_0, y_0 + 1, m_width);
        break;
      case SEC1: 
        u8g2.drawHLine(x_0 + m_width + separator_width + 2, y_0 + 1, s1_width);
        break;
      case SEC2: 
        u8g2.drawHLine(x_0 + m_width + s1_width + separator_width + 3, y_0 + 1, s2_width);
        break;
    }
  }
  refresh_screen = true;
}

//режим установки времени работы
// НЕ РЕНДЕРЕР
uint16_t setupTime(uint16_t T, uint8_t ptr) {

  static uint8_t digit;
  uint8_t tmp_digit;
  int16_t tmp = T;
  
  if (need_to_load_interface) {
    digit = 1;
    
    need_to_load_interface = false;
    printTime(tmp, ptr, true, digit);
  }


  if (enter.isClicked()) {
    printTime(tmp, ptr);

    menu_ptr = CYCLE;
    need_update_EEPROM = true;
    return tmp;
  }

  tmp_digit = digit;

  digit = leftRight(digit);
  if (digit != tmp_digit) {
    printTime(tmp, ptr, true, digit);
  } 
  if (up.isClicked()) {
    //МОГУ ЛИ Я СЕБЕ ДОВЕРЯТЬ, УЧИТЫВАЯ СОСТОЯНИЕ?
    //Оптимизация через Look up table не дает преимущества
    switch(digit) {
      case 1:
        //минуты
        tmp = constrain(tmp + 60, 0, MAX_TIME);
        break;
      case 2:
        //десятки минут
        tmp = constrain(tmp + 10, 0, MAX_TIME);
        break;
      case 3:
        //секунды
        tmp = constrain(tmp + 1, 0, MAX_TIME);
        break;
    }
    printTime(tmp, ptr, true, digit);
  }
  else if (down.isClicked()) {
    //если минуты
    switch(digit) {
      case 1:
        //минуты
        tmp = constrain(tmp - 60, 0, MAX_TIME);
        break;
      case 2:
        //десятки минут
        tmp = constrain(tmp - 10, 0, MAX_TIME);
        break;
      case 3:
        //секунды
        tmp = constrain(tmp - 1, 0, MAX_TIME);
        break;
    }
    printTime(tmp, ptr, true, digit);
  }
  return tmp;
  
}




//Вывод числа в информационной колонке. Ожидает не больше трехзначных
// РЕНДЕРЕР (НО МОЖЕМ ОПТИМИЗИРОВАТЬ)
void printNumbers(uint8_t data, uint8_t ptr, bool is_setup = false, uint8_t digit = 0) {

  uint8_t order = calculateOrder(data);  //рассчет порядка числа

  const uint8_t x_0 = SCREEN_WIDTH - 3 * 6 - DATA_X_BIAS;
  uint8_t y_0 = 2 * ptr * 8 + 1 * 8 + 5;
  
  u8g2.setDrawColor(0);
  u8g2.drawBox(x_0, y_0-8, 20,11);
  u8g2.setDrawColor(1);
  u8g2.setCursor(x_0, y_0);
  if (data == 0) {
    u8g2.print(F("000"));
  }
  if (order < 3) {
    for (uint8_t i = 1; i <= 3 - order; i++)
      u8g2.print(F("0"));  //заполняем нулями числа меньше сотен, чтобы было можно выставлять старшие порядки
  }
  u8g2.print(data);

  if (is_setup) {
    enum ORDER {
      HUNDREDS = 1,
      TENS = 2,
      ONES = 3
    };

    uint8_t h = data/100;
    uint8_t t = (data % 100)/10;
    uint8_t o = data % 10;

    
    char h_c[2];
    char t_c[2];
    char o_c[2];

    itoa(h, h_c, 10);
    itoa(t, t_c, 10);
    itoa(o, o_c,10);


    uint8_t h_width = u8g2.getStrWidth(h_c);
    uint8_t t_width = u8g2.getStrWidth(t_c);
    uint8_t o_width = u8g2.getStrWidth(o_c);

    switch(digit) {
      case HUNDREDS:
        u8g2.drawHLine(x_0, y_0 + 1, h_width);
        break;
      case TENS: 
        u8g2.drawHLine(x_0 + h_width + 1, y_0 + 1, t_width);
        break;
      case ONES: 
        u8g2.drawHLine(x_0 + h_width + t_width + 2, y_0 + 1, o_width);
        break;
    }
  }
  refresh_screen = true;
}

/*
  //вывод одной цифры без ведущих нулей. Используется при установки яркости
  void printOneNumber(uint8_t number, uint8_t ptr, bool is_setup) {

    u8g2.setDrawColor(0);
    u8g2.drawBox(SCREEN_WIDTH - 3 * 6 - DATA_X_BIAS, (2 * ptr + 1) * 8 + 5, 16 + DATA_X_BIAS, 1);
    u8g2.setDrawColor(1);
    u8g2.setCursor(SCREEN_WIDTH - 6 - DATA_X_BIAS, 4 + ptr * 16 + 8);

    u8g2.print(number);
    if (is_setup) {
      u8g2.drawHLine(SCREEN_WIDTH - 6 - DATA_X_BIAS, (2 * ptr + 1) * 8 + 5, 4);
    }
  }
*/

//перегрузка на случай любой (в разумных пределах) длины
// РЕНДЕРЕР
void printText(const __FlashStringHelper* text, uint8_t ptr, uint8_t len) {
  u8g2.setDrawColor(0);
  u8g2.drawBox(SCREEN_WIDTH - 3 * 6 - DATA_X_BIAS, (2 * ptr + 1) * 8 + 5, 16 + DATA_X_BIAS, 1);
  u8g2.setDrawColor(1);
  u8g2.setCursor(SCREEN_WIDTH - len * 6 - DATA_X_BIAS, 4 + ptr * 16 + 8);
  u8g2.print(text);
  refresh_screen = true;
}


//цикл установки чисел в информационной колонке
// НЕ РЕНДЕРЕР
uint8_t setupNumbers(uint8_t data, uint8_t ptr) {
  static uint8_t digit;
  int16_t tmp = data;
  uint8_t tmp_digit;
  //uint8_t order = calculateOrder(data);

  if (need_to_load_interface) {
    digit = 1;
    //tmp = data; 
    //tmp_digit = digit;
    printNumbers(tmp, ptr, true, digit);
    //u8g2.updateDisplay();
    //refresh_screen = true;
    need_to_load_interface = false;
  }

  

    //enter.tick();
    if (enter.isClicked())  {
    //if (u8g2.getMenuEvent() == ENTER) {
      printNumbers(tmp, ptr);
      //u8g2.updateDisplay();
      //refresh_screen = true;
      //data = tmp;
      //need_to_load_interface = true;
      menu_ptr = CYCLE;
      need_update_EEPROM = true;
      return tmp;
    }

    tmp_digit = digit;
    digit = leftRight(digit);
    // TODO !!! проверить, это слишком хорошо чтобы быть правдой
    if (tmp_digit != digit) {
      printNumbers(tmp, ptr, true, digit);

      //refresh_screen = true;
      //u8g2.updateDisplay();
    }
    if (up.isClicked()) {
    //if (u8g2.getMenuEvent() == UP) {
      //если сотни
      switch (digit) {
        case 1:
          tmp = constrain(tmp + 100, 1, MAX_NUM);
          break;
        case 2: 
          tmp = constrain(tmp + 10, 1, MAX_NUM);
          break;
        case 3:
          tmp = constrain(tmp + 1, 1, MAX_NUM);
          break;
      }
      printNumbers(tmp, ptr, true, digit);
      //refresh_screen = true;
      //u8g2.updateDisplay();
    } else if (down.isClicked()) {
    //else if (u8g2.getMenuEvent() == DOWN) {
      //если сотни
      switch (digit) {
        case 1:
          tmp = constrain(tmp - 100, 0, MAX_NUM);
          break;
        case 2: 
          tmp = constrain(tmp - 10, 0, MAX_NUM);
          break;
        case 3:
          tmp = constrain(tmp - 1, 0, MAX_NUM);
          break;
      }
      printNumbers(tmp, ptr, true, digit);
      //refresh_screen = true;
      //u8g2.updateDisplay();
    }
    return tmp;
}


// РЕНДЕРЕР
void clearMenuItem(uint8_t cursor) {
  u8g2.setDrawColor(0);
  u8g2.drawBox(8, 4 + cursor * 16, SCREEN_WIDTH - DATA_COL_WIDTH - DATA_X_BIAS - 8, 10);
  u8g2.setDrawColor(1);
}


//функция чтобы скроллить текст в строке cursor из менюшки setup_cycle
//т.е. цикл находится там, а шаг проводится здесь
// НЕ РЕНДЕРЕР
uint8_t scrollText(char* text, uint8_t cursor, uint8_t counter, uint8_t cutoff = TEXT_MAX_LEN) {

  
  static uint16_t prev_scroll_time;
  
  if ((uint16_t)millis() - prev_scroll_time > SCROLL_PERIOD) {
    //uint8_t len = (strlen(text) + 1) / 2;
    uint8_t len = strlen(text)/2 - 1;
    clearMenuItem(cursor);
    u8g2.setCursor(8, 4 + cursor * 16 + 8);
    if (len >= TEXT_MAX_LEN) {
      uint8_t start_ptr = counter % len;
      //debugln(start_ptr);
      
      char* tmp_substr = substring(text, start_ptr, start_ptr + TEXT_MAX_LEN);
      u8g2.print(tmp_substr);
      //debugln(tmp_substr);
      delete[] tmp_substr;
      
      prev_scroll_time = (uint16_t)millis();
      refresh_screen = true;
      return counter + 1;
    }
    else {
      //!!! TODO: в теории, эту проверку можно перенести на substring, заменяя end на strlen
      u8g2.print(text);
      return counter;
    }
  } else
    return counter;
}



//печатает названия элементов режима установки вращения по страницам на
//основе номера элемента cursor
// РЕНДЕРЕР (НО МОЖНО ОПТИМИЗТРОВАТЬ)
void printSetupPages(uint8_t cursor) {

  if (cursor < 4) {
    for (uint8_t i = 0; i < 4; i++) {
      u8g2.setCursor(8, 4 + i * 16 + 8);

      PGM_P pstr = pgm_read_word(setup_menu_items + i);
      char buffer[strlen_P(pstr)+1];
      strcpy_P(buffer, pstr);

      char* tmp_substr = substring(buffer, 0, TEXT_MAX_LEN);
      u8g2.print(tmp_substr);
      delete[] tmp_substr;
    }
  } else {
    for (uint8_t i = 0; i < 4; i++) {
      u8g2.setCursor(8, 4 + i * 16 + 8);

      PGM_P pstr = pgm_read_word(setup_menu_items + i + 4);
      char buffer[strlen_P(pstr)+1];
      strcpy_P(buffer, pstr);

      char* tmp_substr = substring(buffer, 0, TEXT_MAX_LEN);
      u8g2.print(tmp_substr);
      delete[] tmp_substr;
    }
  }
}

//обновляет элемент менюшки установки режима вращения под номером cursor.
//используется для выхода из режима прокрутки текста
// РЕНДЕРЕР
void refreshMenuItem(char* item, uint8_t cursor) {
  u8g2.setDrawColor(0);
  u8g2.drawBox(8,4+(cursor%4)*16,90 , 10);
  u8g2.setDrawColor(1);
  u8g2.setCursor(8, 4 + (cursor % 4) * 16 + 8);
  
  char* tmp_substr = substring(item, 0, TEXT_MAX_LEN);
  u8g2.print(tmp_substr);
  refresh_screen = true;
  delete[] tmp_substr;
}

//выводит данные меню установки режима вращения в информационную колонку в правой части экрана
// РЕНДЕРЕР
void printSetupData(uint8_t cursor) {
  if (cursor < 4) {
    printTime(CYCLE_DATA.t_accel, 0);
    printNumbers(CYCLE_DATA.v_const, 1);
    printTime(CYCLE_DATA.t_const, 2);
    printTime(CYCLE_DATA.t_slowdown, 3);
  } else {
    printTime(CYCLE_DATA.t_pause, 0);
    printNumbers(CYCLE_DATA.num_cycles, 1);
    printAccelRegime(CYCLE_DATA.is_accel_smooth, false);
    printCycleRegime(CYCLE_DATA.is_bidirectional, false);
  }
}

//выводит текстовое название режима ускорения в информационное меню.
//син - для гладкого, лин для линейного.
//Если выставлен is_setup, то подчеркиевает этот текст
// РЕНДЕРЕР
void printAccelRegime(bool is_smooth, bool is_setup) {
  if (is_smooth)
    printText(F("син"), 6 % 4, 2);
  else
    printText(F("лин"), 6 % 4, 2);

  if (is_setup) {
    u8g2.drawHLine(SCREEN_WIDTH - 3 * 6 - DATA_X_BIAS, (2 * 2 + 1) * 8 + 5, 16);
  }
}

//выводит текстовое название режима повторения в информационное меню.
//1ст в случае одностороннего, 2ст в случае двустороннего
//если выставлен is_setup, то подчеркивает текст
// РЕНДЕРЕР
void printCycleRegime(bool is_reversive, bool is_setup) {
  if (is_reversive)
    printText(F("2ст"), 7 % 4, 2);
  else
    printText(F("1ст"), 7 % 4, 2);
  if (is_setup) {
    u8g2.drawHLine(SCREEN_WIDTH - 3 * 6 - DATA_X_BIAS, (2 * 3 + 1) * 8 + 5, SCREEN_WIDTH - 16 - DATA_X_BIAS);
  }
}

//установка режима ускорения
// НЕ РЕНДЕРЕР
bool setupAccel() {
  
  static bool tmp;
  
  if (need_to_load_interface) {
    printAccelRegime(CYCLE_DATA.is_accel_smooth, true);
    need_to_load_interface = false;
    tmp = CYCLE_DATA.is_accel_smooth;
  }
   
  if (enter.isClicked())  {
    printAccelRegime(tmp, false);
    menu_ptr = CYCLE;
    need_update_EEPROM = true;
    return tmp;
  }
  else if (up.isClicked() or down.isClicked())  {
    tmp = !tmp;
    printAccelRegime(tmp, true);
  }
  return tmp;
  
}

//установка режима повторения
// НЕ РЕНДЕРЕР
bool setupRepeat() {
  static bool tmp;

  if (need_to_load_interface) {
    printCycleRegime(CYCLE_DATA.is_bidirectional, true);
    tmp = CYCLE_DATA.is_bidirectional;
    need_to_load_interface = false; 
  }

  if (enter.isClicked()) {
    printCycleRegime(tmp, false);
    menu_ptr = CYCLE;
    need_update_EEPROM = true;
    return tmp;
  }
  else if (up.isClicked() or down.isClicked()) {
    tmp = !tmp;
    printCycleRegime(tmp, true);
  }
  return tmp;
}


//вывод стандартизированного загрузочного окна для меню
//в конце очищает экран и возвращает курсор домой
// РЕНДЕРЕР (НО МОЖНО ДОРАБОТАТЬ)
void printMenuLoadingScreen(const __FlashStringHelper* menu_name) {
  u8g2.clear();
  u8g2.setCursor(8, 3 * 8 + 6);
  u8g2.print(menu_name);
  u8g2.drawRFrame(0, 3 * 8 - 4, 126, 14, FRAME_RADIUS);
  u8g2.updateDisplay();
  delay(LOADING_TIME);

  u8g2.clear();
}

//функция, вывода и обработки меню режима вращения
// НЕ РЕНДЕРЕР
void setupCycle() {

  bool need_change_page = false;
  static uint8_t setup_ptr;
  static uint8_t counter;  //переменная, считающая циклы, чтобы передавать ее в функцию бегущего текста
  uint8_t tmp_ptr; 
  //static bool never_checked = true;
  
  
  if (need_to_load_interface) {
    setup_ptr = 0;
    counter = 0;
    refresh_screen = true;

    if (is_working /*and never_checked*/) {
      printWarning();
      
      u8g2.clear();
      //never_checked = false;
    
      
    } else {
      printMenuLoadingScreen(F(" Режим"));
    }

    printSetupPages(setup_ptr);
    printSetupData(setup_ptr);
    need_to_load_interface = false;
  }
  

  // Основной процесс рендеринга менюшки
  if (func.isClicked()) {
    need_to_load_interface = true;
    menu_ptr = MAIN;
    return;
  }
  else if (enter.isClicked() and !is_working) {
    need_to_load_interface = true;
    /*
     * Тут нужно пояснение
     * хитрый хак в стиле switch. Все литралы сетапов равны 6..13 (см constants.h), соответственно, в данной
     * реализации прибавление setup_ptr к шестерки автоматически вычисляет нужный литерал
     * всго за один такт 
     
    */
    menu_ptr = 6 + setup_ptr; 
    return;
  }

  //если требуется обновить энергонезависимую память
  if (need_update_EEPROM) {
    CYCLE_DATA.writeDataToMem();
    need_update_EEPROM = false;
  }

  tmp_ptr = setup_ptr;
  setup_ptr = upDown(setup_ptr, SETUP_ITEMS);
  if (tmp_ptr != setup_ptr) {
    //отловили момент перехода к следующему элементу
    counter = 0;
    //refresh_screen = true;

    PGM_P pstr = pgm_read_word(setup_menu_items + tmp_ptr);
    char buffer[strlen_P(pstr)+1];
    strcpy_P(buffer, pstr);
    refreshMenuItem(buffer, tmp_ptr);
  }

  //если нужно перейти с первой на вторую на вторую страницу
  if (((setup_ptr == 4) and (tmp_ptr == 3)) or ((setup_ptr == 3) and (tmp_ptr == 4))) {
    // TODO: ожидаю что это эквивалентно (setup_ptr | tmp_ptr) == 7 (нет, потому-что 7 | 1 == 7)
    //флаг обновления дисплея выставлен в прошлом if 
    need_change_page = true;
  } else {
    //если остались на той-же
    PGM_P pstr = pgm_read_word(setup_menu_items + setup_ptr);
    char buffer[strlen_P(pstr)+1];
    strcpy_P(buffer, pstr);

    counter = scrollText(buffer, setup_ptr % 4, counter);
  }

  if (need_change_page) {
    need_change_page = false;
    u8g2.clear();
    printSetupPages(setup_ptr);

    PGM_P pstr = pgm_read_word(setup_menu_items + setup_ptr);
    char buffer[strlen_P(pstr)+1];
    strcpy_P(buffer, pstr);

    counter = scrollText(buffer, setup_ptr % 4, counter);

    printSetupData(setup_ptr);
  }

  printPtr(setup_ptr%4);
  printScrollBar(setup_ptr, SETUP_ITEMS);

}


//меню режима программирования
void programming() {

  // while(!u8g2.userInterfaceMessage("Пока не реализо-", "вано.", "Работа ведется.", "Ok")) {
  //   ;
  // }
  printWarning();
  need_to_load_interface = true;

  menu_ptr = MAIN;
  return;
  // uint8_t prog_ptr = prog_num;  //положение курсора в меню программирования
  // uint8_t page_num = 0;
  // uint8_t tmp_ptr = 0;

  // printMenuLoadingScreen(F(" Программирование"));
  // printProgrammingPages(0);
  // printScrollBar(prog_ptr, PROG_ITEMS);
  // printPtr(prog_ptr % 4);
  // u8g2.updateDisplay();
  // while (1) {
  //   func.tick();
  //   if (func.isClicked() or func.hold()) return;

  //   enter.tick();
  //   if (enter.isClicked() or enter.hold()) {
  //     setupProgram(prog_ptr);
  //     u8g2.clear();
  //     printProgrammingPages(prog_ptr / 4);
  //   }

  //   tmp_ptr = prog_ptr;
  //   prog_ptr = upDown(prog_ptr, PROG_ITEMS);

  //   if ((tmp_ptr == 3) and (prog_ptr == 4)) {
  //     //переход с первой страницу на вторую
  //     u8g2.clear();
  //     printProgrammingPages(1);
  //     printScrollBar(prog_ptr, PROG_ITEMS);
  //     printPtr(prog_ptr % 4);
  //     u8g2.updateDisplay();
  //   } else if ((tmp_ptr == 4) and (prog_ptr == 3)) {
  //     //переход со второй страницы на первую
  //     u8g2.clear();
  //     printProgrammingPages(0);
  //     printScrollBar(prog_ptr, PROG_ITEMS);
  //     printPtr(prog_ptr % 4);
  //     u8g2.updateDisplay();
  //   } else if (tmp_ptr != prog_ptr) {
  //     printScrollBar(prog_ptr, PROG_ITEMS);
  //     printPtr(prog_ptr % 4);
  //     u8g2.updateDisplay();
  //   }
  //   printScrollBar(prog_ptr, PROG_ITEMS);
  //   printPtr(prog_ptr % 4);
  //   u8g2.updateDisplay();
  // }
}

//вывод элементов меню настройки
// РЕНДЕРЕР (НО МОЖНО ОПТИМИЗИРОВАТЬ)
void printSettingsPages(uint8_t cursor) {

  if (cursor < 4) {
    for (uint8_t i = 0; i < 4; i++) {
      PGM_P pstr = pgm_read_word(settings_items + i);
      char buffer[strlen_P(pstr)+1];
      strcpy_P(buffer, pstr);

      u8g2.setCursor(8, 4 + 16 * i + 8);

      char* tmp_substr = substring(buffer, 0, TEXT_MAX_LEN);
      u8g2.print(tmp_substr);
      delete[] tmp_substr;
    }
    return;
  } else {
    for (uint8_t i = 4; i < 8; i++) {
      PGM_P pstr = pgm_read_word(settings_items + i);
      char buffer[strlen_P(pstr)+1];
      strcpy_P(buffer, pstr);

      u8g2.setCursor(8, 4 + 16 * (i - 4) + 8);

      char* tmp_substr = substring(buffer, 0, TEXT_MAX_LEN);
      u8g2.print(tmp_substr);
      delete[] tmp_substr;
    }
    return;
  }
}



//bool need_sound = true;
// НЕ РЕНДЕРЕР
void settings() {

  static uint8_t settings_cursor;
  static uint8_t counter;  //переменная, считающая циклы, чтобы передавать ее в функцию бегущего текста
  uint8_t tmp_ptr;
  bool need_change_page = false;

  if (need_to_load_interface) {
    settings_cursor = 0;
    counter = 0;
    printMenuLoadingScreen(F(" Hacтройки"));

    printSettingsPages(settings_cursor);
    printSettingsData(settings_cursor);
    need_to_load_interface = false;
  }

  if (func.isClicked()) {
    need_to_load_interface = true;
    menu_ptr = MAIN;
    return;
  }
  else if (enter.isClicked()) {
    need_to_load_interface = true;
    menu_ptr = settings_cursor + 14;
    counter = 0;

    //return; //TODO: (на 18.01) если убрать этот return, то можно сэкономить 2б
  }

  if (refresh_screen) {
    refreshSettings();
  }

  tmp_ptr = settings_cursor;
  settings_cursor = upDown(settings_cursor, SETTINGS_ITEMS);
  if (((tmp_ptr == 4) and (settings_cursor == 3)) or ((tmp_ptr == 3) and (settings_cursor == 4))) {
    //переход со второй страницы на первую
    counter = 0;
    need_change_page = true;
    debugln(F("2->1"));
    
  } else if (tmp_ptr != settings_cursor) {
    counter = 0;
    clearMenuItem(tmp_ptr);

    PGM_P pstr = pgm_read_word(settings_items + tmp_ptr);
    char buffer[strlen_P(pstr)+1];
    strcpy_P(buffer, pstr);

    refreshMenuItem(buffer, tmp_ptr);
    //refresh_screen = true;
  } else {

    //TODO: вполне возможно перенести распаковку PROGMEM на scrollText
    PGM_P pstr = pgm_read_word(settings_items + settings_cursor);
    char buffer[strlen_P(pstr)+1];
    strcpy_P(buffer, pstr);
  
    counter = scrollText(buffer, settings_cursor % 4, counter);
  }

  if (need_change_page) {
    counter = 0;
    u8g2.clear();
    printSettingsPages(settings_cursor);
    printSettingsData(settings_cursor);
    need_change_page = false;
  }

  
  printPtr(settings_cursor % 4);
  printScrollBar(settings_cursor, SETTINGS_ITEMS);
  
}

// РЕНДЕРЕР
void printSafeStopStatus(bool state, bool is_setup = false) {
  const uint8_t ptr = 1;
  u8g2.setDrawColor(0);
  u8g2.drawBox(SCREEN_WIDTH - 4 * 6 - DATA_X_BIAS, 4 * 8 + 5, 21 + DATA_X_BIAS, 8);
  u8g2.setDrawColor(1);
  u8g2.drawHLine(SCREEN_WIDTH - 3 * 6 - DATA_X_BIAS, (2 * ptr + 1) * 8 + 5, 3 * 5 + DATA_X_BIAS);
  
  if (state) {
    u8g2.setCursor(SCREEN_WIDTH - 3 * 6 - DATA_X_BIAS, 4 + ptr * 16 + 8);
    u8g2.print(F("вкл"));
    if (is_setup) {
      u8g2.drawHLine(SCREEN_WIDTH - 3 * 6 - DATA_X_BIAS, (2 * ptr + 1) * 8 + 5, 18);
    }
  } else {
    u8g2.setCursor(SCREEN_WIDTH - 4 * 6 - DATA_X_BIAS, 4 + ptr * 16);
    u8g2.print(F("выкл"));
    if (is_setup) {
      u8g2.drawHLine(SCREEN_WIDTH - 4 * 6 - DATA_X_BIAS, (2 * ptr + 1) * 8 + 5, 24);
    }
  }
}



//вывод статуса текущей громкости
// РЕНДЕРЕР
void printSoundStatus(bool state, bool is_setup = false) {
  const uint8_t ptr = 0;
  u8g2.setDrawColor(0);
  u8g2.drawBox(SCREEN_WIDTH - 4 * 6 - DATA_X_BIAS, 3 * 8 + 5, 21 + DATA_X_BIAS, 8);
  u8g2.setDrawColor(1);
  u8g2.drawHLine(SCREEN_WIDTH - 3 * 6 - DATA_X_BIAS, (2 * ptr + 1) * 8 + 5, DATA_X_BIAS + 15);

  if (state) {
    u8g2.setCursor(SCREEN_WIDTH - 3 * 6 - DATA_X_BIAS, 4 + ptr * 16 + 8);
    u8g2.print(F("вкл"));
    if (is_setup) {
      u8g2.drawHLine(SCREEN_WIDTH - 3 * 6 - DATA_X_BIAS, (2 * ptr + 1) * 8 + 5, 18);
    }
  } else {
    u8g2.setCursor(SCREEN_WIDTH - 4 * 6 - DATA_X_BIAS, 4 + ptr * 16 + 8);
    u8g2.print(F("выкл"));
    if (is_setup) {
      u8g2.drawHLine(SCREEN_WIDTH - 4 * 6 - DATA_X_BIAS, (2 * ptr + 1) * 8 + 5, 24);
    }
  }
  refresh_screen = true;
}

//вывод данных настроек в информационную колонку
// РЕНДЕРЕР
void printSettingsData(uint8_t settings_ptr) {
  printSoundStatus(need_sound);
  //printSafeStopStatus(safe_stop, false);
  //printText(F("TODO"), 3, 4);
  //printText(F("TODO"), 0, 4);
}



//обновение настроек, записанных в энергонезависимой памяти
void refreshSettings() {
  uint8_t tmp = eeprom_read_byte((uint8_t*)0);

  debug(tmp);
  debug(F(" --> "));

  bitWrite(tmp, 2, need_sound);
  //bitWrite(tmp, 5, safe_stop);
  eeprom_update_byte((uint8_t*)0, tmp);
  debugln(tmp);
  debugln(F("Settings updated"));
}

// НЕ РЕНДЕРЕР
// bool setupSafeStop() {
//   static bool tmp_safe_stop;

//   if (need_to_load_interface) {
//     tmp_safe_stop = safe_stop;
//     printSafeStopStatus(tmp_safe_stop, true);
//     //u8g2.updateDisplay();
//     refresh_screen = true;
//     need_to_load_interface = false;
//   }



//     //enter.tick();
//     //up.tick();
//     //down.tick();
//     if (enter.isClicked()) {
//     //if (u8g2.getMenuEvent()==ENTER ) {
//       printSafeStopStatus(tmp_safe_stop);
//       refresh_screen = true;
//       //u8g2.updateDisplay();
//       need_to_load_interface = true;
//       menu_ptr = SETTINGS;
//       return tmp_safe_stop;
//     }
//     else if(up.isClicked() or down.isClicked()) {
//     //else if ((u8g2.getMenuEvent() == UP) or (u8g2.getMenuEvent() == DOWN)) {
//       tmp_safe_stop = !tmp_safe_stop;
//       printSafeStopStatus(tmp_safe_stop, true);
//       refresh_screen = true;
//       //u8g2.updateDisplay();
//     }
//   return tmp_safe_stop;
// }


//установка громкости
// НЕ РЕНДЕРЕР
bool setupSound() {
  bool tmp_need_sound = need_sound;

  if (need_to_load_interface) {
    //tmp_need_sound = need_sound;
    printSoundStatus(tmp_need_sound, true);
    //refresh_screen = true;
    //u8g2.updateDisplay();
    need_to_load_interface = false;
  }

  if (enter.isClicked()) {
    printSoundStatus(tmp_need_sound);
    menu_ptr = SETTINGS;
    return tmp_need_sound;
  }
  else if (up.isClicked() or down.isClicked()) {

    tmp_need_sound = !tmp_need_sound;
    printSoundStatus(tmp_need_sound, true);
  }
  return tmp_need_sound;
  
}

//отрисовка курсора в менющке из восьми элементов
// РЕНДЕРЕР
void printPtr(uint8_t ptr) {
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, 4, SCREEN_HEIGHT-4);
  u8g2.setDrawColor(1);
  u8g2.setCursor(0, ptr * 16 + 4 + 8);
  u8g2.print(F(">"));
}


// РЕНДЕРЕР
void printScrollBar(uint8_t ptr, uint8_t num_items) {
  uint8_t bar_height = (SCREEN_HEIGHT - 8) / num_items;
  u8g2.setDrawColor(0);
  //u8g2.drawBox(SCREEN_WIDTH - 3, 0, 3, SCREEN_HEIGHT);
  u8g2.drawVLine(SCREEN_WIDTH - 1, 4 + bar_height * (ptr-1), 3*bar_height);
  u8g2.setDrawColor(1);
  u8g2.drawVLine(SCREEN_WIDTH - 1, 4 + bar_height * ptr, bar_height);
}

inline void scanButtons() {
  enter.tick();
  func.tick();
  left.tick();
  right.tick();
  up.tick();
  down.tick();

  /*
    Button::tick(enter);
    Button::tick(func);
    Button::tick(left);
    Button::tick(right);
    Button::tick(up);
    Button::tick(down);
  */
}


void loop() {
  //pinMode(ENA_PIN, OUTPUT);
  //digitalWrite(ENA_PIN, HIGH);

  scanButtons();
  //debugln(menu_ptr);
  switch(menu_ptr) {
    case SPEED: 
      speedMenu();
      break;
    
    case MAIN:
      mainMenu();
      break;
    
    case CYCLE:
      setupCycle();
      break;

    case SETTINGS:
      settings();
      break;

    case PROGRAM_SELECT:
      programming();
      break;

    case PROGRAM_SETUP:

      break;
    
    case SETUP_T_ACCEL:
      CYCLE_DATA.t_accel = setupTime(CYCLE_DATA.t_accel, (menu_ptr-6)%4);
      break;

    case SETUP_V: 
      CYCLE_DATA.v_const = setupNumbers(CYCLE_DATA.v_const, (menu_ptr-6)%4);
      break;
    
    case SETUP_T_WORK: 
      CYCLE_DATA.t_const = setupTime(CYCLE_DATA.t_const, (menu_ptr-6)%4);
      break;

    case SETUP_T_SLOWDOWN: 
      CYCLE_DATA.t_slowdown = setupTime(CYCLE_DATA.t_slowdown, (menu_ptr-6)%4);
      break;

    case SETUP_T_PAUSE: 
      CYCLE_DATA.t_pause = setupTime(CYCLE_DATA.t_pause, (menu_ptr-6)%4);
      break;

    case SETUP_N_CYCLES: 
      CYCLE_DATA.num_cycles = setupNumbers(CYCLE_DATA.num_cycles, (menu_ptr-6)%4);
      break;

    case SETUP_SMOOTHNESS:
      CYCLE_DATA.is_accel_smooth = setupAccel();
      break;

    case SETUP_DIR: 
      CYCLE_DATA.is_bidirectional = setupRepeat();
      break;

    case SETUP_SOUND:
      menu_ptr = SETTINGS;
      //need_to_load_interface = true;
      break;

    case SETUP_SAFE_STOP:
      menu_ptr = SETTINGS;
      //need_to_load_interface = true;
      break;

    case SEND_DATA:
      menu_ptr = SETTINGS;
      //need_to_load_interface = true;
      break;

    case RECIEVE_DATA:
      menu_ptr = SETTINGS;
      //need_to_load_interface = true;
      break;

      
  }

  if (refresh_screen) {
    refresh_screen = false;
    u8g2.updateDisplay();
  }
  // меню выбора программы
  // меню программирования 
  

  
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
  while (!enter.isClicked()) {
    enter.tick();
  }
  return;    

}