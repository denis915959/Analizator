#include <SoftwareSerial.h>

#include <ErriezMHZ19B.h>

 #define MHZ19B_TX_PIN        11 //8 // 4
 #define MHZ19B_RX_PIN        10 //9 // 5
 // rx к rx, tx к tx!!

 #include <SoftwareSerial.h>          // Use software serial
 SoftwareSerial mhzSerial(MHZ19B_TX_PIN, MHZ19B_RX_PIN);

// Create MHZ19B object
ErriezMHZ19B mhz19b(&mhzSerial);
long int warming_time = 180000; //600000

void setup()
{
    // Initialize serial
  Serial.begin(115200);

    // Initialize software serial at fixed baudrate
  mhzSerial.begin(9600); // 9600

  Serial.println("Start Warming"); // Warm - червь
  delay(warming_time);
  Serial.println("End Warming");

  
  Serial.print("Zero calibration result: "); // возвращает 0
  Serial.println(mhz19b.startZeroCalibration());    
  mhz19b.setAutoCalibration(true);
}

void loop()
{
  int16_t result_1;
  // Minimum interval between CO2 reads
  if (mhz19b.isReady()) {
    // Read and print CO2
    result_1 = mhz19b.readCO2();
    Serial.print(result_1);
    Serial.println(F(" ppm"));
  }
}