#include "stepper.h"

//! Структура, содержащая данные, используемые в преывании
speedRampData srd;
//! Используется как буффер при подгрузке данных из памяти. Нет необходимости, дубликат




/*! \brief Функция сводящая необходимые мне параметры на язык AVR446
 *
 * Пересчитывает велечины из об/мин в 0.01 рад/с. Также рассчитывает ускорение, 
 * Замдление и кол-во шагов. 
*/
void startMotor() {

  srd.dir = CCW;
  
  /*md_LoadDataFromMem();
  
  uint16_t speed_100 = 10*3.14*md.mem_speed/3; //(2*3.14*100*RPM)/(60);
  int32_t step = (int32_t)md.mem_speed*(int32_t)SPR*(int32_t)(md.mem_t_run + (md.mem_t_decel + md.mem_t_accel)/2)/60; 
  uint16_t accel = speed_100/md.mem_t_accel ;
  uint16_t  decel = speed_100 / md.mem_t_decel;

  

  Serial.println(F("===== Engine started ====="));
  Serial.print(F("t_accel = ")); Serial.println(md.mem_t_accel);
  Serial.print(F("t_run = ")); Serial.println(md.mem_t_run);
  Serial.print(F("t_decel = ")); Serial.println(md.mem_t_decel);
  Serial.print(F("t_pause = ")); Serial.println(md.mem_t_pause);
  Serial.print(F("speed = ")); Serial.print(md.mem_speed); Serial.println(F(" RPM"));
  Serial.print(F("number of cycles = ")); Serial.println(md.mem_n_cycles);
  Serial.print(F("Rotation regime: "));  (md.mem_is_bidir == true) ? Serial.println(F("Bidirectional")) : Serial.println(F("Unidirectional" ));
  */

  
  
  uint16_t speed_100 = 10*PI*CYCLE_DATA.v_const/3; //(2*3.14*100*RPM)/(60);
  int32_t step = (int32_t)CYCLE_DATA.v_const*(int32_t)SPR*(int32_t)(CYCLE_DATA.t_const + (CYCLE_DATA.t_slowdown + CYCLE_DATA.t_accel)/2)/60; 
  uint16_t accel = speed_100/CYCLE_DATA.t_accel ;
  uint16_t  decel = speed_100 / CYCLE_DATA.t_slowdown;

  
  #ifdef IS_DEBUG
  Serial.println(F("===== Engine started ====="));
  Serial.print(F("t_accel = ")); Serial.println(cycle_data.t_accel);
  Serial.print(F("t_run = ")); Serial.println(cycle_data.t_const);
  Serial.print(F("t_decel = ")); Serial.println(cycle_data.t_slowdown);
  Serial.print(F("t_pause = ")); Serial.println(cycle_data.t_pause);
  Serial.print(F("speed = ")); Serial.print(cycle_data.v_const); Serial.println(F(" RPM"));
  Serial.print(F("number of cycles = ")); Serial.println(cycle_data.num_cycles);
  Serial.print(F("Rotation regime: "));  (cycle_data.is_bidirectional) ? Serial.println(F("Bidirectional")) : Serial.println(F("Unidirectional" ));

  Serial.println(F(""));Serial.print(F(""));
  #endif
  
  move(step, accel, decel, speed_100, CYCLE_DATA.t_pause, CYCLE_DATA.num_cycles, CYCLE_DATA.is_bidirectional);
}


