#include "Wire.h"
#include <FS.h>
#include <SD.h>
#include <SPI.h>

#define I2C_DEV_ADDR_1 0x09
#define I2C_DEV_ADDR_2 0x55
#define I2C_DEV_ADDR_3 0x11
const int rele1_pin = 0;
const int rele2_pin = 2;
const int rele3_pin = 4;

char* path = "/data.txt";
char command = -1;
const int warming_time = 300; // время прогрева(в секундах)
const int first_warming_time = 10; //30 - все работает;
int loop_counter = 0;
bool set_zero_flag = true; // если true, то будет использоваться СетЗеро, если false, то не будет 
bool use_autocalibration = false; // если true, то будет использоваться Автокалибровка, если false, то не будет 
bool read_co2 = false; // флаг для работы программы
int delay_between_readings = 150; // при задержке 100 мс частота измерений 1.37 Гц.
const int read_between_warm = 25; // 50

// Переменные для CO2
float ppm_1_medium = 0;
int counter_for_medium_1 = 0; // количество успешных считываний. если =5, то усредняем и обнуляем
int ppm_1_sum = 0;
float ppm_2_medium = 0;
int counter_for_medium_2 = 0; // количество успешных считываний. если =5, то усредняем и обнуляем
int ppm_2_sum = 0;
float ppm_3_medium = 0;
int counter_for_medium_3 = 0; // количество успешных считываний. если =5, то усредняем и обнуляем
int ppm_3_sum = 0;
float ppm_common = 0; // усреднение по 3 датчикам после минимум 5 чтений (это значение записывается на микро-сд)
float time_s = 0; // время в секундах
bool write_sd_flag = false;
float time_step = 0.987; //0.989;

// Переменные для температуры
float temp_1_medium = 0;
int temp_1_sum = 0;
float temp_2_medium = 0;
int temp_2_sum = 0;
float temp_3_medium = 0;
int temp_3_sum = 0;
float temp_common = 0; // усреднение температуры по 3 датчикам

// Переменные для точности
int accuracy_1 = 0;
int accuracy_2 = 0;
int accuracy_3 = 0;

// Переменные для минимального значения CO2
int min_co2_1 = 0;
int min_co2_2 = 0;
int min_co2_3 = 0;

void appendFile(fs::FS &fs, const char * path, const char * message,  bool new_string){ // если new_string==true на новую строку переводит каретку
  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(new_string){
    if(file.println(message)){
    } else {
      Serial.println("Append failed");
    }
  } else {
    if(file.print(message)){
    } else {
      Serial.println("Append failed");
    }
  }
}

void appendFileNumber(fs::FS &fs, const char * path, float number){ // если new_string==true на новую строку переводит каретку
  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(number)){
  } else {
    Serial.println("Append failed");
  }
}

int get_float_size(float ppm){ // выводит длину целой части числа
  int size = -1;
  if(ppm<10){
    size = 1;
  }
  if((ppm<100.00)&&(ppm>=10.00)){
    size = 2;
  }
  if((ppm<1000.00)&&(ppm>=100.00)){
    size = 3;
  }
  if((ppm<10000.00)&&(ppm>=1000.00)){
    size = 4;
  }
  return(size);
}

