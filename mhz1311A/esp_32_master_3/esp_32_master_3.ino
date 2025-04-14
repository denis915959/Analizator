#include "Wire.h"

#define I2C_DEV_ADDR_1 0x09
#define I2C_DEV_ADDR_2 0x55
#define I2C_DEV_ADDR_3 0x11
#include <FS.h>
#include <SD.h>
#include <SPI.h>

char* path = "/data.txt";
char command = -1;
int warming_time = 180; // время прогрева(в секундах)
int loop_counter = 0;
bool set_zero_flag = true; // если true, то будет использоваться СетЗеро, если false, то не будет 
bool use_autocalibration = false; // если true, то будет использоваться Автокалибровка, если false, то не будет 
bool read_co2 = false; // флаг для работы программы
int delay_between_readings = 150; // при задержке 100 мс частота измерений 1.37 Гц.
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


void write_data_to_sd(float ppm_1, float ppm_2, float ppm_3, float medium){
  static char data_array[50];
  static char array_ppm1[9];
  static char array_ppm2[9];
  static char array_ppm3[9];
  static char array_medium[9];

  int size_ppm1 = get_float_size(ppm_1);
  int size_ppm2 = get_float_size(ppm_2);
  int size_ppm3 = get_float_size(ppm_3);
  int size_medium = get_float_size(medium);

  dtostrf(ppm_1, 4, 3, array_ppm1);
  dtostrf(ppm_2, 4, 3, array_ppm2);
  dtostrf(ppm_3, 4, 3, array_ppm3);
  dtostrf(medium, 4, 3, array_medium);
  int counter = 0;
  for(int i=0; i < (size_ppm1 + 4); i++){
    data_array[counter] = array_ppm1[i];
    counter++;
  }
  data_array[counter] = ';';
  counter++;
  data_array[counter] = ' ';
  counter++;
  data_array[counter] = ' ';
  counter++;
  data_array[counter] = ' ';
  counter++;

  for(int i=0; i < (size_ppm2 + 4); i++){
    data_array[counter] = array_ppm2[i];
    counter++;
  }
  data_array[counter] = ';';
  counter++;
  data_array[counter] = ' ';
  counter++;
  data_array[counter] = ' ';
  counter++;
  data_array[counter] = ' ';
  counter++;

  for(int i=0; i < (size_ppm3 + 4); i++){
    data_array[counter] = array_ppm3[i];
    counter++;
  }
  data_array[counter] = ';';
  counter++;
  data_array[counter] = ' ';
  counter++;
  data_array[counter] = ' ';
  counter++;
  data_array[counter] = ' ';
  counter++;

  for(int i=0; i < (size_medium + 4); i++){
    data_array[counter] = array_medium[i];
    counter++;
  }
  data_array[counter] = ';';
  counter++;
  for(int i=counter; i < 40; i++){
    data_array[i] = ' ';
  }
  
  appendFile(SD, path, data_array, true);

  for(int i=0; i < 50; i++){
    data_array[i] = ' ';
  }
}