/*! \brief Инициализирует режим работы с заданным числом шагов.
 *
 * 
 * Шаговый мотор делает заданное число шагов, реализуя рампу с заданным ускорением,
 * замедлением и скоростью. Если ускорение/замедление слишком мало и шагов недостаточно
 * для достижения необходимой скорости, торможение начнется до достижения "полки"
 *
 * \param step - число шагов
 * \param accel - величина ускорения, в 0.01 рад/с^2
 * \param deccel - величина ускорения, в 0.01 рад/с^2
 * \param speed - Необходимая скорость в 0.01 рад/с
 * \param t_pause - время паузы в секундах
 * \param n_cyles - число повторений циклов рампы
 * \param is_bidir - булево значение (true -> двустороннее вращение, false -> одностороннее)
*/
void move(int32_t step, uint16_t accel, uint16_t decel, uint16_t speed, uint16_t t_pause, uint16_t n_cycles, bool is_bidir) {

  //! Число шагов до того, как мы достигнем макс скорости
  uint32_t max_s_lim;
  //! число шагов до начала торможения (если не успеем достичь макс скорости) TODO: НЕ НУЖНО, УБРАТЬ
  uint32_t accel_lim;

  srd.n_cycles = n_cycles;  //И кол-во циклов
  srd.accel = accel;        //Сохранение ускорения для пересчета новых циклов
  srd.decel = decel;
  srd.is_bidir = is_bidir;  //Запомнили одно- или двухсторотнее вращение
  srd.t_pause = t_pause;     //Сохраним время паузы   
  srd.speed = speed;

  

  // Выставим направление вращения, основываясь на знаке числа шагов
  if (step < 0) {
    srd.dir = CCW;
    step = -step;
  } else {
    srd.dir = CW;
  }

  // Двигаться  оько если число шагов не равно нулю
  if (step != 0) {
    // Ввести верхний предел скорости, рассчитав минимальную паузу
    // min_delay = (alpha / tt)/ w
    srd.min_delay = A_T_x100 / speed;

    // Находим паузу до первого шага (c_0) для того, чтобы задать ускорение
    // step_delay = 1/tt * sqrt(2*alpha/accel)
    // step_delay = ( tfreq*0.676/100 )*100 * sqrt( (2*alpha*10000000000) / (accel*100) )/10000
    srd.step_delay = (T1_FREQ_148 * m_sqrt(A_SQ / accel)) / 100;

    // найдем через сколько шагов мы достигнем макс. скорости
    // max_s_lim = speed^2 / (2*alpha*accel)
    max_s_lim = (long)speed * speed / (long)(((long)A_x20000 * accel) / 100);
    //если по какой-то причине мы получим 0, то все равно делаем хоть один шаг
    if (max_s_lim == 0) {
      max_s_lim = 1;
    }

    /* 
     *Найдем через сколько шагов нам нужно было-бы начать торможение, если бы нам нужно было только ускоряться и тормозить /=\ => /\ 
     *n1 = (n1+n2)decel / (accel + decel)
    */
    accel_lim = ((long)step * decel) / (accel + decel);
    // Аналогично, мы должны ускоритьс хлоть один шаг перед началом торможения
    if (accel_lim == 0) {
      accel_lim = 1;
    }

    // Логика такая, что (max_s_lim) * accel/decel = w^2/(2 alpha accel) *accel/decel = w^2/(2 alp
    srd.decel_val = -((long)max_s_lim * accel) / decel;


    // Отсюда srd.decel_val полностью адекватное число шагов для того, чтобы затормозить двигатель
    
    // Так как srd.decel_val < 0 => step + srd.decel_val = step - steps_to_stop - норм 
    srd.decel_start = step + srd.decel_val;

    srd.run_state = ACCEL;
    
    #ifdef IS_DEBUG
    Serial.println(F("ACCEL"));
    #endif

    // Обновим счетчик
    srd.accel_count = 0;
    OCR1A = 10;
    // Установим таймер с делителем на 64
    TCCR1B |= ((0 << CS12) | (1 << CS11) | (1 << CS10));

    //И опустим ENA_PIN, чтобы двигатель работал
    //PORTD &= ~(1<< ENA_PIN); // ENA_PIN -> LOW
    digitalWrite(ENA_PIN, LOW);
  }
}


/*! \brief Инициализация счетчика на Timer1.
 *
 */
void initTimer1(void) {
  // На всякий случай установим что мы в состоянии STOP
  srd.run_state = STOP;
  //Без этого не работает [предположительно] не работает OCR1A
  TCCR1A = 0;
  // Timer/Counter 1 в режиме 4 CTC (Не работает)
  TCCR1B = (1 << WGM12);
  // Timer/Counter 1 Output Compare A Match Interrupt enable.
  TIMSK1 = (1 << OCIE1A);
  
  #ifdef IS_DEBUG
  Serial.println(F("STOP"));
  #endif
}


/*! \brief Прерывание, которое триггерится по таймеру
 *
 *  При совпадения счетчика таймера и содержимого OCR1A
 *  вызывается это прерывание. В нем находится основная логика
 *  рассчета числа тиков таймера до следующего шага
 */
