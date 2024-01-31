#ifndef tinyButton_h
#define tinyButton_h

#include "flags.h"

// TODO: переведи переменные на static
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
// #define STATE 0
// #define LAST_STATE 1
// #define IS_READY 2
// #define READING 3
// #define IS_CLICK 4
// #define IS_RELEASED 5
// #define IS_READY_HOLD 6
// #define IS_HOLD 7

// #define DEBOUNCE_TIME 10

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
  const static uint8_t STATE = 0;
  const static uint8_t LAST_STATE = 1;
  const static uint8_t IS_READY = 2;
  const static uint8_t READING = 3;
  const static uint8_t IS_CLICK = 4;
  const static uint8_t IS_RELEASED = 5;
  const static uint8_t IS_READY_HOLD  = 6;
  const static uint8_t IS_HOLD = 7;

  const static uint8_t DEBOUNCE_TIME = 10;

/*

//https://hackaday.com/2015/12/09/embed-with-elliot-debounce-your-noisy-buttons-part-i/
//https://hackaday.com/2015/12/10/embed-with-elliot-debounce-your-noisy-buttons-part-ii/
enum ButtonStates { UP, DOWN, PRESS, RELEASE };
 
enum ButtonStates delay_debounce(enum ButtonStates button_state) {        
      if (read_button()){                      // if pressed     
          if (button_state == PRESS){
            button_state = DOWN;
        } 
        if (button_state == UP){
            _delay_ms(5);
            if (read_button() == 1){
                button_state = PRESS;
            }
        } 
    } else {                                 // if not pressed 
        if (button_state == RELEASE){
            button_state = UP;
        } 
        if (button_state == DOWN){
            if (read_button() == 0){
                _delay_ms(5);
                if (read_button() == 0){
                    button_state = RELEASE;
                }
            }
        }
    }
    return button_state;
}

*/


  bool _read_button() {
    //return digitalRead(_pin_number);
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
    /*
     * TODO: Допускаю что в нынешних реалиях, эту величину можно перевести 
     * на uint8_t, но это нужно тестировать. Опять-же, пока у меня есть 
     * while(1) это может вызвать проблемы, но очень похоже, что это 
     * поведение не проявится в принципе
     */
    static uint8_t last_deb_t_check; 
    
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
    // TODO: возможно, недопустимая оптимизация
    if (((uint8_t)millis() - last_deb_t_check > DEBOUNCE_TIME) and button_state.read(READING)) {
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

// /******************************************************************************
// debounce.c
// written by Kenneth A. Kuhn
// version 1.00

// This is an algorithm that debounces or removes random or spurious
// transistions of a digital signal read as an input by a computer.  This is
// particularly applicable when the input is from a mechanical contact.  An
// integrator is used to perform a time hysterisis so that the signal must
// persistantly be in a logical state (0 or 1) in order for the output to change
// to that state.  Random transitions of the input will not affect the output
// except in the rare case where statistical clustering is longer than the
// specified integration time.

// The following example illustrates how this algorithm works.  The sequence 
// labeled, real signal, represents the real intended signal with no noise.  The 
// sequence labeled, corrupted, has significant random transitions added to the 
// real signal.  The sequence labled, integrator, represents the algorithm 
// integrator which is constrained to be between 0 and 3.  The sequence labeled, 
// output, only makes a transition when the integrator reaches either 0 or 3.  
// Note that the output signal lags the input signal by the integration time but 
// is free of spurious transitions.
 
// real signal 0000111111110000000111111100000000011111111110000000000111111100000
// corrupted   0100111011011001000011011010001001011100101111000100010111011100010
// integrator  0100123233233212100012123232101001012321212333210100010123233321010
// output      0000001111111111100000001111100000000111111111110000000001111111000

// I have been using this algorithm for years and I show it here as a code
// fragment in C.  The algorithm has been around for many years but does not seem
// to be widely known.  Once in a rare while it is published in a tech note.  It 
// is notable that the algorithm uses integration as opposed to edge logic 
// (differentiation).  It is the integration that makes this algorithm so robust 
// in the presence of noise.
// ******************************************************************************/

// /* The following parameters tune the algorithm to fit the particular
// application.  The example numbers are for a case where a computer samples a
// mechanical contact 10 times a second and a half-second integration time is
// used to remove bounce.  Note: DEBOUNCE_TIME is in seconds and SAMPLE_FREQUENCY
// is in Hertz */

// #define DEBOUNCE_TIME		0.3
// #define SAMPLE_FREQUENCY	10
// #define MAXIMUM			(DEBOUNCE_TIME * SAMPLE_FREQUENCY)

// /* These are the variables used */
// unsigned int input;       /* 0 or 1 depending on the input signal */
// unsigned int integrator;  /* Will range from 0 to the specified MAXIMUM */
// unsigned int output;      /* Cleaned-up version of the input signal */


// /* Step 1: Update the integrator based on the input signal.  Note that the 
// integrator follows the input, decreasing or increasing towards the limits as 
// determined by the input state (0 or 1). */

//   if (input == 0)
//     {
//     if (integrator > 0)
//       integrator--;
//     }
//   else if (integrator < MAXIMUM)
//     integrator++;

// /* Step 2: Update the output state based on the integrator.  Note that the
// output will only change states if the integrator has reached a limit, either
// 0 or MAXIMUM. */

//   if (integrator == 0)
//     output = 0;
//   else if (integrator >= MAXIMUM)
//     {
//     output = 1;
//     integrator = MAXIMUM;  /* defensive code if integrator got corrupted */
//     }

// /********************************************************* End of debounce.c */

#endif
