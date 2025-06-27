#include "Wire.h"
#include <SoftwareSerial.h>
#define MHZ19B_TX_PIN        11 //8 // 4//11(main)
#define MHZ19B_RX_PIN        10 //9 // 5//10(main)

SoftwareSerial mhzSerial(MHZ19B_TX_PIN, MHZ19B_RX_PIN);

// 0x09 - первая
// 0x55 - вторая
#define I2C_DEV_ADDR 0x55 //0x09 - первая, 0x55 - вторая, 0x11 - третья


int warming_time = 0;
int ppm = 0;
int temperature = 0; // Температура
int accuracy = 0;    // Точность
int min_co2 = 0;     // Минимальное значение CO2
int command = 0;
char res = 0;

// Объявление функции converter_to_number
int converter_to_number(char* array, int size);

void onRequest() {
  // Отправляем все данные: CO2, температура, точность и минимальное значение CO2
  int size = 9; // 9 байт для всех данных
  char message[9];

  // CO2 (2 байта)
  message[0] = (ppm >> 8) & 0xFF; // Старший байт CO2
  message[1] = ppm & 0xFF;        // Младший байт CO2

  // Температура (1 байт)
  message[2] = temperature;

  // Точность (1 байт)
  message[3] = accuracy;

  // Минимальное значение CO2 (2 байта)
  message[4] = (min_co2 >> 8) & 0xFF; // Старший байт min_co2
  message[5] = min_co2 & 0xFF;        // Младший байт min_co2

  // Оставшиеся байты (можно использовать для других данных или оставить нулями)
  message[6] = 0;
  message[7] = 0;
  message[8] = 0;

  // Отправляем данные
  for (int i = 0; i < size; i++) {
    Wire.write(message[i]);
  }
  Serial.println();
}

int readCO2() {
  byte cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
  byte response[9];
  mhzSerial.write(cmd, sizeof(cmd));
  if (mhzSerial.available() >= 9) {
    mhzSerial.readBytes(response, 9);
    if (response[0] == 0xFF && response[1] == 0x86) {
      // CO2 (2 байта)
      ppm = response[2] * 256 + response[3];

      // Температура (1 байт)
      temperature = response[4] - 40;

      // Точность (1 байт)
      accuracy = response[5];

      // Минимальное значение CO2 (2 байта)
      min_co2 = response[6] * 256 + response[7];

      Serial.print("CO2: ");
      Serial.print(ppm);
      Serial.print(" ppm, Temp: ");
      Serial.print(temperature);
      Serial.print(" °C, Accuracy: ");
      Serial.print(accuracy);
      Serial.print(", Min CO2: ");
      Serial.print(min_co2);
      Serial.println(" ppm");

      return ppm;
    }
  } else {
    Serial.println("No response from sensor.");
    return -1;
  }
}

byte calculateChecksum(byte* data) {
  byte checksum = 0;
  for (int i = 1; i < 8; i++) {
    checksum += data[i];
  }
  return 0xFF - checksum + 1;
}

void setZeroPoint() {
  byte command[9] = {0xFF, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  command[8] = calculateChecksum(command);
  mhzSerial.write(command, 9);
}

void useAutocalibration(bool flag) {
  byte command[9] = {0xFF, 0x01, 0x87, 0x79, (flag ? 0xA0 : 0x00), 0x00, 0x00, 0x00, 0x00};
  command[8] = calculateChecksum(command);
  mhzSerial.write(command, 9);
}

void onReceive(int len) {
  int size_result = 8;
  char result[size_result];
  Wire.readBytes(result, size_result);
  command = (int)result[0];
  
  int tmp = 0; // Объявляем переменную tmp вне switch-case

  switch (command) {
    case 1: // Считать данные CO2 и температуру
      tmp = readCO2();
      if (tmp >= 0) {
        ppm = tmp;
        res = 0;
      } else {
        res = 1;
      }
      break;
    case 2: // Включить автокалибровку
      useAutocalibration(true);
      res = 0;
      break;
    case 3: // Выключить автокалибровку
      useAutocalibration(false);
      res = 0;
      break;
    case 4: // Применить метод SetZeroPoint
      setZeroPoint();
      res = 0;
      break;
    case 5: // Установить время прогрева
      warming_time = converter_to_number(result, size_result);
      res = 0;
      break;
    default:
      // Ничего не делаем
      break;
  }

  int sensor_rele = (int)result[5];
  int led_rele = (int)result[6];
  int kuler_rele = (int)result[7];
  //Serial.print(sensor_rele);
 // Serial.println(" rele");
  if(sensor_rele==22){
    digitalWrite(2, HIGH);
    digitalWrite(3, HIGH);
    /*delay(8000);
    digitalWrite(2, LOW);
    digitalWrite(3, LOW);*/
  }
  if(sensor_rele==21){
    digitalWrite(2, HIGH);
    digitalWrite(3, LOW);
    //delay(8000);
    //digitalWrite(2, LOW);
  }
  if(sensor_rele==12){
    digitalWrite(3, HIGH);
    digitalWrite(2, LOW);
    //delay(8000);
    //digitalWrite(3, LOW);
  }
  if((sensor_rele==11)||(sensor_rele==0)){
    digitalWrite(2, LOW);
    digitalWrite(3, LOW);
  }

  if(led_rele==2){
    digitalWrite(4, HIGH);
  }
  if((led_rele==0)||(led_rele==1)){
    digitalWrite(4, LOW);
  }

  if(kuler_rele==2){
    digitalWrite(5, HIGH);
  }
  if((kuler_rele==0)||(led_rele==1)){
    digitalWrite(5, LOW);
  }
}

// Определение функции converter_to_number
int converter_to_number(char* array, int size) {
  int number = 0;
  for (int i = 1; i < size; i++) {
    number = number * 10 + (int)array[i];
  }
  return number;
}

void setup() {
  Serial.begin(115200);
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
  Wire.begin((uint8_t)I2C_DEV_ADDR);
  mhzSerial.begin(9600);
  pinMode(2, OUTPUT); // первый датчик реле
  pinMode(3, OUTPUT); // второй датчик реле
  pinMode(4, OUTPUT); // светодиод реле
  pinMode(5, OUTPUT); // вентилятор реле
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
}

void loop() {
}