ISR(TIMER1_COMPA_vect) {
  // Время следующей паузы
  volatile uint32_t new_step_delay;
  // для запоминания последней паузы при ускорении. Используется для "затравки" торможения
  volatile static int16_t last_accel_delay;
  // считает шаги при постоянном вращении (?)
  volatile static uint32_t step_count = 0;
  // сохраняем остаток от деления при вычислении периода между шагами
  volatile static uint32_t rest = 0;

  if (pwr_loss and (srd.run_state != STOP)) {
    //отловили отключение энергии
    saveRampState(step_count, rest, last_accel_delay);
    pwr_loss =  false;
    need_to_stop = true; //Попытаемся затормозить насколько хватит энергии
  }

  if (is_restoring) {
    // Если востанавливаем состояние рампы после нежелательного перезапуска
    restoreRampState();
    #ifdef IS_DEBUG
    Serial.println(F("Data loaded succesfully"));
    #endif

    is_restoring = false;
    
    //Вернем все параметры рампы как-будто мы только начали очередной цикл (удержим только число оставшихся циклов) 
    last_accel_delay = 0;
    step_count = 0;
    rest = 0;
    srd.run_state = ACCEL;
    srd.accel_count = 0;
    srd.step_delay = (T1_FREQ_148 * m_sqrt(A_SQ / srd.accel)) / 100;
    OCR1A = srd.step_delay; 
    digitalWrite(ENA_PIN, LOW);
    is_working = true;

    return;
  }
  else {
    //загрузим в OCR1A паузу c_i (изначально c_0, но обновляется при каждом вызове)
    OCR1A = srd.step_delay;
    OCR1B = (srd.step_delay >> 8);
  }
  

  switch (srd.run_state) {
    case STOP:
      // Если не работаем, то обнулим все и остановим таймер1
      is_stopping = false;
      //need_refresh_speed_menu = true;
      step_count = 0;
      rest = 0;
      // Остановим Timer1
      TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));
      digitalWrite(ENA_PIN, HIGH);
      break;

    case ACCEL:
    // Если в режиме ускорения
      if (need_to_stop) {
        //  обработка необходимости остановки
        need_to_stop = false; //сняли флаг
        is_stopping = true;

        srd.run_state = DECEL;

        #ifdef IS_DEBUG
        Serial.println(F("DECEL"));
        #endif

        //  далее пересчитаем режим торможения. Для этого необходимо пересчитать srd.accel_count
        // логика такая, что для достижения фиксированной скорости n1*a1 = n2*a2, т.е. n_decel = n_accel*(a2/a1)
        srd.accel_count = - ((long)step_count * srd.accel) / USER_DECEL;

        new_step_delay = srd.step_delay;  //Делаем еще одну задержку аналогичную предыдущей
      }
      else {
        //делаем шаг
        doTheFStep(srd.dir);
        step_count++; //прибавим общее число шагов
        srd.accel_count++;  //и прибавим счтчик ускорения
        new_step_delay = srd.step_delay - (((2 * (long)srd.step_delay) + rest) / (4 * srd.accel_count + 1));  // Нашли новую паузу 
        rest = ((2 * (long)srd.step_delay) + rest) % (4 * srd.accel_count + 1); //обновил остаток для следующей итерации
        
        
        // Проверим, не достигли ли мы макс скорости
        if (new_step_delay <= srd.min_delay) {
          last_accel_delay = new_step_delay;  //сохранили последний вычисленный период ожидания
          new_step_delay = srd.min_delay; // Записали что теперь мы вращаемся с фиксированной скоростью, определяемой минимальной паузой
          rest = 0;

          srd.run_state = RUN;
          
          #ifdef IS_DEBUG
          Serial.println(F("RUN"));
          #endif 
        }
      }
      break;

    case RUN:
      if (need_to_stop) {
        //обработка необходимости остановки
        need_to_stop = false; //сняли флаг
        is_stopping = true;

        srd.run_state = DECEL;

        #ifdef IS_DEBUG
        Serial.println(F("DECEL"));
        #endif

        //далее пересчитаем режим торможения. Для этого необходимо пересчитать srd.accel_count
        srd.accel_count = - (long)srd.speed*srd.speed/(long)(((long)A_x20000 * USER_DECEL) / 100);

        new_step_delay = last_accel_delay;
        //OCR1A = 10; //минимальная задержка чтобы затриггерить новое прерывание 
      }
      else {
        doTheFStep(srd.dir);
        step_count++;
        new_step_delay = srd.min_delay; //каждый раз обновляем паузу как минмиальную задержку
        // Проверим, не пора ли нам остановиться
        if (step_count >= srd.decel_start) {
          srd.accel_count = srd.decel_val;  //Загрузили кол-во шагов, необходимое для торможения (вернее, их отрицательную величину)
          // Начать торможение с той-же паузой, которая была вычислена на последнем ускорении (TODO: ?!)
          new_step_delay = last_accel_delay;
          srd.run_state = DECEL;
          
          #ifdef IS_DEBUG
          Serial.println(("DECEL"));
          #endif
          //rest=0;
        }
      }
      break;

    case DECEL:
    // Аналогично ускорению,
      if (need_to_stop) {
        //  обработка необходимости остановки
        need_to_stop = false; //сняли флаг
        is_stopping = true;

        srd.run_state = DECEL;
        
        #ifdef IS_DEBUG
        Serial.println(F("DECEL"));
        #endif
        
        //  далее пересчитаем режим торможения. Для этого необходимо пересчитать srd.accel_count
        // логика такая, что для достижения фиксированной скорости n1*a1 = n2*a2, т.е. n_decel1 = n_decel2 * (a2/a1)
        // Здесь не уверен. Оставшееся число шагов =-srd.accel_count, т.е. работать нужно не со step_count, а именно с ним
        srd.accel_count = ((long)srd.accel_count * srd.decel) / USER_DECEL;

        //Serial.println(srd.accel_count);
        new_step_delay = srd.step_delay;  //Делаем еще один 
      } 
      else {
        doTheFStep(srd.dir);
        step_count++;
        srd.accel_count++;
        new_step_delay = srd.step_delay + (((2 * (long)srd.step_delay) + rest) / (-4*srd.accel_count -3));
        rest = ((2 * (long)srd.step_delay) + rest) % (-4 * srd.accel_count - 3);
        // Проверка на последний шаг
        if (srd.accel_count >= 0) {
          if (is_stopping) {
            //Если пользователь закончил работу и мы закончили тормозить, то останавливаем все рассчеты
            srd.run_state = STOP;
            
            #ifdef IS_DEBUG
            Serial.println(F("STOP"));
            #endif
            
            is_working = false;
            refresh_screen = true;
          }
          else{
            if (srd.t_pause == 0) {
              //Если нет паузы, то проводим все вычисления здесь
              if (srd.n_cycles==1) {
                //Если после этого не осталось циклов, то тормозим
                srd.run_state = STOP; 
                #ifdef IS_DEBUG
                Serial.println(F("STOP"));
                #endif
                digitalWrite(ENA_PIN, HIGH);
                is_working = false;
                refresh_screen = true;
                //PORTD |= (1<< ENA_PIN); // ENA_PIN -> HIGH (убрали ток удержания)
              }
              else {
                //Если циклы остались (бесконечно или конечно) 
                if (srd.is_bidir) {
                  //Если двустороннее вращение, то меняем направление
                  srd.dir = (srd.dir == CCW)?CW:CCW;
                }
                if (srd.n_cycles!=0) {
                  srd.n_cycles--;
                }
                //Логика запуска цикла по-новой 
                //! TODO: возможно, следует сохранять не ускорение, а сразу c_0 
                new_step_delay = (T1_FREQ_148 * m_sqrt(A_SQ / srd.accel))/100;
                rest = 0;
                step_count = 0;
                srd.accel_count = 0;
                srd.run_state = ACCEL;
                
                #ifdef IS_DEBUG
                Serial.println(F("ACCEL"));
                #endif
                
                OCR1A = new_step_delay; //На всякий случай
              }
            }
            else {
              
              //Запускаем паузу, и перенесем все рассчеты нового цикла в нее
              srd.run_state = PAUSE;
              
              #ifdef IS_DEBUG
              Serial.println(F("PAUSE"));
              #endif
              // Перейдем в режим паузы и каждую секунду будем триггерить прерывание, пока не истечет необходимое время 
              noInterrupts();  // Отключаем прерывания
              
              TCCR1A = 0;
              TCCR1B = 0;

              TCCR1B |= (1 << WGM12); // Выставляем CTC (Clear Timer on Compare Match mode)
              TCCR1B |= (1 << CS12);  // ставим делитель на 256

              OCR1A = 62500 - 1;  // Выставим величину, с которой мы сравниваем на прерывания каждую секунду 

              TIMSK1 |= (1 << OCIE1A);  // Разрешаем  TIMER1_COMPA прерывания (не факт что нужно, просто на всякий случай)          
              //srd.t_pause = md.mem_t_pause; //Выставим счетчик паузы из памяти (вероятно, может понадобиться вычесть 1с)
              srd.t_pause = CYCLE_DATA.t_pause;
              interrupts();  // Разрешаем прерывания
            }
          }
        }
      }
    break;

    case PAUSE:
      if (need_to_stop) {
        // Если пользователю нужно остановить работу во время паузы
        need_to_stop = false;
        srd.run_state = STOP; 
        
        #ifdef IS_DEBUG
        Serial.println(F("STOP"));
        #endif
        
        digitalWrite(ENA_PIN, HIGH);
        is_working = false;
        refresh_screen = true;
        //PORTD |= (1<< ENA_PIN); // ENA_PIN -> HIGH
      }
      else {
        srd.t_pause--; //Вычли один из периодов паузы

        if (srd.t_pause == 0) {
          //Если пауза кончилась, действуем полностью аналогично ситуации с концом ускорения 
          if (srd.n_cycles==1) {
            //Если после этого не осталось циклов, то тормозим
            srd.run_state = STOP; 
            
            #ifdef IS_DEBUG
            Serial.println(F("STOP"));
            #endif
            digitalWrite(ENA_PIN, HIGH);
            is_working = false;
            refresh_screen = true;
            //PORTD |= (1<< ENA_PIN); // ENA_PIN -> HIGH
          }
          else {
            //Если циклы остались (бесконечно или конечно) 
            if (srd.is_bidir) {
              //Если двустороннее вращение, то меняем направление
              srd.dir = (srd.dir == CCW) ? CW : CCW;
            }
            if (srd.n_cycles!=0) {
              srd.n_cycles--;
            }
            //Логика запуска цикла по-новой 
            new_step_delay = (T1_FREQ_148 * m_sqrt(A_SQ / srd.accel))/100;
            //Serial.println(new_step_delay);
            rest = 0;
            step_count = 0;
            srd.accel_count = 0;
            srd.run_state = ACCEL;
            
            #ifdef IS_DEBUG
            Serial.println(F("ACCEL"));
            #endif
            
            noInterrupts();
            TCCR1A = 0;
            TCCR1B = (1<< WGM12); // Выставляем CTC (Clear Timer on Compare Match mode)

            TCCR1B |= ((0 << CS12) | (1 << CS11) | (1 << CS10));  // вернем делитель на 64
            
            TIMSK1 = (1 << OCIE1A);
            OCR1A = new_step_delay; //На всякий случай
            interrupts();

          }
        }
      }
    break;
  }
   
  srd.step_delay = new_step_delay;
}