void write_data_to_sd(float ppm_1, float ppm_2, float ppm_3, float medium_ppm,
                      float temp_1, float temp_2, float temp_3, float medium_temp,
                      int accuracy_1, int accuracy_2, int accuracy_3,
                      int min_co2_1, int min_co2_2, int min_co2_3, float time_1) {
  static char data_array[150];
  static char array_ppm1[9], array_ppm2[9], array_ppm3[9], array_medium_ppm[9];
  static char array_temp1[9], array_temp2[9], array_temp3[9], array_medium_temp[9];
  static char array_accuracy1[9], array_accuracy2[9], array_accuracy3[9];
  static char array_min_co2_1[9], array_min_co2_2[9], array_min_co2_3[9];
  static char array_time[9];

  // Преобразуем числа в строки
  dtostrf(ppm_1, 4, 3, array_ppm1);
  dtostrf(ppm_2, 4, 3, array_ppm2);
  dtostrf(ppm_3, 4, 3, array_ppm3);
  dtostrf(medium_ppm, 4, 3, array_medium_ppm);
  dtostrf(temp_1, 4, 3, array_temp1);
  dtostrf(temp_2, 4, 3, array_temp2);
  dtostrf(temp_3, 4, 3, array_temp3);
  dtostrf(medium_temp, 4, 3, array_medium_temp);
  dtostrf(accuracy_1, 4, 3, array_accuracy1);
  dtostrf(accuracy_2, 4, 3, array_accuracy2);
  dtostrf(accuracy_3, 4, 3, array_accuracy3);
  dtostrf(min_co2_1, 4, 3, array_min_co2_1);
  dtostrf(min_co2_2, 4, 3, array_min_co2_2);
  dtostrf(min_co2_3, 4, 3, array_min_co2_3);
  dtostrf(time_1, 4, 3, array_time);

  // Формируем строку для записи
  snprintf(data_array, sizeof(data_array),
           "%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s",
           array_ppm1, array_ppm2, array_ppm3, array_medium_ppm,
           array_temp1, array_temp2, array_temp3, array_medium_temp,
           array_accuracy1, array_accuracy2, array_accuracy3,
           array_min_co2_1, array_min_co2_2, array_min_co2_3, array_time);

  // Записываем данные в файл
  appendFile(SD, path, data_array, true);

  // Очищаем массив
  for(int i = 0; i < 150; i++) {
    data_array[i] = ' ';
  }
}

char* converter_to_array(char* result, char command, int warming_time_s) {
  int size = 0;
  bool flag = false;
  if(warming_time_s>=1000){
    size = 4;
    flag = true;
  }
  if((warming_time_s>=100)&&(flag == false)){
    size = 3;
    flag = true;
  }
  if((warming_time_s>=10)&&(flag == false)){
    size = 2;
    flag = true;
  }
  if((warming_time_s>=0)&&(flag == false)){
    size = 1;
    flag = true;
  }

  for(int i = 1; i < 5; i++){
    result[i] = 0;
  }
  for (int i = 0; i < size; i++){
    char tmp = warming_time_s%10; 
    result[4 - i] = tmp;
    warming_time_s = (int) warming_time_s/10;    
  }
  result[0] = command;
  return(result);
}

int converter_to_number(char* array, int size) {
  int number = 0;
  for (int i=1; i<size; i++){
    number = number*10 + (int)array[i];
  }
  return(number);
}

