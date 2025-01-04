// это основная версия кода для Ардуино
#include "Wire.h"
#include <SoftwareSerial.h>
//#include <CO2.h>

#define MHZ19B_TX_PIN        11 //8 // 4
#define MHZ19B_RX_PIN        10 //9 // 5
 // rx к rx, tx к tx!!
SoftwareSerial mhzSerial(MHZ19B_TX_PIN, MHZ19B_RX_PIN);
//CO2 mhz19b(&mhzSerial);

// 0x09 - первая
// 0x55 - вторая
#define I2C_DEV_ADDR 0x09 //0x09 - первая, 0x55 - вторая, 0x11 - третья

int warming_time = 0;
int ppm = 0;
int command = 0;
char res = 0;


char* converter_to_array(char* result, char error, int ppm) // конвертация числа типа int в массив типа char. обрабатывает значения, не более 9999 
{
  int size = 0;
  bool flag = false;
  if(ppm>=1000){
    size = 4;
    flag = true;
  }
  if((ppm>=100)&&(flag == false)){
    size = 3;
    flag = true;
  }
  if((ppm>=10)&&(flag == false)){
    size = 2;
    flag = true;
  }
  if((ppm>=0)&&(flag == false)){
    size = 1;
    flag = true;
  }

  
  for(int i = 1; i < 5; i++){
    result[i] = 0;
  }
  
  for (int i = 0; i < size; i++){
    char tmp = ppm%10; 
    result[4 - i] = tmp;
    ppm = (int) ppm/10;  
  }
  result[0] = error;
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


void onRequest() { // отправляет ответ на есп32
  // ppm - глобпльная переменная, причем измерение CO2 происходит в onReceive. Тогда все должно хорошо работать
  int size = 5;
  //char error = 255; // =0, значит все хорошо. Если больше 0, то есть какая-то ошибка при чтении
  // измерение концентрации Co2 однократно
  // внутри измерения CO2, если произошла ошибка, то error > 0, если все хорошо, то error = 0
  //error=0; // перенести это в блок измерения CO2
  char* message = new char[5];
  converter_to_array(message, res, ppm);
  for (int i = 0; i < size; i++){
    Wire.write(message[i]);
  }
  /*if(command==5){ // Возможно, вернуть
    delay(warming_time*1000); // т.е задержка только после того, как отправили сообщение об успешном приеме команды
  }*/
  delete[] message;
  Serial.println();
}

int readCO2() {
    byte cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79}; // Команда для чтения CO2
    byte response[9]; // Массив для ответа
    mhzSerial.write(cmd, sizeof(cmd)); // Отправляем команду
    if (mhzSerial.available() >= 9) { // Проверяем наличие данных
        mhzSerial.readBytes(response, 9); // Читаем ответ
        // Проверка контрольной суммы
        if (response[0] == 0xFF && response[1] == 0x86) {
            int co2ppm = response[2] * 256 + response[3]; // Концентрация CO2 в ppm
            //Serial.print("CO2 Concentration: ");
            Serial.print(co2ppm);
            Serial.println(" ppm");
            return(co2ppm); // Успешное считывание
        }
    } else {
        Serial.println("No response from sensor.");
        return(-1); // Ошибка считывания
    }
}


byte calculateChecksum(byte* data) {
  byte checksum = 0;
  for (int i = 1; i < 8; i++) { // Считаем контрольную сумму для первых 8 байт
    checksum += data[i];
  }
  return 0xFF - checksum + 1; // Контрольная сумма
}

void setZeroPoint() {
  byte command[9] = {0xFF, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  command[8] = calculateChecksum(command); // Вычисляем контрольную сумму
  mhzSerial.write(command, 9);
}


void useAutocalibration(bool flag) {
  byte command[9] = {0xFF, 0x01, 0x87, 0x79, (flag ? 0xA0 : 0x00), 0x00, 0x00, 0x00, 0x00};
  command[8] = calculateChecksum(command);
  mhzSerial.write(command, 9);
}


void onReceive(int len) { // принимает информацию с есп32
  int size_result = 5;
  char result[size_result];
  Wire.readBytes(result, size_result);
  command = (int)result[0];
  switch(command) {
    case 1: // считать данные CO2 с датчика одлнократно
      int tmp = readCO2(); //mhz19b.readCO2();
      if(tmp>=0){
        ppm = tmp;
        res = 0; // 0 - значит все хорошо
      } else{
        res = 1;
      }
      break;
    case 2: // включить автокалибровку
      useAutocalibration(true);
      res = 0;
      break;
    case 3: // выключить автокалибровку
      useAutocalibration(false);
      res = 0;
      break;
    case 4: // применить метод SetZeroPoint
      setZeroPoint();
      res = 0;
      break;
    case 5: // установить время прогрева и прогреть датчик (код декодирования массива сюда перетащить)
      warming_time = converter_to_number(result, size_result);
      res = 0;
      break;
    default: ;
}
}

void setup() {  
  Serial.begin(115200);
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
  Wire.begin((uint8_t)I2C_DEV_ADDR); // именно так!
  mhzSerial.begin(9600);
}

void loop() {
  
}