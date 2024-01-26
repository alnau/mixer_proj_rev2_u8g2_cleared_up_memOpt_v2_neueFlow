



#include "constants.h"
#include "libs_header.h"
#include "utils.h"
//#include "prog_utils.h"
#include "cycle_data.h"
#include "stepper.h"
#include "global_variables.h"
#include "io_constructor.h"


//#define FPSTR(pstr) (const __FlashStringHelper*)(pstr)



//#define IS_DEBUG
void printMenuLoadingScreen(const __FlashStringHelper* menu_name);


//==== Кнопки ======
// Заменил на EncButton, так что (теоретически) использование памяти должно сократиться со 138 до 30 байт
// #define EB_NO_BUFFER
// #define EB_NO_COUNTER
// #define EB_NO_CALLBACK
// #define EB_NO_FOR






//prog_data PROG_DATA(PROG_FIRST_BYTE);






//задаем дефолтные значения рабочего режима, записываем их в энергонезависимую память
//а также задаем стандартные "настройки" в первый байт EEPROM
void initData() {
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

  menu_ptr = SPEED;
  //PROG_DATA.load_data_to_buff(0,0);
}

void loadSettigsRegister() {
  uint8_t data_container = 0;
  data_container = eeprom_read_byte((uint8_t*)0);  //xxx(SAFE)(PROG)(SOUND)(EMERG)(INIT)
  safe_stop = (data_container >> 4) & 0b00000001;
  emergency_stop = (data_container >> 1) & 0b00000001;
  need_sound = (data_container >> 2) & 0b00000001;
}


void initDisplay() {
  //Wire.setClock(800000L);


  //u8g2.begin(/* menu_select_pin= */ ENTER_BTN, /* menu_next_pin= */ RIGHT_BTN, /* menu_prev_pin= */ LEFT_BTN, /* menu_up_pin= */ UP_BTN, /* menu_down_pin= */ DOWN_BTN, /* menu_home_pin= */ FUNC_BTN);
  u8g2.begin(/* menu_select_pin= */ ENTER_BTN, /* menu_next_pin= */ U8X8_PIN_NONE, /* menu_prev_pin= */ U8X8_PIN_NONE, /* menu_up_pin= */ U8X8_PIN_NONE, /* menu_down_pin= */ U8X8_PIN_NONE, /* menu_home_pin= */ FUNC_BTN);
  //u8g2.setContrast(0);
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_haxrcorp4089_t_cyrillic);

  printMenuLoadingScreen(F(" Загрузка..."));
}


void powerLoss() {
  
  if (is_working) {
    noInterrupts();

    pwr_loss = true;

    uint8_t eeprom_0x00 = eeprom_read_byte(0);
    //EEPROM.get(0, eeprom_0x00);
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
  //delay(100);
  pinMode(BUZZER, OUTPUT);
  initDisplay();
  initData();
  initStepper();
  initTimer1();
  checkEmergencyStop();
  menu_ptr = SPEED;
  need_to_load_interface = true;
  

  // подключили кнопку на D2 и GND
  pinMode(PWR_LOSS, INPUT); //Не обязательно, т.к. все пины по умолчанию работают как INPUT
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
    digitalWrite(ENA_PIN, LOW); //Разбудим двигатель


    // Выведем меню на экран
    u8g2.clear();
    u8g2.setCursor(0, 8);
    // TODO: перевести на userInterfaceMessage()
    u8g2.print(F("Контроллер был \n\r отключен во время \n\r работы \n\r Зажмите Enter чтобы \n\r продолжить"));
    //u8g2. ("Контроллер был", "отключен во время работы", "Зажмите [E] чтобы продолжить", "Ok"); //блокирующее. Будет тяжело мерцать и орать
    u8g2.updateDisplay();

    //необходимо подгрузить параметры последнего режима и выставить is_working, чтобы работа мешалки продолжалась по прерыванию таймера
    // working_in_programming_mode = (tmp >> 3) & 0b00000001;  // выставили режим, в котором происходит работа
    // restore_last_process();                                 //восстановили предыдущий процесс

    uint16_t last_update_time = (uint16_t)millis();
    bool is_lit = false;
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
        u8g2.setCursor(0, 8);
        u8g2.print(F(" Контроллер был \n\r отключен во время \n\r работы \n\r Зажмите Enter чтобы \n\r продолжить"));
        u8g2.updateDisplay();
        last_update_time = (uint16_t)millis();
      }
	    enter.tick();
      if (enter.isClicked()) {
	  //if (u8g2.getMenuEvent() == ENTER) {
        //u8g2.setContrast(0);
        //noTone(BUZZER);
        emergency_stop = false;
        uint8_t tmp = eeprom_read_byte((uint8_t*)0);
        tmp = tmp & 0b11111101;  //вернули второй бит в состояние 0
        eeprom_update_byte((uint8_t*)0, tmp);
        //mainMenu();
        return;
      }
    }
  }
}