void setup() {
  pinMode(rele1_pin, OUTPUT); // первый
  pinMode(rele2_pin, OUTPUT); // второй
  pinMode(rele3_pin, OUTPUT); // третий
  digitalWrite(rele1_pin, HIGH);
  digitalWrite(rele2_pin, HIGH); // поменять на HIGH с нормальным реле!
  digitalWrite(rele3_pin, HIGH);
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Wire.begin();
  if(!SD.begin()){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  appendFile(SD, path, "CO2_1;CO2_2;CO2_3;CO2_medium;Temp_1;Temp_2;Temp_3;Temp_medium;Accuracy_1;Accuracy_2;Accuracy_3;Min_CO2_1;Min_CO2_2;Min_CO2_3; time", true);
}

int data_1[5];
int data_2[5];
int data_3[5];
void loop() { // данные не пишутся на флешку перед прогревом. попробовать писать данные на флешку
  if(loop_counter == 0){// этот блок вызывается только на первой итерации цикла loop
    delay(first_warming_time*1000);
  }
  if(loop_counter == read_between_warm){
    bool first_reset = false;
    bool second_reset = false;
    bool third_reset = false;
    bool reset = false;
    if(data_1[4]==15){
      first_reset = true;
      reset = true;
    }
    if(data_2[4]==15){
      second_reset = true;
      reset = true;
    }
    if(data_3[4]==15){
      third_reset = true;
      reset = true;
    }
    if(reset == true){
      loop_counter = 0; 
      if(first_reset==true){
        digitalWrite(rele1_pin, LOW);
      } 
      if(second_reset==true){
        digitalWrite(rele2_pin, LOW);
      } 
      if(third_reset==true){
        digitalWrite(rele3_pin, LOW);
      }; 
      delay(10*1000);
      digitalWrite(rele1_pin, HIGH);
      digitalWrite(rele2_pin, HIGH);
      digitalWrite(rele3_pin, HIGH);
      delay(first_warming_time*1000);
    } else
    {
      write_sd_flag = true;
    }
    //проверка данных с датчика
    // если 15 по первому, то: first_reset=true, reset = true
    // если 15 по второму, то: second_reset=true, reset = true
    // если 15 по третьему, то: third_reset=true, reset = true, 
    // if(reset == true){ loop_counter = 0; if(first_reset==true){первое реле выключить}; if(second_reset==true){второе реле выключить}; if(third_reset==true){третье реле выключить}; delay(10*1000); все три реле снова включить;  delay(first_warming_time*1000);}
    // if(reset == false){break;}
  }
  switch(loop_counter) { // если loop_counter<9, читаем данные. если loop_counter = 10, то command = 5
  // if(loop_counter==11) command = 4 (здесь более развенуто); if(loop_counter==12) command = 2(более развернуто); 
    case read_between_warm: // 0
      read_co2 = false;
      command = 5; // прогрев
      //write_data_to_sd(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
      break;
    case (read_between_warm + 1): // 1 
      read_co2 = false;
      if(set_zero_flag){
        command = 4;
      }else{
        if(use_autocalibration){
          command = 2;
        }else{
          command = 3;
        }
      }
      break;
    case (read_between_warm + 2): // 2
      read_co2 = false;
      if(set_zero_flag){
        if(use_autocalibration){
          command = 2;
        }else{
          command = 3;
        }
      }else{
        read_co2 = true;
        command = 1;
      }
      break;
    default:
      read_co2 = true;
      command = 1;
  }

  // Создание запроса на ардуины
  char* message = new char[5];
  converter_to_array(message, command, warming_time); // элемент 1 - command. Результат записывается в message
  int size_send_message = 5;

  Serial.print("loop counter = ");
  Serial.println(loop_counter);

  // Отправка запроса на Ардуино 1
  Wire.beginTransmission(I2C_DEV_ADDR_1);
  for (int i = 0; i < size_send_message; i++){
    Wire.write(message[i]);
  }

  // Получение ответа от Ардуино 1
  while(Wire.endTransmission(true) != 0){
    Serial.println("Error 1");
  }
  uint8_t bytesReceived = Wire.requestFrom(I2C_DEV_ADDR_1, 9); // Чтение 9 байт с slave
  if (bytesReceived >= 9) {
    uint8_t co2_high = Wire.read();
    uint8_t co2_low = Wire.read();
    uint8_t temp = Wire.read();
    uint8_t accuracy = Wire.read();
    uint8_t min_co2_high = Wire.read();
    uint8_t min_co2_low = Wire.read();
    Wire.read(); Wire.read(); Wire.read(); // Пропускаем оставшиеся байты

    // Преобразуем данные
    int co2 = (co2_high << 8) | co2_low;
    int min_co2 = (min_co2_high << 8) | min_co2_low;

    // Обработка данных
    ppm_1_sum += co2;
    temp_1_sum += temp;
    accuracy_1 = accuracy;
    min_co2_1 = min_co2;
    counter_for_medium_1++;
  }

  // Отправка запроса на Ардуино 2
  Wire.beginTransmission(I2C_DEV_ADDR_2);
  for (int i = 0; i < size_send_message; i++){
    Wire.write(message[i]);
  }

  // Получение ответа от Ардуино 2
  while(Wire.endTransmission(true) != 0){
    Serial.println("Error 2");
  }
  bytesReceived = Wire.requestFrom(I2C_DEV_ADDR_2, 9); // Чтение 9 байт с slave
  if (bytesReceived >= 9) {
    uint8_t co2_high = Wire.read();
    uint8_t co2_low = Wire.read();
    uint8_t temp = Wire.read();
    uint8_t accuracy = Wire.read();
    uint8_t min_co2_high = Wire.read();
    uint8_t min_co2_low = Wire.read();
    Wire.read(); Wire.read(); Wire.read(); // Пропускаем оставшиеся байты

    // Преобразуем данные
    int co2 = (co2_high << 8) | co2_low;
    int min_co2 = (min_co2_high << 8) | min_co2_low;

    // Обработка данных
    ppm_2_sum += co2;
    temp_2_sum += temp;
    accuracy_2 = accuracy;
    min_co2_2 = min_co2;
    counter_for_medium_2++;
  }

  // Отправка запроса на Ардуино 3
  Wire.beginTransmission(I2C_DEV_ADDR_3);
  for (int i = 0; i < size_send_message; i++){
    Wire.write(message[i]);
  }

  // Получение ответа от Ардуино 3
  while(Wire.endTransmission(true) != 0){
    Serial.println("Error 3");
  }
  bytesReceived = Wire.requestFrom(I2C_DEV_ADDR_3, 9); // Чтение 9 байт с slave
  if (bytesReceived >= 9) {
    uint8_t co2_high = Wire.read();
    uint8_t co2_low = Wire.read();
    uint8_t temp = Wire.read();
    uint8_t accuracy = Wire.read();
    uint8_t min_co2_high = Wire.read();
    uint8_t min_co2_low = Wire.read();
    Wire.read(); Wire.read(); Wire.read(); // Пропускаем оставшиеся байты

    // Преобразуем данные
    int co2 = (co2_high << 8) | co2_low;
    int min_co2 = (min_co2_high << 8) | min_co2_low;

    // Обработка данных
    ppm_3_sum += co2;
    temp_3_sum += temp;
    accuracy_3 = accuracy;
    min_co2_3 = min_co2;
    counter_for_medium_3++;
  }

  // Усреднение данных
  if((counter_for_medium_1 >= 5) && (counter_for_medium_2 >= 5) && (counter_for_medium_3 >= 5)) {
    ppm_1_medium = ppm_1_sum / counter_for_medium_1;
    temp_1_medium = temp_1_sum / counter_for_medium_1;
    ppm_1_sum = 0;
    temp_1_sum = 0;
    counter_for_medium_1 = 0;

    ppm_2_medium = ppm_2_sum / counter_for_medium_2;
    temp_2_medium = temp_2_sum / counter_for_medium_2;
    ppm_2_sum = 0;
    temp_2_sum = 0;
    counter_for_medium_2 = 0;

    ppm_3_medium = ppm_3_sum / counter_for_medium_3;
    temp_3_medium = temp_3_sum / counter_for_medium_3;
    ppm_3_sum = 0;
    temp_3_sum = 0;
    counter_for_medium_3 = 0;

    ppm_common = (ppm_1_medium + ppm_2_medium + ppm_3_medium) / 3;
    temp_common = (temp_1_medium + temp_2_medium + temp_3_medium) / 3;
    
    if(loop_counter < read_between_warm){
      data_1[(int)(loop_counter/5)] = ppm_1_medium;
      data_2[(int)(loop_counter/5)] = ppm_2_medium;
      data_3[(int)(loop_counter/5)] = ppm_3_medium;
    }
    if(write_sd_flag){
      write_data_to_sd(ppm_1_medium, ppm_2_medium, ppm_3_medium, ppm_common,
                     temp_1_medium, temp_2_medium, temp_3_medium, temp_common,
                     accuracy_1, accuracy_2, accuracy_3,
                     min_co2_1, min_co2_2, min_co2_3, time_s);
      time_s = time_s + time_step;
    }

    // Запись данных на SD-карту
    /*write_data_to_sd(ppm_1_medium, ppm_2_medium, ppm_3_medium, ppm_common,
                     temp_1_medium, temp_2_medium, temp_3_medium, temp_common,
                     accuracy_1, accuracy_2, accuracy_3,
                     min_co2_1, min_co2_2, min_co2_3, time_s);*/
  }

  Serial.println();
  if(loop_counter == read_between_warm){
    delay((warming_time + 1) * 1000);    
  } else if(read_co2 == true){
    delay(delay_between_readings);
  } else{
    delay(3000);
  }
  loop_counter++;
}