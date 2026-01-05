#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

/*#define BTN1 12
#define BTN2 13*/

//QueueHandle_t q;
class Buttons{
  private:
  QueueHandle_t q;
  const int on_off_pin = 25;
  const int ok_pin = 27;
  static void buttonReaderStatic(void* pv) {
    Buttons* instance = static_cast<Buttons*>(pv);
    instance->buttonReader();
  }
  void buttonReader(/*void* pv*/) {
    pinMode(on_off_pin, INPUT);
    pinMode(ok_pin, INPUT);
    
    int b1_prev = 1, b2_prev = 1;
    unsigned long t1 = 0, t2 = 0;
    
    while(1) {
        // Кнопка 1
        int b1 = digitalRead(on_off_pin);
         if(b1 == 1) {
            // Кнопка нажата
            uint8_t data = 1;
            xQueueSend(q, &data, 0);
        }
        
        // Кнопка 2
        int b2 = digitalRead(ok_pin);
         if(b2 == 1) {
            // Кнопка нажата
            uint8_t data = 2;
            xQueueSend(q, &data, 0);
        }
        
        vTaskDelay(50 / portTICK_PERIOD_MS); //10
    }
  }
  public:
  Buttons(){
    q = xQueueCreate(5, sizeof(uint8_t));
    if(q == NULL) {
      Serial.println("ERROR: Failed to create queue!");
      return;
    }
    xTaskCreate(buttonReaderStatic, "btn", 2048, this, 2, NULL);
  }

  

  bool on_click(){
    uint8_t data;
    if(xQueueReceive(q, &data, 0) == pdTRUE) {
      if(data==1){
        return(true);
      } /*else{
        return(false)
      }*/
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
   //Serial.println("k1");
    //uint8_t data;
    //Serial.println(digitalRead(25));
    /*if(xQueueReceive(q, &data, 0) == pdTRUE) {
      if(data==1){
        Serial.println("1");
      } else{
        Serial.println("2");
      }
    }*/
    Serial.println("k1");
    if(buttons.on_click()==true){
      Serial.println("1");
    }
    
    delay(200);
}