void initStepper() {
  
  pinMode(ENA_PIN,OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN,OUTPUT);
  //DDRD |= (1 << ENA_PIN); //pinMode(ENA_PIN, OUTPUT);
  //DDRD |= (1 << STEP_PIN); //pinMode(STEP_PIN, OUTPUT);
  //DDRD |= (1 << DIR_PIN); //pinMode(DIR_PIN, OUTPUT);

  digitalWrite(ENA_PIN, HIGH);
  //PORTD |= (1<< ENA_PIN);  //digitalWrite(ENA_PIN, HIGH);
  
}

void doTheFStep(uint8_t dir) {

  if (dir == CCW) { 
    digitalWrite(DIR_PIN, HIGH);
    //PORTD |= (1 << DIR_PIN);  
  }
  else {
    digitalWrite(DIR_PIN,LOW); 
    //PORTD &= ~(1 << DIR_PIN);
  }
  
  digitalWrite(STEP_PIN, HIGH);
  //PORTD |= (1 << STEP_PIN); //digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(2);
  //PORTD &= ~(1 << STEP_PIN); //digitalWrite(STEP_PIN, LOW);
  digitalWrite(STEP_PIN, LOW);
}

#ifdef IS_DEBUG
void printRampData() {
  

  Serial.print(F("run_state: ")); Serial.println(srd.run_state);
  Serial.print(F("dir: ")); Serial.println(srd.dir);
  Serial.print(F("speed: ")); Serial.println(srd.speed);
  Serial.print(F("step_delay: ")); Serial.println(srd.step_delay);
  Serial.print(F("decel_start: ")); Serial.println(srd.decel_start);
  Serial.print(F("decel_val: ")); Serial.println(srd.decel_val);
  Serial.print(F("min_delay: ")); Serial.println(srd.min_delay);
  Serial.print(F("accel_count: ")); Serial.println(srd.accel_count);
  Serial.print(F("accel: ")); Serial.println(srd.accel);
  Serial.print(F("decel: ")); Serial.println(srd.decel);
  Serial.print(F("n_cycles: ")); Serial.println(srd.n_cycles);
  Serial.print(F("is_bidir: ")); Serial.println((bool)srd.is_bidir);
  Serial.print(F("t_pause: ")); Serial.println(srd.t_pause);

}

