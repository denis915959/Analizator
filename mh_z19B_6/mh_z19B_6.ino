// это рабочая версия кода на конец 11 октября
#include <CO2.h>
#include <SPI.h>
#include <SD.h>
File myFile;

//скорость порта 115200

// Пин 11 Arduino к MOSI микро-SD карты
// Пин 12 Arduino к MISO микро-SD карты
// Пин 13 Arduino к SCK микро-SD карты
// Пин 10 Arduino к CS (Chip Select) микро-SD карты

#define MHZ19B_TX_PIN        8 //10 //8 // 4
#define MHZ19B_RX_PIN        9 //11 //9 // 5

#define MHZ19B_TX_PIN_2        6 //8
#define MHZ19B_RX_PIN_2        7 //9

#define MHZ19B_TX_PIN_3        4
#define MHZ19B_RX_PIN_3        5

// rx к rx, tx к tx!!

#include <SoftwareSerial.h>          // Use software serial

int sensor_number = 0;
float result_1 = 0;
float result_2 = 0;
float result_3 = 0;
float medium;
long int warming_time = 180000;
int repeat = 5;

void setup()
{
    // Initialize serial
  Serial.begin(115200);
  Serial.println("Start Warming!");
  delay(warming_time);
  Serial.println("End Warming!");

  if (!SD.begin(10)) {
    Serial.println("Ошибка инициализации SD-карты");
    return;
  }
  Serial.println("SD-карта инициализирована");
  
  myFile = SD.open("data.txt", FILE_WRITE);
  if (myFile) {
    myFile.println("sensor1; sensor2; sensor3; medium"); // это заголовок, чтобы при расшифровке было понятнее что есть что
    myFile.close(); // Закрываем файл
  } else {
    Serial.println("Ошибка открытия файла");
  }
}

void loop()
{
  if(sensor_number == 0){
    SoftwareSerial mhzSerial(MHZ19B_TX_PIN, MHZ19B_RX_PIN);
    mhzSerial.begin(9600);
     // Create MHZ19B object
    CO2 mhz19b(&mhzSerial);
    mhz19b.setAutoCalibration(false);
    int i = 0;
    int sum = 0;
    while(i < repeat){ // надо несколько измерений именно с созданным каналом передачи. Иначе хрень показывает (если канал постоянно пересоздавать)
    // Minimum interval between CO2 reads
      if (mhz19b.isReady()) {
        // Read and print CO2
        int tmp = mhz19b.readCO2();
        sum = sum + tmp;
        i++;
      }
    }
    result_1 = sum/repeat;
    Serial.print(result_1);
    Serial.println(F(" ppm  1 sensor"));
  } 
  if(sensor_number == 1){
    SoftwareSerial mhzSerial_2(MHZ19B_TX_PIN_2, MHZ19B_RX_PIN_2);
    mhzSerial_2.begin(9600);
    CO2 mhz19b_2(&mhzSerial_2);
    mhz19b_2.setAutoCalibration(false);
    int i = 0;
    int sum = 0;
    while(i < repeat){
    // Minimum interval between CO2 reads
      if (mhz19b_2.isReady()) {
        // Read and print CO2
        sum = sum + mhz19b_2.readCO2();
        i++;
      }
    }
    result_2 = sum/repeat;
    Serial.print(result_2);
    Serial.println(F(" ppm  2 sensor"));
  }
  if(sensor_number == 2){
    SoftwareSerial mhzSerial_3(MHZ19B_TX_PIN_3, MHZ19B_RX_PIN_3);
    mhzSerial_3.begin(9600);
    CO2 mhz19b_3(&mhzSerial_3);
    mhz19b_3.setAutoCalibration(false);
    int i = 0;
    int sum = 0;
    while(i < repeat){
    // Minimum interval between CO2 reads
      if (mhz19b_3.isReady()) {
        // Read and print CO2
        sum = sum + mhz19b_3.readCO2();
        i++;
      }
    }
    result_3 = sum/repeat;
    Serial.print(result_3);
    Serial.println(F(" ppm  3 sensor"));
    medium = (result_1 + result_2 + result_3)/3;
    Serial.print("medium = ");
    Serial.print(medium);
    Serial.println(" ppm");
    Serial.println(" ");

    myFile = SD.open("data.txt", FILE_WRITE);
    if (myFile) { 
      myFile.print(result_1);  //далее данные в файл пишем
      myFile.print(";  ");
      myFile.print(result_2);
      myFile.print(";  ");
      myFile.print(result_3);
      myFile.print(";  ");
      myFile.println(medium);
      myFile.close(); // Закрываем файл
    } else {
      Serial.println("Ошибка открытия файла");
    }

    medium = 0;
    result_1 = 0;
    result_2 = 0;
    result_3 = 0;
  }
  sensor_number = (sensor_number + 1)%3;
}