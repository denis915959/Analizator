#include <SoftwareSerial.h>

//#include <ErriezMHZ19B.h>
#include <CO2.h>

 #define MHZ19B_TX_PIN        11 //8 // 4
 #define MHZ19B_RX_PIN        10 //9 // 5
 // rx к rx, tx к tx!!

 #include <SoftwareSerial.h>          // Use software serial
 SoftwareSerial mhzSerial(MHZ19B_TX_PIN, MHZ19B_RX_PIN);

// Create MHZ19B object
//ErriezMHZ19B mhz19b(&mhzSerial);
CO2 mhz19b(&mhzSerial);
long int warming_time = 180000; //600000
int repeat = 5; // даже если 2 раза будет осечка, программа уложится в 0.9



byte calculateChecksum(byte* data) {
  byte checksum = 0;
  for (int i = 1; i < 8; i++) { // Считаем контрольную сумму для первых 8 байт
    checksum += data[i];
  }
  return 0xFF - checksum + 1; // Контрольная сумма
}

void setZeroPoint() {
  Serial.println("Установка нулевой точки...");
  
  // Пример команды для установки нулевой точки
  // Команда может отличаться в зависимости от документации
  byte command[9] = {0xFF, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  
  // Вычисляем контрольную сумму
  command[8] = calculateChecksum(command);
  
  // Отправляем команду на установку нулевой точки
  mhzSerial.write(command, 9);
  
  Serial.println("Команда на установку нулевой точки отправлена.");
}



void setup()
{
    // Initialize serial
  Serial.begin(115200);

    // Initialize software serial at fixed baudrate
  mhzSerial.begin(9600); // 9600

  Serial.println("Start Warming"); // Warm это червь
  //delay(warming_time);
  Serial.println("End Warming");

  
 // Serial.print("Zero calibration result: "); // возвращает 0
  //Serial.println(mhz19b.startZeroCalibration());
  setZeroPoint(); 
  mhz19b.setAutoCalibration(true);
}

void loop()
{
  int16_t result;
  // Minimum interval between CO2 reads
  int i = 0;
  int16_t sum = 0;
  while(i < repeat){
  // Minimum interval between CO2 reads
    if (mhz19b.isReady()) {
      int16_t tmp = mhz19b.readCO2();
      if(tmp>=0){
        sum = sum + tmp;
        i++;
      }
    }
  }
  result = sum/repeat;
  Serial.print(result);
  Serial.println(F(" ppm"));
  delay(800);

  /*if (mhz19b.isReady()) {
    // Read and print CO2
    result_1 = mhz19b.readCO2();
    Serial.print(result_1);
    Serial.println(F(" ppm"));
  }*/
}