void printRampDataEEPROM() {
  Serial.print(F("run_state: ")); Serial.println(eeprom_read_byte((uint8_t *)RAMP_FIRST_BYTE));
  Serial.print(F("dir: ")); Serial.println(eeprom_read_byte((uint8_t *)RAMP_FIRST_BYTE+1));
  Serial.print(F("speed: ")); Serial.println(eeprom_read_word((uint16_t *)RAMP_FIRST_BYTE+2));
  Serial.print(F("step_delay: ")); Serial.println(eeprom_read_word((uint16_t *)RAMP_FIRST_BYTE+4));
  Serial.print(F("decel_start: ")); Serial.println(eeprom_read_dword((uint32_t *)RAMP_FIRST_BYTE+6));
  Serial.print(F("decel_val: ")); Serial.println(eeprom_read_dword((int32_t *)RAMP_FIRST_BYTE+10));
  Serial.print(F("min_delay: ")); Serial.println(eeprom_read_word((int16_t *)RAMP_FIRST_BYTE+14));
  Serial.print(F("accel_count: ")); Serial.println(eeprom_read_dword((int32_t *)RAMP_FIRST_BYTE+16));
  Serial.print(F("accel: ")); Serial.println(eeprom_read_word((uint16_t *)RAMP_FIRST_BYTE+20));
  Serial.print(F("decel: ")); Serial.println(eeprom_read_word((uint16_t *)RAMP_FIRST_BYTE+22));
  Serial.print(F("n_cycles: ")); Serial.println(eeprom_read_byte((uint8_t *)RAMP_FIRST_BYTE+24));
  Serial.print(F("is_bidir: ")); Serial.println((bool)eeprom_read_byte((uint8_t *)RAMP_FIRST_BYTE+25));
  Serial.print(F("t_pause: ")); Serial.println(eeprom_read_word((uint16_t *)RAMP_FIRST_BYTE+26));
}
#endif


