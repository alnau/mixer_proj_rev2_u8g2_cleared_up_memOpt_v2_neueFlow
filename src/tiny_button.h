#ifndef tinyButton_h
#define tinyButton_h

#include "flags.h"


/**
 * \brief: облегченная библиотека опроса кнопок с дебаунсом. Отслеживает нажатия и отпускания кнопок
 *
 * Занимает порядка 280 байт в програмной памяти, а также 5 байт в динамической
 *
 * tick(): опрашивает кнопку и обновляет флаги состояния 
 * isClicked(): возвращает true если произошло нажатие. false иначе
 * isReleased(): возвращает true если кнопку отпустили. false иначе
 * getDebounceTime(): возвращает время дебаунса (0-255 мс)
 * setDebounceTime(): позволяет изменить время дебаунса (0-255 мс)
 * 
 * В теории можно без особых усложнений обобщить эту либу еще и на удержание кнопки
 * Вся сложность лишь в том, чтобы это не интерферировало с чтением кнопки
 */

//Номера битов в которых хранятся флаги
#define STATE 0
#define LAST_STATE 1
#define IS_READY 2
#define READING 3
#define IS_CLICK 4
#define IS_RELEASED 5
#define IS_READY_HOLD 6
#define IS_HOLD 7

#define DEBOUNCE_TIME 50

class Button {
private: 
  flag button_state;            //8-ми битный буффер для регистрации состояния кнопки (см #define)
  //uint8_t _debounce_time = 50;  //Переменная времени дебаунса. Возможно измениение 0-255
  uint8_t _pin_number = 0;  //Номер пина кнопки (проверено на Attiny x5)
  /**
   * \brief: чтение ТЕКУЩЕГО состояния кнопки. При active low 0 -> нажата  
   *
   * \return: возвращает состояние бита, отвечающего данной кнопке (проаерено на attiny85)
   */
  uint8_t _read_button() {
    return digitalRead(_pin_number);
  }
public:
/**
 * \brief: конструктор экземпляра кнопки
 *
 * настраивает пин на INPUT_PULLUP, а также "инициализирует" регистр, в котором хранятся
 * флаги состояния кнопки: button_state
 */
  Button(uint8_t _PIN) {
  
    //создадим экземпляр кнопки
    _pin_number = _PIN;

    //PINx = _PINx; 
 
    //далее необходимо вписать состояние в button_state();
    button_state.set(STATE);
    button_state.set(LAST_STATE);

    //Конфигурация пинов  
    pinMode(_pin_number, INPUT_PULLUP);
    // DDRB &= ~(1 << _PIN);  //выставим PIN как вход (занулим бит)
    // PORTB |= (1 << _PIN);
    // MCUCR &= ~(1 << PUD);  //запишем 0 в PUD чтобы был input pullup
  }

  /**
   * \brief: опрос состояния кнопки. 
   * \WARNING: следует вызывать так часто, как это возмножно 
   *
   * Основная функция библиотеки, модифицирующая состояние кнопки. Внутри происходит 
   * регистрация изменения состояния кнопки, а также софт-дебаунс, необходимый для стабильной 
   * работы со стандартныи push button
   *
   * TODO: добавить регистрацию зажатия кнопки
   */
  void tick() {
    static uint16_t last_deb_t_check;
    button_state.reset(IS_READY);
    button_state.reset(IS_CLICK);
    button_state.reset(IS_RELEASED);

    

    button_state.write(STATE, _read_button()); //записали текущее состояние кнопки
    if (button_state.read(STATE)!=button_state.read(LAST_STATE) and !button_state.read(READING)) {
      //если текущее состояние отличается от предыдущего и еще не началичтение
      last_deb_t_check = (uint16_t)millis();
      button_state.reset(IS_READY); //записали что пока не готовы  
      button_state.set(READING);  //записали что пока считываем 
    }

    if (((uint16_t)millis() - last_deb_t_check > DEBOUNCE_TIME) and button_state.read(READING)) {
      button_state.reset(READING); //сняли флаг чтения
      uint8_t state = _read_button();
      if (button_state.read(STATE) == state) {
        //если все-таки сохранилось новое значение
        button_state.write(LAST_STATE, state);  //записали текущее значение как предыдущее
        if (state)
          button_state.set(IS_RELEASED); //кнопка высокая => отжали
        else
          button_state.set(IS_CLICK); //кнопка низкая => нажали
      }
      button_state.set(IS_READY); //обозначили что готовы вернуть значение
    }
  }


  /**
   * \brief: функция, возвращающая true в случае, если кнопка была нажата
   *
   * \WARNING: Если все еще идет анализ дебаунса, то будет возвращено false, даже если кнопка была зажата. 
   * Результат будет положителен если кнопка была зажата И если закончился период дебаунса . 
   */
  bool isClicked() {
    if (!button_state.read(IS_READY) or button_state.read(READING)) 
      return false; //если не готовы или читаем, то вернули ложь
    button_state.reset(IS_READY);  
    return button_state.read(IS_CLICK);
  }

  /**
   * \brief: функция, возвращающая true в случае, если кнопка была отпущена
   *
   * \WARNING: Если все еще идет анализ дебаунса, то будет возвращено false, даже если кнопка была отпущена. 
   * Результат будет положителен если кнопка была отпущена И если закончился период дебаунса . 
   */
  bool isReleased() {
    if (!button_state.read(IS_READY) or button_state.read(READING)) 
      return false; //если не готовы или читаем, то вернули ложь
    button_state.reset(IS_READY);  
    return button_state.read(IS_RELEASED);
  }

//   bool isHold() {

//   }

};

#endif
