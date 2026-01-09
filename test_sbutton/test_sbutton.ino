#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

class Buttons{
  private:
  QueueHandle_t q;
  QueueHandle_t longPressQueue;
  bool reading_queue = true; // чтобы очередь опрашивалась один раз при нажатии кнопки, а не по числу вызовов методов click
  uint8_t data = -1;
  const int on_off_pin = 25;
  const int ok_pin = 27;
  const int left_pin = 12; //13;
  const int right_pin = 32;
  const int up_pin = 26; // кнопка вверх
  const int down_pin = 14; // кнопка вниз
  const int queue_delay = 50; // в мс
  const unsigned long LONG_PRESS_TIME = 1500;  // 3 секунды

  static void buttonReaderStatic(void* pv) {
    Buttons* instance = static_cast<Buttons*>(pv);
    instance->buttonReader();
  }

  void buttonReader() { // поток работает до конца работы программы
    int b1_prev = 0;
    int b2_prev = 0;
    int b3_prev = 0;
    int b4_prev = 0;
    int b5_prev = 0;
    int b6_prev = 0;
    unsigned long int press_start_time = 0; // для проверки длинного нажатия
    bool ok_pressed = false;
    unsigned long current_time; // текущее время
    
    while(1) { // этот цикл будет длиться до конца выполнения программы
        current_time = millis(); // текущее время
        // Кнопка 1
        int b1 = digitalRead(on_off_pin);
        if((b1 == 1)&&(b1_prev == 0)) { // если кнопка нажата
          vTaskDelay(queue_delay / portTICK_PERIOD_MS);
          b1 = digitalRead(on_off_pin);
          if((b1 == 1)&&(b1_prev == 0)){
            uint8_t data = 1; // 1, если кнопка 1 нажата
            xQueueSend(q, &data, 0);
          }
        }
        b1_prev = b1; 
        // Кнопка 2
        int b2 = digitalRead(ok_pin);
        if((b2 == 1)&&(b2_prev == 1)) { // проверка длинного нажатия
          if(ok_pressed && press_start_time > 0) {
            if((current_time - press_start_time) >= LONG_PRESS_TIME) {
              uint8_t data = 7; // 7 - если кнопка ok была долго нажата
              xQueueSend(longPressQueue, &data, 0);
              press_start_time = 0; // Сбрасываем таймер, чтобы не отправлять повторно
              ok_pressed = false;
            }
          }
        }
        if((b2 == 1)&&(b2_prev == 0)) { // если кнопка нажата
          vTaskDelay(queue_delay / portTICK_PERIOD_MS);
          b2 = digitalRead(ok_pin);
          if((b2 == 1)&&(b2_prev == 0)){
            uint8_t data = 2; // 2, если кнопка 2 нажата
            xQueueSend(q, &data, 0);
            press_start_time = current_time; // Начало отсчета длинного нажатия
            ok_pressed = true;
          }
          if(b2 == 0){  // Если кнопка отпущена
            if(ok_pressed) {
              ok_pressed = false;
              press_start_time = 0;
            }
          }
        }
        b2_prev = b2;
        // Кнопка 3
        int b3 = digitalRead(left_pin);
        if((b3 == 1)&&(b3_prev == 0)) { // если кнопка нажата
          vTaskDelay(queue_delay / portTICK_PERIOD_MS);
          b3 = digitalRead(left_pin);
          if((b3 == 1)&&(b3_prev == 0)){
            uint8_t data = 3; // 3, если кнопка 3 нажата
            xQueueSend(q, &data, 0);
          }
        }
        b3_prev = b3; 
        // Кнопка 4
        int b4 = digitalRead(right_pin);
        if((b4 == 1)&&(b4_prev == 0)) { // если кнопка нажата
          vTaskDelay(queue_delay / portTICK_PERIOD_MS);
          b4 = digitalRead(right_pin);
          if((b4 == 1)&&(b4_prev == 0)){
            uint8_t data = 4; // 4, если кнопка 4 нажата
            xQueueSend(q, &data, 0);
          }
        }
        b4_prev = b4; 
        // Кнопка 5
        int b5 = digitalRead(up_pin);
        if((b5 == 1)&&(b5_prev == 0)) { // если кнопка нажата
          vTaskDelay(queue_delay / portTICK_PERIOD_MS);
          b5 = digitalRead(up_pin);
          if((b5 == 1)&&(b5_prev == 0)){
            uint8_t data = 5; // 5, если кнопка 5 нажата
            xQueueSend(q, &data, 0);
          }
        }
        b5_prev = b5;
        // Кнопка 6
        int b6 = digitalRead(down_pin);
        if((b6 == 1)&&(b6_prev == 0)) { // если кнопка нажата
          vTaskDelay(queue_delay / portTICK_PERIOD_MS);
          b6 = digitalRead(down_pin);
          if((b6 == 1)&&(b6_prev == 0)){
            uint8_t data = 6; // 1, если кнопка 1 нажата
            xQueueSend(q, &data, 0);
          }
        }
        b6_prev = b6; 
        vTaskDelay(5 / portTICK_PERIOD_MS); //10 Приостанавливает выполнение текущей задачи на 10 миллисекунд. portTICK_PERIOD_MS - перевод "тиков" в мс
    }
  }
  public:
  Buttons(){
    pinMode(on_off_pin, INPUT);
    pinMode(ok_pin, INPUT);
    pinMode(left_pin, INPUT);
    pinMode(right_pin, INPUT);
    pinMode(up_pin, INPUT);
    pinMode(down_pin, INPUT);
    q = xQueueCreate(5, sizeof(uint8_t)); // 2 - приоритет (выше, чем у loop), это запуск потока
    longPressQueue = xQueueCreate(5, sizeof(uint8_t));
    if(q == NULL) {
      Serial.println("ERROR: Failed to create queue!");
      return;
    }
    xTaskCreate(buttonReaderStatic, "btn", 2048, this, 2, NULL);
  }

  int get_button_number(){
    if(xQueueReceive(q, &data, 0) == pdTRUE) {
      return(data);
    } else{
      return(0);
    }
  }

  bool ok_long_pressed() {
    uint8_t btnData;
    if(xQueueReceive(longPressQueue, &btnData, 0) == pdTRUE) {
      return (btnData == 7);
    }
    return false;
  }
};


Buttons buttons;
void setup() {
    Serial.begin(115200);
}

void loop() {
    //Serial.println("k1");
    switch(buttons.get_button_number()){
      case 1:
      Serial.println("1");
      break;
      case 2:
      Serial.println("2");
      break;
      case 3:
      Serial.println("3");
      break;
      case 4:
      Serial.println("4");
      break;
      case 5:
      Serial.println("5");
      break;
      case 6:
      Serial.println("6");
      break;
    }
    if(buttons.ok_long_pressed()){
      Serial.println("7");
    }

    /*if(buttons.ok_click()==true){
      Serial.println("2");
    }
    if(buttons.on_off_click()==true){
      Serial.println("1");
    }
    
    if(buttons.left_click()==true){
      Serial.println("3");
    }
    if(buttons.right_click()==true){
      Serial.println("4");
    }
    if(buttons.up_click()==true){
      Serial.println("5");
    }
    if(buttons.down_click()==true){
      Serial.println("6");
    }*/
    delay(200);
}