void restoreRampState() {
  /*
    srd.run_state = eeprom_read_word((uint8_t *)RAMP_FIRST_BYTE);
    srd.dir = eeprom_read_word((uint8_t *)RAMP_FIRST_BYTE+1);
    srd.speed = eeprom_read_word((uint16_t *)RAMP_FIRST_BYTE+2);
    srd.step_delay = eeprom_read_word((uint16_t *)RAMP_FIRST_BYTE+4);
    srd.decel_start = eeprom_read_word((uint32_t *)RAMP_FIRST_BYTE+6);
    srd.decel_val = eeprom_read_word((int32_t *)RAMP_FIRST_BYTE+10);
    srd.min_delay = eeprom_read_word((int16_t *)RAMP_FIRST_BYTE+14);
    srd.accel_count = eeprom_read_word((int32_t *)RAMP_FIRST_BYTE+16);
    srd.accel = eeprom_read_word((uint16_t *)RAMP_FIRST_BYTE+20);
    srd.decel = eeprom_read_word((uint16_t *)RAMP_FIRST_BYTE+22);
    srd.n_cycles = eeprom_read_word((uint8_t *)RAMP_FIRST_BYTE+24);
    srd.is_bidir = eeprom_read_word((bool *)RAMP_FIRST_BYTE+25);
    srd.t_pause = eeprom_read_word((uint16_t *)RAMP_FIRST_BYTE+26);
  */
  eeprom_read_block((void*)&srd, (uint16_t*)RAMP_FIRST_BYTE, sizeof(srd));
}