char* converter_to_array(char* result, char command, int warming_time_s) // конвертация числа типа int в массив типа char. обрабатывает значения, не более 9999 
{
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

int converter_to_number(char* array, int size)
{
  int number = 0;
  for (int i=1; i<size; i++){
    number = number*10 + (int)array[i];
  }
  return(number);
}

void setup() {
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

  appendFile(SD, path, "     1         2          3         medium", true);  // создает файл и пишет в него данные типа char
}

void loop() {
  switch(loop_counter) { // свитч убрать, сделать глобальные флаги. и проверять по флагам. 
    case 0:
      command = 5;
      break;
    case 1: 
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
    case 2:
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
  //создание запроса на ардуины
  char* message = new char[5];
  converter_to_array(message, command, warming_time); // элемент 1 - command. Результат записывается в message
  int size_send_message = 5;

  // Отправка запроса на Ардуино 1
  Wire.beginTransmission(I2C_DEV_ADDR_1);
  //Serial.print("command: "); 
  for (int i = 0; i < size_send_message; i++){
    Wire.write(message[i]);
    //Serial.print((int)message[i]); // если не приводить char к типу int, то будут просто печататься квадратики
    //Serial.print("; ");
  }
  //Serial.println();

  //получение ответа от Ардуино 1
  while(Wire.endTransmission(true)!=0){ // пока есть ошибка соединения по i2c
    Serial.println("Error 1");
  }
  uint8_t bytesReceived = Wire.requestFrom(I2C_DEV_ADDR_1, 5); // Чтение 5 байт с slave
  
  //Serial.print("requestFrom: ");
  int res_1 = -1;
  int ppm_1_tmp = 0;
  if (bytesReceived > 0) {  //если принято больше, чем 0 байт
    char* temp = new char[bytesReceived];
    Wire.readBytes(temp, bytesReceived);
    /*for(int i=0; i<bytesReceived; i++)
    {
      Serial.print((int)temp[i]);
      Serial.print("; "); 
    }
    Serial.println(); */
    ppm_1_tmp = converter_to_number(temp, bytesReceived);
    //Serial.println(ppm_1_tmp);
    res_1 = (int)temp[0]; // результат работы ардуино. 0 - успех, 1 - ошибка 
    delete[] temp;
  }


  // Отправка запроса на Ардуино 2
  Wire.beginTransmission(I2C_DEV_ADDR_2);
  for (int i = 0; i < size_send_message; i++){
    Wire.write(message[i]);
  }
  
  //получение ответа от Ардуино 2
  while(Wire.endTransmission(true)!=0){ // пока есть ошибка соединения по i2c
    Serial.println("Error 2");
  }
  bytesReceived = Wire.requestFrom(I2C_DEV_ADDR_2, 5); // Чтение 5 байт с slave
  
  //Serial.print("requestFrom 2: ");
  int res_2 = -1;
  int ppm_2_tmp = 0;
  if (bytesReceived > 0) {  //если приято больше, чем 0 байт, изменить эту проверку на нормальную 
    char* temp = new char[bytesReceived];
    Wire.readBytes(temp, bytesReceived);
    /*for(int i=0; i<bytesReceived; i++)
    {
      Serial.print((int)temp[i]);
      Serial.print("; "); 
    }
    Serial.println();*/
    ppm_2_tmp = converter_to_number(temp, bytesReceived);
    //Serial.println(ppm_2_tmp);
    res_2 = (int)temp[0]; // результат работы ардуино. 0 - успех, 1 - ошибка 
    delete[] temp;
  }

  // Отправка запроса на Ардуино 3
  Wire.beginTransmission(I2C_DEV_ADDR_3);
  for (int i = 0; i < size_send_message; i++){
    Wire.write(message[i]);
  }
  delete[] message; // при добавлении четвертой ардуины это надо убравть
  

  //получение ответа от Ардуино 3
  while(Wire.endTransmission(true)!=0){ // пока есть ошибка соединения по i2c
    Serial.println("Error 3");
  }
  bytesReceived = Wire.requestFrom(I2C_DEV_ADDR_3, 5); // Чтение 5 байт с slave
  
  //Serial.print("requestFrom 3: ");
  int res_3 = -1;
  int ppm_3_tmp = 0;
  if (bytesReceived > 0) {  //если приято больше, чем 0 байт, изменить эту проверку на нормальную 
    char* temp = new char[bytesReceived];
    Wire.readBytes(temp, bytesReceived);
    /*for(int i=0; i<bytesReceived; i++)
    {
      Serial.print((int)temp[i]);
      Serial.print("; "); 
    }
    Serial.println();*/
    ppm_3_tmp = converter_to_number(temp, bytesReceived);
    //Serial.println(ppm_3_tmp);
    res_3 = (int)temp[0]; // результат работы ардуино. 0 - успех, 1 - ошибка 
    delete[] temp;
  }


  // обработка ответа от всех ардуино
  if((read_co2 == true)&&(res_1==1)){
    Serial.println("Нужен перезапрос по первому датчику");
  }else{
    ppm_1_sum+=ppm_1_tmp;
    counter_for_medium_1 ++;
  }
  if((read_co2 == true)&&(res_2==1)){
    Serial.println("Нужен перезапрос по второму датчику");
  }else{
    ppm_2_sum+=ppm_2_tmp;
    counter_for_medium_2 ++;
  }
  if((read_co2 == true)&&(res_3==1)){
    Serial.println("Нужен перезапрос по третьему датчику");
  }else{
    ppm_3_sum+=ppm_3_tmp;
    counter_for_medium_3 ++;
  }

  if((counter_for_medium_1 >= 5)&&(counter_for_medium_2 >= 5)&&(counter_for_medium_3 >= 5)){ // если по каждому датчику минимум 5 чтений, то усредняем
    ppm_1_medium = ppm_1_sum/counter_for_medium_1;
    counter_for_medium_1 = 0;
    ppm_1_sum = 0;
    Serial.print("ppm_1_medium end = ");
    Serial.println(ppm_1_medium);

    ppm_2_medium = ppm_2_sum/counter_for_medium_2;
    counter_for_medium_2 = 0;
    ppm_2_sum = 0;
    Serial.print("ppm_2_medium end = ");
    Serial.println(ppm_2_medium);

    ppm_3_medium = ppm_3_sum/counter_for_medium_3;
    counter_for_medium_3 = 0;
    ppm_3_sum = 0;
    Serial.print("ppm_3_medium end = ");
    Serial.println(ppm_3_medium);

    ppm_common = (ppm_1_medium + ppm_2_medium + ppm_3_medium)/3;
    Serial.print("ppm_common = ");
    Serial.println(ppm_common);
    write_data_to_sd(ppm_1_medium, ppm_2_medium, ppm_3_medium, ppm_common);

  }

  Serial.println();
  if(loop_counter==0){
    delay((warming_time+1)*1000);    
  } else if(read_co2 == true){
    delay(delay_between_readings);
  } else{
    delay(3000);
  }
  loop_counter++;
}