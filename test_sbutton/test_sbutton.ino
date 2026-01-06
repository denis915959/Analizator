#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

class Buttons{
  private:
  QueueHandle_t q;
  const int on_off_pin = 25;
  //const int ok_pin = 27;
  static void buttonReaderStatic(void* pv) {
    Buttons* instance = static_cast<Buttons*>(pv);
    instance->buttonReader();
  }
  void buttonReader() {
    pinMode(on_off_pin, INPUT);
    int b1_prev = 0;
    
    while(1) { // этот цикл будет длиться до конца выполнения программы
        // Кнопка 1
        int b1 = digitalRead(on_off_pin);
         if((b1 == 1)&&(b1_prev == 0)) { // если кнопка нажата
            uint8_t data = 1; // 1, если кнопка 1 нажата
            xQueueSend(q, &data, 0);
        }
        b1_prev = b1;        
        vTaskDelay(50 / portTICK_PERIOD_MS); //10 Приостанавливает выполнение текущей задачи на 50 миллисекунд. portTICK_PERIOD_MS - перевод "тиков" в мс
    }
  }
  public:
  Buttons(){
    q = xQueueCreate(5, sizeof(uint8_t)); // 2 - приоритет (выше, чем у loop), это запуск потока
    if(q == NULL) {
      Serial.println("ERROR: Failed to create queue!");
      return;
    }
    xTaskCreate(buttonReaderStatic, "btn", 2048, this, 2, NULL);
  }

  bool on_off_click(){
    uint8_t data;
    if(xQueueReceive(q, &data, 0) == pdTRUE) {
      if(data==1){
        return(true);
      }
    } else{
      return(false);
    }
  }
};

Buttons buttons;
void setup() {
    Serial.begin(115200);
    delay(1000);
}

void loop() {
    Serial.println("k1");
    if(buttons.on_click()==true){
      Serial.println("1");
    }
    
    delay(200);
}