void saveRampState(uint32_t step_count, uint32_t rest, uint16_t last_accel_delay) {

  /*eeprom_update_word((uint8_t *)RAMP_FIRST_BYTE, srd.run_state);//
    eeprom_update_word((uint8_t *)(RAMP_FIRST_BYTE+1), srd.dir);//
    eeprom_update_word((uint16_t *)(RAMP_FIRST_BYTE+2), srd.speed);//
    eeprom_update_word((uint16_t *)(RAMP_FIRST_BYTE+4), srd.step_delay);//
    eeprom_update_word((uint32_t *)(RAMP_FIRST_BYTE+6), srd.decel_start);//
    eeprom_update_word((int32_t *)(RAMP_FIRST_BYTE+10), srd.decel_val);//
    eeprom_update_word((int16_t *)(RAMP_FIRST_BYTE+14), srd.min_delay);//
    eeprom_update_word((int32_t *)(RAMP_FIRST_BYTE+16), srd.accel_count);//
    eeprom_update_word((uint16_t *)(RAMP_FIRST_BYTE+20), srd.accel);
    eeprom_update_word((uint16_t *)(RAMP_FIRST_BYTE+22), srd.decel);
    eeprom_update_word((uint8_t *)(RAMP_FIRST_BYTE+24), srd.n_cycles);
    eeprom_update_word((bool *)(RAMP_FIRST_BYTE+25), srd.is_bidir);
    eeprom_update_word((uint16_t *)(RAMP_FIRST_BYTE+26), srd.t_pause);
  */
  eeprom_write_block((void*)&srd, (uint16_t*)RAMP_FIRST_BYTE, sizeof(srd));
#ifdef IS_DEBUG
  Serial.println(F("Ramp data has been saved"));
  Serial.println(F("===== Data from EEPROM ====="));
  printRampDataEEPROM();
  Serial.println(F("")); Serial.println(F(""));
  Serial.println(F("===== Actual data ====="));
  printRampData();
#endif
}

/*! \brief Square root routine.
 *
 * sqrt routine 'grupe', from comp.sys.ibm.pc.programmer
 * Subject: Summary: SQRT(int) algorithm (with profiling)
 *    From: warwick@cs.uq.oz.au (Warwick Allison)
 *    Date: Tue Oct 8 09:16:35 1991
 *
 *  \param x  Value to find square root of.
 *  \return  Square root of x.
 */
uint32_t m_sqrt(uint32_t x) {
  register uint32_t xr;  // result register
  register uint32_t q2;  // scan-bit register
  register uint8_t f;   // flag (one bit)

  xr = 0;            // clear result
  q2 = 0x40000000L;  // higest possible result bit
  do {
    if ((xr + q2) <= x) {
      x -= xr + q2;
      f = 1;  // set flag
    } else {
      f = 0;  // clear flag
    }
    xr >>= 1;
    if (f) {
      xr += q2;  // test flag
    }
  } while (q2 >>= 2);  // shift twice
  if (xr < x) {
    return xr + 1;  // add for rounding
  } else {
    return xr;
  }
}

static unsigned long m_qrt(unsigned long x) {
    if (x == 0) {
        return 0;
    }

    register uint32_t xr = x;
    register uint32_t y = 1;

    while (xr > y) {
        xr = (2 * xr + y) / 3;
        y = x / (xr * x);
    }

    return xr;
}

static uint32_t m_qrt2(uint32_t x) {
    register uint32_t low = 0; 
    register uint32_t high = x; 
    register uint32_t mid;
    register uint32_t result;

    while (low <= high) {
        mid = low + (high - low) >> 1;
        register uint32_t cube = mid * mid * mid;

        if (cube == x)
            return mid;

        if (cube < x) {
            low = mid + 1;
            result = mid; // Update the potential result
        } else {
            high = mid - 1;
        }
    }
    return result; // Return the closest integer value
}