//выводит разделы главного меню
// РЕНДЕРЕР
void printMainMenu() {
  for (int i = 0; i < MM_ITEMS; i++) {
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
  uint8_t tmp_ptr;

  if (need_to_load_interface) {
    main_menu_ptr = 0;
    tmp_ptr = 0;
    refresh_screen = true;
    u8g2.clear();
    u8g2.setCursor(0, 8);
    printMainMenu();
    printPtr(main_menu_ptr);
    //u8g2.updateDisplay();
    need_to_load_interface = false;
  }

  
  //основной процесс в меню
  enter.tick();
  func.tick();
  if (enter.isClicked()) {
  //if (u8g2.getMenuEvent() == ENTER) {
    //refresh_screen = true;
    need_to_load_interface = true;
    switch (main_menu_ptr) {
      case 0: menu_ptr = CYCLE; break;
      case 1: menu_ptr = PROGRAM_SELECT; break;
      case 2: menu_ptr = SETTINGS; break;
    }
  }
  else if(func.isClicked()) {
  //else if (u8g2.getMenuEvent() == FUNC) {
    menu_ptr = SPEED;
    need_to_load_interface = true;
    return;
  }

  tmp_ptr = main_menu_ptr;
  main_menu_ptr = upDown(main_menu_ptr, MM_ITEMS);

  if (tmp_ptr != main_menu_ptr) {
    //printMainMenu();
    printPtr(main_menu_ptr);
    refresh_screen = true;
    //u8g2.updateDisplay();
  }
  if (refresh_screen) {
    u8g2.clear();
    printMainMenu();
    printPtr(main_menu_ptr);
    //u8g2.updateDisplay();
    //refresh_screen = false;
  }

}


// //выводит предупреждение и спрашивает точно ли пользователь хочет остановить работу
// //РЕНДЕРЕР
// void printStopWarning() {
//   u8g2.clear();
//   u8g2.setCursor(0, 8);
//   u8g2.print(F("Зажмите левую и прав-ую стрелки одноврем-\n\rенно если действит-\n\rельно хотите \n\rостановить процесс"));
//   refresh_screen = true;
//   //u8g2.updateDisplay();
// }

//Выводит текущую скорость
//НЕ РЕНДЕРЕР
void printSpeedMenu(uint8_t speed, bool is_working) {
  //scale = 5 => w = 6*4 = 24
  const uint8_t start_row_y = 3;

  uint8_t order = calculateOrder(speed);
  u8g2.clear();
  //раньше использовал u8g2_font_6x12_t_cyrillic
  //u8g2.setFont(u8g2_font_haxrcorp4089_t_cyrillic);
  u8g2.setCursor(5, 18);
  if (is_working)
    u8g2.print(F("вкл"));
  else
    u8g2.print(F("выкл"));

  // if (srd.dir == CCW) {
  //   //считаем вращение против часовой отрицательным
  //   u8g2.drawBox(5 + (3 - order) * u8g2.getMaxCharWidth() + 8 - 20, 8 * start_row_y - 4 + 19, 7, 3);
  // }
  u8g2.setCursor(5 + (3 - order) * u8g2.getMaxCharWidth() + 8-10, 8 * start_row_y + 26);

  //был u8g2_font_inr24_t_cyrillic
  u8g2.setFont(u8g2_font_inr24_mn);
  u8g2.print(speed);
  u8g2.setFont(u8g2_font_haxrcorp4089_t_cyrillic);
  u8g2.drawRFrame(0, 8 * start_row_y - 4, 126, 36, FRAME_RADIUS);
  u8g2.setCursor(75 + 5 + 8-10, 32 + 8 * start_row_y - 6);
  u8g2.print(F("об/мин"));
  //oled.update();
}





void speed_menu() {

  uint8_t speed; 
  static uint8_t prev_speed;
  static uint16_t t_since_last_update;

  //если только зашли после пред. менюшки
  if (need_to_load_interface) {
    t_since_last_update = (uint16_t)millis();
    speed = 0;
    refresh_screen = true;

    if (is_working and (srd.run_state != PAUSE)) {
      // w = ALPHA*f/delay[рад/с]
      // RPM = w/(2*pi)*60 [об/мин]
      speed = (uint8_t)((30*ALPHA*T1_FREQ)/(3.14159*srd.step_delay));

      if (prev_speed != speed) {
        prev_speed = speed;
        refresh_screen = true;
      }  
      need_to_load_interface = false;
    }

    printSpeedMenu(speed, is_working);
    //u8g2.updateDisplay();
  }

  
  //обработка меню при вызове
  if (is_working and (srd.run_state != PAUSE)) {
    // w = ALPHA*f/delay[рад/с]
    // RPM = w/(2*pi)*60 [об/мин]
    speed = (uint8_t)((30*ALPHA*T1_FREQ)/(3.14159*srd.step_delay));

    if ((prev_speed != speed) and ((uint16_t)millis() - t_since_last_update > 200)) {
      prev_speed = speed;
      refresh_screen = true;
      t_since_last_update = (uint16_t)millis();
    }   
  }
  enter.tick();
  if (enter.isClicked() and !is_working) {
  //if ((u8g2.getMenuEvent() == ENTER) and !is_working) {

    if (!need_to_stop) {
      // if (working_in_programming_mode) {
      //   //проверили что не идет работа в режиме программирования
      //   u8g2.clear();
      //   u8g2.print(F("Запуск невозможен\n\r"));
      //   u8g2.print(F("Уже выполняется работа по\n\r"));
      //   u8g2.print(F("программе"));
      //   return;
      // }
      //если не работали и нажали Enter, начинаем работу
      is_working = true;
      startMotor();
    }
    //enter.clear();
  }
  enter.tick();
  func.tick();
  if (enter.isClicked()) {
  //if (u8g2.getMenuEvent() == ENTER) {
    if (is_working and !need_to_stop) {
      //Если двигатель работал и пользователю необходимо его остановить
      need_to_stop = true;  //Установим флаг, который будет обрабатываться в прерывании motor_cntr.c
      refresh_screen = true;
    }
    //enter.clear();
  }
  else if (func.isClicked()) {
  //else if (u8g2.getMenuEvent() == FUNC) {
    //если пользователь нажал на настройку,выкидываем его в главное меню, при этом продолжая работу в прерываниях
    //mainMenu();
    menu_ptr = MAIN;
    need_to_load_interface = true;
    refresh_screen = true;
    return;
  }
}





//выводит время в формате M:SS у правого края экрана
// Если выставлен is_setup, то подчеркнет цифру
//с порядковым номером digit
//РЕНДЕРЕР (НО МОЖНО ОПТИМИЗИРОВАТЬ)
void printTime(uint16_t T, uint8_t ptr, bool is_setup, uint8_t digit) {

  const uint8_t MIN = 1;
  const uint8_t SEC1 = 2;
  const uint8_t SEC2 = 3;

  uint8_t m = T/60;
  uint8_t s1 = (T % 60) / 10;
  uint8_t s2 = (T % 60) % 10;

  char m_c[2];
  char s1_c[2];
  char s2_c[2];

  itoa(m, m_c, 10);     //переведем инт в строку (десятичное представление)
  itoa(s1, s1_c, 10);
  itoa(s2, s2_c, 10);

  uint8_t x_0 = SCREEN_WIDTH - 4 * 6 - DATA_X_BIAS + 5;
  uint8_t y_0 = 4 + ptr * 16 + 8;

  uint8_t m_width = u8g2.getStrWidth(m_c);
  uint8_t s1_width = u8g2.getStrWidth(s1_c);
  uint8_t s2_width = u8g2.getStrWidth(s2_c);
  uint8_t separator_width = u8g2.getStrWidth(":");

  //u8g2.clearDisplay();
  u8g2.setDrawColor(0);
  u8g2.drawBox(x_0, y_0-8, 20,11);
  //u8g2.drawBox(x_0,y_0, 20, 3);
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
}


//перегрузка без is_setup. Внутренности почти аналогичны, не считая отсутствие условия на is_setup
// РЕНДЕРЕР
void printTime(uint16_t T, uint8_t ptr) {

  uint8_t m = T/60;
  uint8_t s1 = (T % 60) / 10;
  uint8_t s2 = (T % 60) % 10;

  uint8_t x_0 = SCREEN_WIDTH - 4 * 6 - DATA_X_BIAS + 5;
  uint8_t y_0 = 4 + ptr * 16 + 8;



  //u8g2.clearDisplay();
  u8g2.setDrawColor(0);
  u8g2.drawBox(x_0, y_0-8, 20,11);
  //u8g2.drawBox(x_0,y_0, 20, 3);
  u8g2.setDrawColor(1);

  u8g2.setCursor(x_0, y_0);
  u8g2.print(m);
  u8g2.print(F(":"));
  u8g2.print(s1);
  u8g2.print(s2);
}


//режим установки времени работы
// НЕ РЕНДЕРЕР
uint16_t setupTime(uint16_t T, uint8_t ptr) {

  static uint8_t digit;
  static uint8_t tmp_digit;
  uint16_t tmp;
  
  if (need_to_load_interface) {
    digit = 1;
    tmp_digit = digit;
    tmp = T;
    need_to_load_interface = false;
    printTime(tmp, ptr, true, digit);
    refresh_screen = true;
    //u8g2.updateDisplay();
  }



  enter.tick();
  if (enter.isClicked()) {
  //if (u8g2.getMenuEvent() ==ENTER) {
    printTime(tmp, ptr, false, 0);
    //u8g2.updateDisplay();
    //T = tmp;
    refresh_screen = true;
    menu_ptr = CYCLE;
    need_to_load_interface = true;
    return tmp;
  }

  tmp_digit = digit;
  digit = leftRight(digit);
  if (digit != tmp_digit) {
    printTime(tmp, ptr, true, digit);
    refresh_screen = true;
    //u8g2.updateDisplay();
  }
  up.tick();
  down.tick();
  if (up.isClicked()) {
  //if (u8g2.getMenuEvent() == UP) {

    //если минуты
    if (digit == 1) {
      tmp = constrain(tmp + 60, 0, MAX_TIME);
    }
    //десятки секунд
    if (digit == 2) {
      tmp = constrain(tmp + 10, 0, MAX_TIME);
    }
    //и секунды
    if (digit == 3) {
      tmp = constrain(tmp + 1, 0, 255);
    }
    printTime(tmp, ptr, true, digit);
    refresh_screen = true;
    //u8g2.updateDisplay();
  }
  else if (down.isClicked()) {
  //else if (u8g2.getMenuEvent() == DOWN) {
    //printTime(tmp, ptr, true, digit);
    //если минуты
    if (digit == 1) {
      tmp = constrain(tmp - 60, 0, MAX_TIME);
    }
    if (digit == 2) {
      tmp = constrain(tmp - 10, 0, MAX_TIME);
    }
    if (digit == 3) {
      tmp = constrain(tmp - 1, 0, MAX_TIME);
    }
    printTime(tmp, ptr, true, digit);
    //u8g2.updateDisplay();
    refresh_screen = true;
  }
  return tmp;
  
}



//Вывод числа в информационной колонке. Ожидает не больше трехзначных
// РЕНДЕРЕР
void printNumbers(uint8_t data, uint8_t ptr) {

  uint8_t order = calculateOrder(data);  //рассчет порядка числа

  uint8_t x_0 = SCREEN_WIDTH - 3 * 6 - DATA_X_BIAS;
  uint8_t y_0 = (2 * ptr + 1) * 8 + 5;

  u8g2.setDrawColor(0);
  u8g2.drawBox(x_0, y_0-8, 20,11);
  u8g2.setDrawColor(1);
  u8g2.setCursor(x_0, y_0);
  if (order < 3) {
    for (int i = 1; i <= 3 - order; i++)
      u8g2.print(F("0"));  //заполняем нулями числа меньше сотен, чтобы было можно выставлять старшие порядки
  }
  u8g2.print(data);
}

//перегрузка для установки чисел. если выставлен is_setup, то рисует черту под digit порядком
// РЕНДЕРЕР (НО МОЖЕМ ОПТИМИЗИРОВАТЬ)
void printNumbers(uint8_t data, uint8_t ptr, bool is_setup, uint8_t digit) {

  uint8_t order = calculateOrder(data);  //рассчет порядка числа

  uint8_t x_0 = SCREEN_WIDTH - 3 * 6 - DATA_X_BIAS;
  uint8_t y_0 = (2 * ptr + 1) * 8 + 5;
  
  u8g2.setDrawColor(0);
  u8g2.drawBox(x_0, y_0-8, 20,11);
  u8g2.setDrawColor(1);
  u8g2.setCursor(x_0, y_0);
  if (order < 3) {
    for (int i = 1; i <= 3 - order; i++)
      u8g2.print(F("0"));  //заполняем нулями числа меньше сотен, чтобы было можно выставлять старшие порядки
  }
  u8g2.print(data);

  if (is_setup) {
    const uint8_t HUNDREDS = 1;
    const uint8_t TENS = 2;
    const uint8_t ONES = 3;

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
    //Serial.println(data);
    //Serial.println(h_width);
    //Serial.println(t_width);
    //Serial.println(o_width);

    switch(digit) {
      case HUNDREDS:
        u8g2.drawHLine(x_0, y_0+1, h_width);
        break;
      case TENS: 
        u8g2.drawHLine(x_0 + h_width + 1, y_0 + 1, t_width);
        break;
      case ONES: 
        u8g2.drawHLine(x_0 + h_width + t_width + 2, y_0 + 1, o_width);
        break;
    }
  }
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

//Вывод текста в информационной колонке, ожидает трехбуквенный
// РЕНДЕРЕР
void printText(const __FlashStringHelper* text, uint8_t ptr) {
  u8g2.drawHLine(SCREEN_WIDTH - 3 * 6 - DATA_X_BIAS, (2 * ptr + 1) * 8 + 5, 16);
  u8g2.setCursor(SCREEN_WIDTH - 3 * 6 - DATA_X_BIAS, 4 + ptr * 16 + 8);
  u8g2.print(text);
}

//перегрузка на случай любой (в разумных пределах) длины
// РЕНДЕРЕР
void printText(const __FlashStringHelper* text, uint8_t ptr, uint8_t len) {
  u8g2.setDrawColor(0);
  u8g2.drawBox(SCREEN_WIDTH - 3 * 6 - DATA_X_BIAS, (2 * ptr + 1) * 8 + 5, 16 + DATA_X_BIAS, 1);
  u8g2.setDrawColor(1);
  u8g2.setCursor(SCREEN_WIDTH - len * 6 - DATA_X_BIAS, 4 + ptr * 16 + 8);
  u8g2.print(text);
}


//цикл установки чисел в информационной колонке
// НЕ РЕНДЕРЕР
uint8_t setupNumbers(uint8_t data, uint8_t ptr) {
  static uint8_t digit;
  static uint8_t tmp;
  static uint8_t tmp_digit;
  //uint8_t order = calculateOrder(data);

  if (need_to_load_interface) {
    digit = 1;
    tmp = data; 
    tmp_digit = digit;
    printNumbers(tmp, ptr, true, digit);
    //u8g2.updateDisplay();
    refresh_screen = true;
    need_to_load_interface = false;
  }

  

    enter.tick();
    if (enter.isClicked())  {
    //if (u8g2.getMenuEvent() == ENTER) {
      printNumbers(tmp, ptr);
      //u8g2.updateDisplay();
      refresh_screen = true;
      data = tmp;
      need_to_load_interface = true;
      menu_ptr = CYCLE;
      return tmp;
    }

    tmp_digit = digit;
    digit = leftRight(digit);
    if (tmp_digit != digit) {
      printNumbers(tmp, ptr, true, digit);
      refresh_screen = true;
      //u8g2.updateDisplay();
    }

    printNumbers(tmp, ptr, true, digit);
    
    up.tick();
    down.tick();
    if (up.isClicked()) {
    //if (u8g2.getMenuEvent() == UP) {
      //если сотни
      if (digit == 1) {
        tmp = constrain(tmp + 100, 0, MAX_NUM);
      } else if (digit == 2) {
        tmp = constrain(tmp + 10, 0, MAX_NUM);
      } else {
        tmp = constrain(tmp + 1, 0, MAX_NUM);
      }
      printNumbers(tmp, ptr, true, digit);
      refresh_screen = true;
      //u8g2.updateDisplay();
    } else if (down.isClicked()) {
    //else if (u8g2.getMenuEvent() == DOWN) {
      //если сотни
      if (digit == 1) {
        tmp = constrain(tmp - 100, 0, MAX_NUM);
      } else if (digit == 2) {
        tmp = constrain(tmp - 10, 0, MAX_NUM);
      } else {
        tmp = constrain(tmp - 1, 0, MAX_NUM);
      }
      printNumbers(tmp, ptr, true, digit);
      refresh_screen = true;
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

  uint8_t len = (strlen(text) + 1) / 2;
  if (len >= TEXT_MAX_LEN) {
    if ((uint16_t)millis() - prev_scroll_time > SCROLL_PERIOD) {
      //char tmp[strlen(text)+1];

      uint8_t i = counter % len;
      //tmp = substring(text, i, i + 2 * TEXT_MAX_LEN);
      clearMenuItem(cursor);
      u8g2.setCursor(8, 4 + cursor * 16 + 8);
      u8g2.print(substring(text, i, i + 2 * TEXT_MAX_LEN));
      //delay(1000 / SCROLL_FREQ);
      prev_scroll_time = (uint16_t)millis();
      refresh_screen = true;
      //u8g2.updateDisplay();
      return counter + 1;
    } else
      return counter;
  } else if (len < TEXT_MAX_LEN) {
    clearMenuItem(cursor);
    u8g2.setCursor(8, 4 + cursor * 16 + 8);
    u8g2.print(text);
    return counter;
  }
  else  
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

      u8g2.print(substring(buffer, 0, 2 * TEXT_MAX_LEN));
    }
  } else {
    for (uint8_t i = 0; i < 4; i++) {
      u8g2.setCursor(8, 4 + i * 16 + 8);

      PGM_P pstr = pgm_read_word(setup_menu_items + i + 4);
      char buffer[strlen_P(pstr)+1];
      strcpy_P(buffer, pstr);

      u8g2.print(substring(buffer, 0, 2 * TEXT_MAX_LEN));
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
  u8g2.print(substring(item, 0, 2 * TEXT_MAX_LEN));
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
    printTime(CYCLE_DATA.t_pause, 4 % 4);
    printNumbers(CYCLE_DATA.num_cycles, 5 % 4);
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
    refresh_screen = true;
    //u8g2.updateDisplay();
    need_to_load_interface = false;
    tmp = CYCLE_DATA.is_accel_smooth;
  }
  
  

    enter.tick();
    up.tick();
    down.tick();

    
    if (enter.isClicked())  {
    //if (u8g2.getMenuEvent() == ENTER) {
      printAccelRegime(tmp, false);
      refresh_screen = true;
      //u8g2.updateDisplay();
      need_to_load_interface = true;
      menu_ptr = CYCLE;
    }
    else if (up.isClicked() or down.isClicked())  {
    //else if ((u8g2.getMenuEvent() == UP) or (u8g2.getMenuEvent() == DOWN)) {
      tmp = !tmp;
      printAccelRegime(tmp, true);
      refresh_screen = true;
      //u8g2.updateDisplay();
    }
    return tmp;
  
}

//установка режима повторения
// НЕ РЕНДЕРЕР
bool setupRepeat() {
  static bool tmp;

  if (need_to_load_interface) {
    printCycleRegime(CYCLE_DATA.is_bidirectional, true);
    refresh_screen = true;
    //u8g2.updateDisplay();
    tmp = CYCLE_DATA.is_bidirectional;
    need_to_load_interface = false; 
  }

    enter.tick();
    up.tick();
    down.tick();
    if (enter.isClicked()) {
    //if (u8g2.getMenuEvent() == ENTER) {
      need_to_load_interface = true;
      printCycleRegime(tmp, false);
      refresh_screen = true;
      //u8g2.updateDisplay();
      menu_ptr = CYCLE;
      return tmp;
    }
    else if (up.isClicked() or down.isClicked()) {
    //else if ((u8g2.getMenuEvent() == UP) or (u8g2.getMenuEvent() == DOWN)) {
      tmp = !tmp;
      printCycleRegime(tmp, true);
      refresh_screen = true;
      //u8g2.updateDisplay();
    }
  
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
  delay(LOADING_TIME/2);

  u8g2.clear();
  u8g2.setCursor(0, 8);
}

//функция, вывода и обработки меню режима вращения
// НЕ РЕНДЕРЕР
void setupCycle() {

  static uint8_t setup_ptr;
  static uint8_t curr_page;
  static uint8_t counter;  //переменная, считающая циклы, чтобы передавать ее в функцию бегущего текста
  uint8_t tmp_ptr;
  bool need_update;
  
  
  if (need_to_load_interface) {
    setup_ptr = 0;
    curr_page = 0;
    counter = 1;
    setup_ptr = 0;
    need_update = false;
    refresh_screen = true;

    // TODO: Это проблематичное место, нужно переписать
    if (is_working) {
      u8g2.userInterfaceMessage("Устройство уже за-", "пущено. Изменение", "установок невозможно", "Ok");
      u8g2.clear();
      
    } else {
      printMenuLoadingScreen(F(" Режим"));
    }

    printSetupPages(0);
    printPtr(setup_ptr);
    printSetupData(0);

    //u8g2.updateDisplay();
    refresh_screen = true;
    need_to_load_interface = false;
  }
  

  // Основной процесс рендеринга менюшки
  enter.tick();
  func.tick();
  if (func.isClicked()) {
  //if (u8g2.getMenuEvent() == FUNC) {
    need_to_load_interface = true;
    menu_ptr = MAIN;
    return;
  }
  else if (enter.isClicked() and !is_working) {
  //else if ((u8g2.getMenuEvent() == ENTER) and !is_working) {
    need_update = true;
    need_to_load_interface = true;
    // switch (setup_ptr) {
    //   case 0: CYCLE_DATA.t_accel = setupTime(CYCLE_DATA.t_accel, setup_ptr % 4); break;
    //   case 1: CYCLE_DATA.v_const = setupNumbers(CYCLE_DATA.v_const, setup_ptr % 4); break;
    //   case 2: CYCLE_DATA.t_const = setupTime(CYCLE_DATA.t_const, setup_ptr % 4); break;
    //   case 3: CYCLE_DATA.t_slowdown = setupTime(CYCLE_DATA.t_slowdown, setup_ptr % 4); break;
    //   case 4: CYCLE_DATA.t_pause = setupTime(CYCLE_DATA.t_pause, setup_ptr % 4); break;
    //   case 5: CYCLE_DATA.num_cycles = setupNumbers(CYCLE_DATA.num_cycles, setup_ptr % 4); break;
    //   case 6: CYCLE_DATA.is_accel_smooth = setupAccel(); break;
    //   case 7: CYCLE_DATA.is_bidirectional = setupRepeat(); break;
    // }
    /*
     * Тут нужно пояснение
     * хитрый хак в стиле switch. Все литралы сетапов равны 6..13 (см constants.h), соответственно, в данной
     * реализации прибавление setup_ptr к шестерки автоматически вычисляет нужный литерал
     * всго за один такт 
     
    */
    menu_ptr = 6 + setup_ptr; 
  
  }

  //если требуется обновить энергонезависимую память
  if (need_update) {
    CYCLE_DATA.writeDataToMem();
    need_update = false;
  }

  tmp_ptr = setup_ptr;
  setup_ptr = upDown(setup_ptr, SETUP_ITEMS);
  if (tmp_ptr != setup_ptr) {
    //отловили момент перехода к следующему элементу
    counter = 0;
    refresh_screen = true;
  }

  //если нужно перейти с первой на вторую на вторую страницу
  if ((setup_ptr >= 4) && (curr_page == 0)) {
    u8g2.clear();
    printSetupPages(7);

    PGM_P pstr = pgm_read_word(setup_menu_items + setup_ptr);
    char buffer[strlen_P(pstr)+1];
    strcpy_P(buffer, pstr);

    counter = scrollText(buffer, setup_ptr % 4, counter);

    printSetupData(7);
    curr_page = 1;
    refresh_screen = true;
    //u8g2.updateDisplay();
  } else if ((setup_ptr < 4) && (curr_page == 1)) {
    //если переход со второ на первую
    u8g2.clear();
    printSetupPages(0);

    PGM_P pstr = pgm_read_word(setup_menu_items + setup_ptr);
    char buffer[strlen_P(pstr)+1];
    strcpy_P(buffer, pstr);

    counter = scrollText(buffer, setup_ptr % 4, counter);

    printSetupData(0);
    curr_page = 0;
    refresh_screen = true;
    //u8g2.updateDisplay();
  } else {
    //если остались на той-же
    PGM_P pstr = pgm_read_word(setup_menu_items + setup_ptr);
    char buffer[strlen_P(pstr)+1];
    strcpy_P(buffer, pstr);

    counter = scrollText(buffer, setup_ptr % 4, counter);
    if (refresh_screen) {
      clearMenuItem(tmp_ptr);
      PGM_P pstr = pgm_read_word(setup_menu_items + tmp_ptr);
      char buffer[strlen_P(pstr)+1];
      strcpy_P(buffer, pstr);

      refreshMenuItem(buffer, tmp_ptr);  //вернули текст эл-та который только что скролили в нормальное состояние
      
      //u8g2.updateDisplay();
      //refresh_screen = true;
    }
  }
  //counter = scrollText(buffer, setup_ptr % 4, counter);
  printPtr(setup_ptr%4);
  printScrollBar(setup_ptr, SETUP_ITEMS);

}


//меню режима программирования
void programming() {

  while(!u8g2.userInterfaceMessage("Пока не реализо-", "вано.", "Работа ведется.", "Ok")) {
    ;
  }
  need_to_load_interface = true;
  refresh_screen = true;
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
    for (int i = 0; i < 4; i++) {
      PGM_P pstr = pgm_read_word(settings_items + i);
      char buffer[strlen_P(pstr)+1];
      strcpy_P(buffer, pstr);

      u8g2.setCursor(8, 4 + 16 * i + 8);
      u8g2.print(substring(buffer, 0, 2 * TEXT_MAX_LEN));
    }
  } else {
    for (int i = 4; i < 8; i++) {
      PGM_P pstr = pgm_read_word(settings_items + i);
      char buffer[strlen_P(pstr)+1];
      strcpy_P(buffer, pstr);

      u8g2.setCursor(8, 4 + 16 * (i - 4) + 8);
      u8g2.print(substring(buffer, 0, 2 * TEXT_MAX_LEN));
    }
  }
}





//bool need_sound = true;
// НЕ РЕНДЕРЕР
void settings() {

  static uint8_t settings_cursor;
  uint8_t counter;  //переменная, считающая циклы, чтобы передавать ее в функцию бегущего текста
  uint8_t tmp_ptr;

  if (need_to_load_interface) {
    settings_cursor = 0;
    counter = 0;
    tmp_ptr = 0;
    printMenuLoadingScreen(F(" Hacтройки"));

    printSettingsPages(settings_cursor);
    printSettingsData(settings_cursor);
    printPtr(settings_cursor);
    //u8g2.updateDisplay();
    refresh_screen = true;
    need_to_load_interface = false;
  }

  func.tick();
  enter.tick();
  if (func.isClicked()) {
  //if (u8g2.getMenuEvent() == FUNC) {
    need_to_load_interface = true;
    menu_ptr = MAIN;
    return;
  }
  else if (enter.isClicked()) {
  //else if (u8g2.getMenuEvent() == ENTER) {
    refresh_screen = true;
    need_to_load_interface = true;
    switch (settings_cursor) {
      // case 0: need_sound = setupSound(); break;
      // case 1: safe_stop = setupSafeStop(); break;
      menu_ptr = settings_cursor + 14;

      case 2: debugln(F("Данные1")); break;
      case 3: debugln(F("Данные2")); break;

    }
  }

  if (refresh_screen) {
    refreshSettings();
  }

  tmp_ptr = settings_cursor;
  settings_cursor = upDown(settings_cursor, SETTINGS_ITEMS);
  if ((tmp_ptr == 4) and (settings_cursor == 3)) {
    //переход со второй страницы на первую
    u8g2.clear();

    debugln(F("2->1"));
    printSettingsPages(settings_cursor);
    printSettingsData(settings_cursor);
    printPtr(settings_cursor % 4);
    printScrollBar(settings_cursor, SETTINGS_ITEMS);
    refresh_screen = true;
    //u8g2.updateDisplay();
  } else if ((tmp_ptr == 3) and (settings_cursor == 4)) {
    //переход с первой на вторую
    u8g2.clear();

    debugln(F("1->2"));

    printSettingsPages(settings_cursor);
    printSettingsData(settings_cursor);
    //printPtr(settings_cursor % 4);
    //printScrollBar(settings_cursor, SETTINGS_ITEMS);
    refresh_screen = true;
    //u8g2.updateDisplay();
  } else if (tmp_ptr != settings_cursor) {
    counter = 0;
    clearMenuItem(tmp_ptr);

    PGM_P pstr = pgm_read_word(settings_items + tmp_ptr);
    char buffer[strlen_P(pstr)+1];
    strcpy_P(buffer, pstr);

    refreshMenuItem(buffer, tmp_ptr);
    //printPtr(settings_cursor % 4);
    //printScrollBar(settings_cursor, SETTINGS_ITEMS);
    refresh_screen = true;
    //u8g2.updateDisplay();
  } else {
    PGM_P pstr = pgm_read_word(settings_items + settings_cursor);
    char buffer[strlen_P(pstr)+1];
    strcpy_P(buffer, pstr);
  
    counter = scrollText(buffer, settings_cursor % 4, counter);
  }

  
  printPtr(settings_cursor % 4);
  printScrollBar(settings_cursor, SETTINGS_ITEMS);
  //refresh_screen = true;
  
}

// РЕНДЕРЕР
void printSafeStopStatus(bool state, bool is_setup) {
  uint8_t ptr = 1;
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
void printSoundStatus(bool state, bool is_setup) {
  uint8_t ptr = 0;
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
}

//вывод данных настроек в информационную колонку
// РЕНДЕРЕР
void printSettingsData(uint8_t settings_ptr) {
  printSoundStatus(need_sound, false);
  printSafeStopStatus(safe_stop, false);
  printText(F("TODO"), 3, 4);
  printText(F("TODO"), 0, 4);
}



//обновение настроек, записанных в энергонезависимой памяти
void refreshSettings() {
  uint8_t tmp = eeprom_read_byte((uint8_t*)0);

  debug(tmp);
  debug(F(" --> "));

  bitWrite(tmp, 2, need_sound);
  bitWrite(tmp, 5, safe_stop);
  eeprom_update_byte((uint8_t*)0, tmp);
  debugln(tmp);
  debugln(F("Settings updated"));
}

// НЕ РЕНДЕРЕР
bool setupSafeStop() {
  static bool tmp_safe_stop;

  if (need_to_load_interface) {
    tmp_safe_stop = safe_stop;
    printSafeStopStatus(tmp_safe_stop, true);
    //u8g2.updateDisplay();
    refresh_screen = true;
    need_to_load_interface = false;
  }



    enter.tick();
    up.tick();
    down.tick();
    if (enter.isClicked()) {
    //if (u8g2.getMenuEvent()==ENTER ) {
      printSafeStopStatus(tmp_safe_stop, false);
      refresh_screen = true;
      //u8g2.updateDisplay();
      need_to_load_interface = true;
      menu_ptr = SETTINGS;
      return tmp_safe_stop;
    }
    else if(up.isClicked() or down.isClicked()) {
    //else if ((u8g2.getMenuEvent() == UP) or (u8g2.getMenuEvent() == DOWN)) {
      tmp_safe_stop = !tmp_safe_stop;
      printSafeStopStatus(tmp_safe_stop, true);
      refresh_screen = true;
      //u8g2.updateDisplay();
    }
  return tmp_safe_stop;
}


//установка громкости
// НЕ РЕНДЕРЕР
bool setupSound() {
  static bool tmp_need_sound;

  if (need_to_load_interface) {
    tmp_need_sound = need_sound;
    printSoundStatus(tmp_need_sound, true);
    refresh_screen = true;
    //u8g2.updateDisplay();
    need_to_load_interface = false;
  }


    enter.tick();
    up.tick();
    down.tick();
    if (enter.isClicked()) {
    //if (u8g2.getMenuEvent() == ENTER) {
      printSoundStatus(tmp_need_sound, false);
      refresh_screen = true;
      //u8g2.updateDisplay();
      menu_ptr = SETTINGS;
      need_to_load_interface = true;
      return tmp_need_sound;
    }
    else if (up.isClicked() or down.isClicked()) {
    //else if ((u8g2.getMenuEvent() == UP) or (u8g2.getMenuEvent() == DOWN)) {
      tmp_need_sound = !tmp_need_sound;
      printSoundStatus(tmp_need_sound, true);
      refresh_screen = true;
      //u8g2.updateDisplay();
    }
    return tmp_need_sound;
  
}

//отрисовка курсора в менющке из восьми элементов
// РЕНДЕРЕР
void printPtr(uint8_t ptr) {
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, 4, SCREEN_HEIGHT);
  u8g2.setDrawColor(1);
  u8g2.setCursor(0, ptr * 16 + 4 + 8);
  u8g2.print(F(">"));
}


// РЕНДЕРЕР
void printScrollBar(uint8_t ptr, uint8_t num_items) {
  uint8_t bar_height = (SCREEN_HEIGHT - 8) / num_items;
  u8g2.setDrawColor(0);
  u8g2.drawBox(SCREEN_WIDTH - 3, 0, 3, SCREEN_HEIGHT);
  u8g2.setDrawColor(1);
  u8g2.drawVLine(SCREEN_WIDTH - 1, 4 + bar_height * ptr, bar_height);
}




void loop() {
  //Serial.println(USER_DECEL); //9.549 = 60/(2*PI)
  Serial.println(menu_ptr);

  switch(menu_ptr) {
    case SPEED: 
      speed_menu();
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
      
      break;

    case SETUP_SAFE_STOP:

      break;

    case SEND_DATA:
      menu_ptr = SETTINGS;
      need_to_load_interface = true;
      break;

    case RECIEVE_DATA:
      menu_ptr = SETTINGS;
      need_to_load_interface = true;
      break;

      
  }

  if (refresh_screen) {
    refresh_screen = false;
    u8g2.updateDisplay();
  }
  // меню выбора программы
  // меню программирования 
  

  
}