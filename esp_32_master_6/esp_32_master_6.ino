#include "Wire.h"
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <ESP8266_LCD_1602_RUS.h>
#include <font_LCD_1602_RUS.h>

#define I2C_DEV_ADDR_1 0x09
#define I2C_DEV_ADDR_2 0x55
//#define I2C_DEV_ADDR_3 0x11
const int rele1_pin = 0;
const int rele2_pin = 2;
const int rele3_pin = 4;

char* path = "/data.txt";
char command = -1;
const int warming_time = 30; //300; // время прогрева(в секундах)
int rele_open_time = 4000;//7000  // это время размыкания реле при ошибке 15
const int first_warming_time = 10; //10 - все работает;
int loop_counter = 0;
bool set_zero_flag = true; // если true, то будет использоваться СетЗеро, если false, то не будет 
bool use_autocalibration = false; // если true, то будет использоваться Автокалибровка, если false, то не будет 
bool led_between_warm = true; // true - светодиод включается после проверки устройства на весь период прогрева. false - включается сразу после прогрева
const int led_time = 450;// 450 // время работы светодиода в секундах
bool read_co2 = false; // флаг для работы программы
int delay_between_readings = 161; //130; //150(добавился экран и это замедляет код); // при задержке 100 мс частота измерений 1.37 Гц.
const int read_between_warm = 25; // 50
const int delay_in_command0 = 960; // 1 секунда в цикле прогрева

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
float time_step = 1.000; //0.987;

int led_time_counter = 0; // счетчик времени работы светодиода 

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

// флаги для отправки состояний реле на ардуино нано
char sensor_rele = 0;
char led_rele = 0; // 0 - не инициализировано, 1 - выключено, 2 - включено
char kuler_rele = 0; // 0 - не инициализировано, 1 - выключено, 2 - включено
const bool led_before_warm = false; // true - светодиод включается при прогреве, false - светодиод включается после прогрева



struct LedCorrection { // структура данных для хранения аппроксимации падения напряжения от светодода
  float led;
  float k; // коэффициент коррекции
};

// этот класс нужен для вынесения логики, связанной с с расчетом процентов и постобработкой данных с делителя 12В, за пределы класса Display
class Charge{ // класс для считывания данных с делителя (с постобработкой данных с делителя) и конвертации усредненного значения в проценты.
  private:
    const int charge_pin_12 = 33; // пин делителя 12В 
    const int charge_pin_led = 35; // пин делителя светодиода 
    int low_border = 2067; //нижняя граница (8.5 В)
    int high_border = 3075; //3065;//3050;//3111; //верхняя граница (ее надо увеличить после подключения светодиода)
    //float volt_1 = (high_border - low_border)/100; // 3.9
    /*static const int tableSize = 6; // static const позволяет использовать значение на этапе компиляции
    const Voltage_Percent voltage_percent_list[tableSize] = {
      {12.6, 100.0}, // 12.6
      {11.8,  80.0}, // 12
      {11.2,  60.0},
      {10.8,  40.0},
      {10.3,  20.0},
      {8.5,   0.0}
    };*/
    static const int pointsCount = 4; // кол-во точек в led_correction
    const LedCorrection approximation[pointsCount] = {
      {0.0, 0.0},
      {1500.0, 0.0038},
      {1670.0, 0.009/*0.0115*/},
      {2410.0, 0.018/*0.0228*/}
    };

    float get_correction_led(int charge_led) { // возвращает коэффициэнт коррекции для светодиода (через линейную аппроксимацию)
      if (charge_led <= approximation[0].led) return approximation[0].k; // Если charge_led меньше минимального значения, берём первую точку
      if (charge_led >= approximation[pointsCount - 1].led) return approximation[pointsCount - 1].k; // Если y больше максимального, берём последнюю точку
  
      // Ищем интервал, в который попадает charge_led
      for (int i = 0; i < pointsCount - 1; i++) {
        if (charge_led >= approximation[i].led && charge_led <= approximation[i + 1].led) {
          // Линейная интерполяция: k = k1 + (k2 - k1) * (charge_led - led1) / (led2 - led1)
          float led1 = approximation[i].led;
          float led2 = approximation[i + 1].led;
          float k1 = approximation[i].k;
          float k2 = approximation[i + 1].k;
          return (k1 + (k2 - k1) * (charge_led - led1) / (led2 - led1));
        }
      }
      return 0.0; // на случай ошибки
    }

  public:
  Charge(){
    pinMode(charge_pin_12, INPUT);
    pinMode(charge_pin_led, INPUT);
  }

  int get_delitel_12(){ // вывод данных с делителя 12В, сюда добавить пересчет данных с делителя 12В в зависимости от данных с делителя светодиода
    int charge = analogRead(charge_pin_12); 
    int charge_led = analogRead(charge_pin_led); // читаем данные с делителя напряжения светодиода
    float k = get_correction_led(charge_led);
    int result = (int)(charge*(1-(approximation[pointsCount-1].k - k))); // т.е берем max значение k и из него вычитаем
    return(result);
  }

  int convert_charge_to_percent(float medium){ // конвертация данных с делителя в проценты
    float percent_1 = (high_border - low_border)/100;
    int percents = (int)((medium - low_border)/percent_1);
    if(percents>100){
      return(100);
    }
    if(percents<0){
      return(0);
    }
    return(percents);
  }
};


class Display{
  private:
  bool first_print = true; // первая печать на очищенном экране или просто первая печать в программе
  bool first_com0_print = true; // первая печать на экран  коианды 0, так как дальше обновляется только количество секунд
  bool first_com2_print = true; // первая печать на экран  коианды 2, так как дальше обновляется только количество секунд
  int counter = 0; // счетчик итераций (надо для отображения заряда)
  int sum_charge_12 = 0; // сумма показаний с делителя напряжения на 12V
  int max_iter = 100; //200; // количество измерений для усреднения
  int percent = 100; // заряд в процентах
  Charge charge;

  LCD_1602_RUS lcd;//(0x27, 20, 4); // Адрес I2C 0x27, 20x4
  byte customBat[8] = { // символ заряда батареи
    0b11111,  
    0b11111,  
    0b11111,  
    0b11111, 
    0b11111, 
    0b11111,
    0b11111,
    0b00000
  };


  void print_charge(int charge){ // часть метода print_battery, выводит только заряд в процентах
    bool print_space = false;
    if(charge<100){
      print_space = true; // печать пробела при заряде меньше 100% 
    }
    lcd.setCursor(0, 0);
    lcd.print("    "); // очистка строки
    lcd.setCursor(0, 0);
    lcd.print(charge, DEC);
    lcd.print("%");
    if(print_space){
      lcd.print(" ");
    }
  }

  void print_battery(int charge){ // печатает заряд в процентах (метод print_charge) и шкалу заряда в скобках
    lcd.setCursor(0, 0);
    bool change_bat = false; // надо ли менять индикацию батареи или нет
    if((charge<100)&&(charge%20 == 0)){ // в данном случае надо менять индикацию (это условие может быть другим)
      change_bat = true;
    }
    int num_indicator = (int)((charge-1)/20) + 1; // количество делений батареи
    if(charge<=2){
      num_indicator=0; 
    }
    print_charge(charge); // печать заряда в процентах
    if((first_print)||(change_bat)){ // либо если первая печать, либо если надо поменять индикацию
      lcd.print(" [");
      for (int i=0; i<num_indicator; i++){
        lcd.write(byte(7)); // print("|"); 
      }
      for (int i=0; i<(5-num_indicator); i++){
        lcd.print(" "); // print("|"); 
      }
      lcd.print("]");
    }
    if(charge==9){
      lcd.setCursor(10, 0); //???
      lcd.print("  ");
    }
    first_print = false;
  }
  public:
  Display() : lcd(0x27, 20, 4){}

  void begin() {
    lcd.init();
    lcd.backlight();
    lcd.createChar(7, customBat);

    delay(500);
    int sum_first = 0;
    int k=75;
    for(int i=0; i<k; i++){
      sum_first+=charge.get_delitel_12();
      delay(10);
    }
    float medium = sum_first/k;
    percent = charge.convert_charge_to_percent(medium); // сделать глобальной
    print_battery(percent);
  }

  void print_message(/*int charge, */int num_message, int arr[]){ // печатает сообщения. На вход номер команды и дополнительная информация (в массиве она лекжит). Некоторые команды (0 и 2) умные и обновляют только секунды, чтобы остальное изображение не мерцало
    if(num_message!=0){
      first_com0_print = true;
    }
    if(num_message!=2){
      first_com2_print = true;
    }
    if((first_com0_print)&&(first_com2_print)){ // если одна из этих 2 команд уже на экране, то не надо очищать экран
      lcd.clear();
    }
    first_print = true;
    delay(10);
    int k;
    print_battery(percent);//charge);
    switch(num_message) {
    case 0: // проверка окончена. прогрев завершится через n сек
      if(first_com0_print){ // т.е все сообщение печатается только 1 раз, дальше обновляются только секунды
        lcd.setCursor(0, 1);
        lcd.print("П"); // rus
        lcd.print("POBEPKA ");
        lcd.print("OKOH");
        lcd.print("Ч"); // rus
        lcd.print("EHA.");
        lcd.setCursor(0, 2);
        lcd.print("П"); // rus
        lcd.print("PO");
        lcd.print("Г"); // rus
        lcd.print("PEB 3ABEP"); // ENG
        lcd.print("ШИ"); // rus
        lcd.print("TC"); // ENG
        lcd.print("Я"); // rus
        lcd.setCursor(0, 3);
        lcd.print("Ч"); // rus
        lcd.print("EPE3 ");
        lcd.print(arr[0], DEC);
        lcd.print(" CEK");
      } else{
        if(arr[0]==9){
          lcd.setCursor(6, 3);
          lcd.print("            ");
          lcd.setCursor(8, 3);
          lcd.print("CEK");
        }
        if(arr[0]==99){
          lcd.setCursor(6, 3);
          lcd.print("            ");
          lcd.setCursor(9, 3);
          lcd.print("CEK");
        }
        if(arr[0]==999){
          lcd.setCursor(6, 3);
          lcd.print("            ");
          lcd.setCursor(10, 3);
          lcd.print("CEK");
        }
        lcd.setCursor(6, 3);
        lcd.print(arr[0], DEC);
      }
      first_com0_print = false;
    break;
    case 1: // проверка устройства
      lcd.setCursor(0, 1);
      lcd.print("П");
      lcd.print("POBEPKA YCTPO");
      lcd.print("Й");
      lcd.print("CTBA");
    break;
    case 2: // измерение: 
      if(first_com2_print){
        lcd.setCursor(0, 1);
        lcd.print("И"); // rus
        lcd.print("3MEPEH");
        lcd.print("И"); // rus
        lcd.print("E: ");
        lcd.print(arr[0], DEC);
        lcd.print(" CEK");
      } else{
        if(arr[0]==10){
          lcd.setCursor(11, 1);
          lcd.print("         ");
          lcd.setCursor(14, 1);
          lcd.print("CEK");
        }
        if(arr[0]==100){
          lcd.setCursor(11, 1);
          lcd.print("         ");
          lcd.setCursor(15, 1);
          lcd.print("CEK");
        }
        if(arr[0]==1000){
          lcd.setCursor(11, 1);
          lcd.print("         ");
          lcd.setCursor(16, 1);
          lcd.print("CEK");
        }
        lcd.setCursor(11, 1);
        lcd.print(arr[0], DEC);
      }
      first_com2_print = false;
    break;
    case 3: // вставьте sd-карту 
      lcd.setCursor(0, 1);
      lcd.print("BCTABbTE SD-KAPTY");
    break;
    case 4: // нет соединения с измерительным модулем
      lcd.setCursor(0, 1);
      lcd.print("HET COE");
      lcd.print("ДИ"); // rus
      lcd.print("HEH");
      lcd.print("ИЯ"); // rus
      lcd.print(" C");
      lcd.setCursor(0, 2);
      lcd.print("И"); // rus
      lcd.print("3MEP");
      lcd.print("И"); // rus
      lcd.print("TE");
      lcd.print("Л"); // rus
      lcd.print("bH");
      lcd.print("Ы");
      lcd.print("M");
      lcd.setCursor(0, 3);
      lcd.print("MO");
      lcd.print("Д"); // rus
      lcd.print("Y");
      lcd.print("Л"); // rus
      lcd.print("EM");
    break;
    case 5: // замените датчик k
      lcd.setCursor(0, 1);
      lcd.print("3AMEH");//COE");
      lcd.print("И"); // rus
      lcd.print("TE ");
      lcd.print("Д"); // rus
      lcd.print("AT");
      lcd.print("ЧИ"); // rus
      lcd.print("K ");//CEHCOP ");
      if(arr[0]>2){ // проверка на корректность данных в массиве
      k=0;
      } else{
        k=arr[0];
      }
      lcd.print(k, DEC);
    break;
    case 6: // нет соединения с датчиком k
      lcd.setCursor(0, 1);
      lcd.print("HET COE");
      lcd.print("ДИ"); // rus
      lcd.print("HEH");
      lcd.print("ИЯ "); // rus
      lcd.print("C");
      lcd.setCursor(0, 2);
      lcd.print("Д"); // rus
      lcd.print("AT");
      lcd.print("ЧИ"); // rus
      lcd.print("KOM ");
      if(arr[0]>2){ // проверка на корректность данных в массиве
      k=0;
      } else{
        k=arr[0];
      }
      lcd.print(k, DEC);
    break;
    case 7: // обнаружена неисправность: нет соединения с датчиками k1, k2
      lcd.setCursor(0, 1);
      lcd.print("O");
      lcd.print("Б"); // rus
      lcd.print("HAPY");
      lcd.print("Ж"); // rus
      lcd.print("EHA HE");
      lcd.print("И"); // rus
      lcd.print("C");
      lcd.print("П"); // rus
      lcd.print("PAB-");
      lcd.setCursor(0, 2);
      lcd.print("HOCTb: HET COE");
      lcd.print("ДИ"); // rus
      lcd.print("HE-");
      lcd.setCursor(0, 3);
      lcd.print("H");
      lcd.print("ИЯ"); // rus
      lcd.print(" C ");
      lcd.print("Д"); // rus
      lcd.print("AT");
      lcd.print("ЧИ"); // rus
      lcd.print("KAM");
      lcd.print("И ");
      if(arr[0]>2){ // проверка на корректность данных в массиве
      k=0;
      } else{
        k=arr[0];
      }
      lcd.print(k, DEC);
      lcd.print(", ");
      if(arr[1]>2){ // проверка на корректность данных в массиве
      k=0;
      } else{
        k=arr[1];
      }
      lcd.print(k, DEC);
    break;
    case 8: // ! проверьте исправность вентилятора
      lcd.setCursor(0, 1);
      lcd.print("! ПPOBEPbTE ИCПPAB-");
      lcd.setCursor(0, 2);
      lcd.print("HOCTb BEHTИЛЯTOPA");
    break;
    case 9: // ! замените главный светодиод
      lcd.setCursor(0, 1);
      lcd.print("! 3AMEHИTE ГЛABHЫЙ");
      lcd.setCursor(0, 2);
      lcd.print("CBETOДИOД");
    break;
    }  
  }

  void update_charge(){ // этот метод должен вызываться в loop (или любом другом цикле), он считывает данные с делителя 12В и делает усреднение по заранее заданному числу измерений. Если процент меньше текущего, то он автоматически обновляется на экране
    sum_charge_12+=charge.get_delitel_12(); // здесь get_delitel
    counter++;
    if(counter==max_iter){
      float medium = sum_charge_12/max_iter;
      int percent_tmp = charge.convert_charge_to_percent(medium);
      if(percent_tmp < percent){
        percent = percent_tmp;
        print_battery(percent);
      }
      counter=0;
      sum_charge_12=0;
    }    
  }
};


class Kuler{ // класс для работы с вентилятором охлаждения
  private: 
  int termistor_pin = 34;
  int value_cold = 2175; // 2160
  int value_35 = 2160; // на 220-й секунде
  int value_40 = 2260; // значение термистора при 40 градусах (на 350-й секунде)
  int value_50 = -1;
  const int n = 30; // количество измерений для усреднения
  int sum = 0;
  int iter_counter = 0; // счетчик итераций основного цикла
  public:
  void begin(){
    pinMode(termistor_pin, INPUT);
  }

  int get_temperature(){ 
    return(analogRead(termistor_pin));
  }

  bool check_hot(){
    bool res = false;
    sum+=get_temperature();
    iter_counter++;
    if(iter_counter == n){
      int temp = (int)(sum/n);
      Serial.print("Temp = ");
      Serial.println(temp);
      if(temp>=value_40){
        res=true;        
      }
      sum=0;
      iter_counter=0;
    }
    return(res);
  }

  bool check_cold(){ // на случай, если вентилятор уже достаточно остудил радиатор
    bool res = false;
    if(iter_counter == n){
      int temp = (int)(sum/n);
      Serial.print("Temp = ");
      Serial.println(temp);
      if(temp <= value_cold){
        res=true;        
      }
    }
    return(res);
  }
};


// sd надо бы тоже в класс
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

void write_data_to_sd(float ppm_1, float ppm_2, /*float ppm_3,*/ float medium_ppm,
                      float temp_1, float temp_2, /*float temp_3,*/ float medium_temp,
                      int accuracy_1, int accuracy_2, /*int accuracy_3,*/
                      int min_co2_1, int min_co2_2, /*int min_co2_3,*/ float time_1) {
  static char data_array[150];
  static char array_ppm1[9], array_ppm2[9], /*array_ppm3[9],*/ array_medium_ppm[9];
  static char array_temp1[9], array_temp2[9], /*array_temp3[9],*/ array_medium_temp[9];
  static char array_accuracy1[9], array_accuracy2[9]/*, array_accuracy3[9]*/;
  static char array_min_co2_1[9], array_min_co2_2[9]/*, array_min_co2_3[9]*/;
  static char array_time[9];

  // Преобразуем числа в строки
  dtostrf(ppm_1, 4, 3, array_ppm1);
  dtostrf(ppm_2, 4, 3, array_ppm2);
  /*dtostrf(ppm_3, 4, 3, array_ppm3);*/
  dtostrf(medium_ppm, 4, 3, array_medium_ppm);
  dtostrf(temp_1, 4, 3, array_temp1);
  dtostrf(temp_2, 4, 3, array_temp2);
  /*dtostrf(temp_3, 4, 3, array_temp3);*/
  dtostrf(medium_temp, 4, 3, array_medium_temp);
  dtostrf(accuracy_1, 4, 3, array_accuracy1);
  dtostrf(accuracy_2, 4, 3, array_accuracy2);
  /*dtostrf(accuracy_3, 4, 3, array_accuracy3);*/
  dtostrf(min_co2_1, 4, 3, array_min_co2_1);
  dtostrf(min_co2_2, 4, 3, array_min_co2_2);
  /*dtostrf(min_co2_3, 4, 3, array_min_co2_3);*/
  dtostrf(time_1, 4, 3, array_time);

  // Формируем строку для записи
  snprintf(data_array, sizeof(data_array),
           "%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;"/*%s;%s;%s;%s"*/,
           array_ppm1, array_ppm2, /*array_ppm3,*/ array_medium_ppm,
           array_temp1, array_temp2, /*array_temp3,*/ array_medium_temp,
           array_accuracy1, array_accuracy2, /*array_accuracy3,*/
           array_min_co2_1, array_min_co2_2, /*array_min_co2_3,*/ array_time);

  // Записываем данные в файл
  appendFile(SD, path, data_array, true);

  // Очищаем массив
  for(int i = 0; i < 150; i++) {
    data_array[i] = ' ';
  }
}

char* converter_to_array(char* result, char command, int warming_time_s, char sensor_rele, char led_rele, char kuler_rele) {
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
  result[5] = sensor_rele;
  result[6] = led_rele;
  result[7] = kuler_rele;
  return(result);
}




Display display; // конструктор не принимает параметров, значит скобки не нужны
Kuler kuler;
void setup() {
  display.begin(); 
  kuler.begin();
 /// pinMode(rele1_pin, OUTPUT); // первый
  //pinMode(rele2_pin, OUTPUT); // второй
  //pinMode(rele3_pin, OUTPUT); // третий
  //digitalWrite(rele1_pin, HIGH);
  //digitalWrite(rele2_pin, HIGH); // поменять на HIGH с нормальным реле!
  //digitalWrite(rele3_pin, HIGH);
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

  appendFile(SD, path, "CO2_1;CO2_2;CO2_medium;Temp_1;Temp_2;Temp_medium;Accuracy_1;Accuracy_2;Min_CO2_1;Min_CO2_2; time"/*"CO2_1;CO2_2;CO2_3;CO2_medium;Temp_1;Temp_2;Temp_3;Temp_medium;Accuracy_1;Accuracy_2;Accuracy_3;Min_CO2_1;Min_CO2_2;Min_CO2_3; time"*/, true);
  if(led_between_warm){
    led_time_counter = warming_time;
  }
}

int data_1[5];
int data_2[5];
//int data_3[5];

int n=0;
int myArray[] = {3, 3};
bool reset = false;


void loop() { // данные не пишутся на флешку перед прогревом. попробовать писать данные на флешку
  static int izmer_counter = 0; // счетчик количества измерений (для вывода на экран)
  bool first_reset = false;
  bool second_reset = false;
  bool no_print_display=false; // флаг, чтобы сообщение "проверка устройства" не переизображалось несколько раз

  bool hot = false;
  if(led_rele == 2){
    if(kuler_rele == 2){
      if(kuler.check_cold()){
        kuler_rele = 1;
      }
    }
    if(kuler.check_hot()){ // радиатор достаточно прогрелся?
      kuler_rele = 2;
    }
  }


  if((reset == true)&&(loop_counter==2)){ // т.е. ошибка 15 отработана реле
    no_print_display = true;
    reset=false;
    loop_counter=0;
  }
  if(led_time_counter == led_time){ // нужное время прошло и реле надо отключить
    led_rele = 1; // 1 - выключено
    kuler_rele = 1;
  }
  if((loop_counter==(read_between_warm + 3))&&(led_between_warm==false)){ // включение реле после прогрева. если read_between_warm + 2, то включается за 3 секунды до конца прогрева
    led_rele = 2;
  }
  //bool third_reset = false;
  //bool reset = false;
  if(loop_counter == 0){// этот блок вызывается только на первой итерации цикла loop
    if(no_print_display==false){
      display.print_message(1, myArray);
    }
    display.update_charge();
    delay(first_warming_time*1000);
  }
  if(loop_counter == read_between_warm){
    /*bool first_reset = false;
    bool second_reset = false;
    bool third_reset = false;
    bool reset = false;*/
    if(data_1[4]==15){
      first_reset = true;
      reset = true;
    }
    if(data_2[4]==15){
      second_reset = true;
      reset = true;
    }
    /*if(data_3[4]==15){
      third_reset = true;
      reset = true;
    }*/
    if(reset == true){ // здесь добавить блок отправки команды на ардуино, команда 11 
      loop_counter = 0; 
      if(first_reset==true){
        //digitalWrite(rele1_pin, LOW);
        sensor_rele = 20;
      } else{
        sensor_rele = 10;
      }
      if(second_reset==true){
        //digitalWrite(rele2_pin, LOW);
        sensor_rele = sensor_rele + 2;
      } else{
        sensor_rele = sensor_rele + 1;
      }
      /*if(third_reset==true){
        digitalWrite(rele3_pin, LOW);
      } */
      /*delay(10*1000);
      digitalWrite(rele1_pin, HIGH);
      digitalWrite(rele2_pin, HIGH);
      digitalWrite(rele3_pin, HIGH);
      display.update_charge();
      delay(first_warming_time*1000);*/
    } else{
      write_sd_flag = true;
    }
  }






  switch(loop_counter) { // если loop_counter<9, читаем данные. если loop_counter = 10, то command = 5
  // if(loop_counter==11) command = 4 (здесь более развенуто); if(loop_counter==12) command = 2(более развернуто); 
    case read_between_warm: // 0
      read_co2 = false;
      command = 5; // прогрев
      if(led_between_warm){ // включение реле перед прогревом, если надо включить перед прогревом 
        led_rele = 2; // 2 - включено, 1 - выключено
      }
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
      if(loop_counter > (read_between_warm + 2)){
        if((izmer_counter%5)==0){
          display.update_charge();
          myArray[0] = (int)(izmer_counter/5);
          display.print_message(2, myArray);
          led_time_counter++; 
        }
        izmer_counter++;
        //led_time_counter++; 
      }
  }
  if((reset == true)&&(loop_counter==1)){ // чтобы команда на отключение реле отправилась, а чтение данных с датчика не производилось
    command = 0;
    sensor_rele=11;
  }


  // Создание запроса на ардуины
  static const int size_send_message = 8; // 5
  char* message = new char[size_send_message];
  converter_to_array(message, command, warming_time, sensor_rele, led_rele, kuler_rele); // элемент 1 - command. Результат записывается в message
  //Serial.print((int)sensor_rele);
  //Serial.println(" rele");

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

  /*// Отправка запроса на Ардуино 3
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
  }*/

  // Усреднение данных
  if((counter_for_medium_1 >= 5) && (counter_for_medium_2 >= 5) /*&& (counter_for_medium_3 >= 5)*/) {
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

    /*ppm_3_medium = ppm_3_sum / counter_for_medium_3;
    temp_3_medium = temp_3_sum / counter_for_medium_3;
    ppm_3_sum = 0;
    temp_3_sum = 0;
    counter_for_medium_3 = 0;*/

    ppm_common = (ppm_1_medium + ppm_2_medium/* + ppm_3_medium*/) / 2;//3;
    temp_common = (temp_1_medium + temp_2_medium/* + temp_3_medium*/) / 2;//3;
    
    if(loop_counter < read_between_warm){
      data_1[(int)(loop_counter/5)] = ppm_1_medium;
      data_2[(int)(loop_counter/5)] = ppm_2_medium;
      //data_3[(int)(loop_counter/5)] = ppm_3_medium;
    }
    if(write_sd_flag){
      write_data_to_sd(ppm_1_medium, ppm_2_medium, /*ppm_3_medium, */ppm_common,
                     temp_1_medium, temp_2_medium, /*temp_3_medium, */temp_common,
                     accuracy_1, accuracy_2, /*accuracy_3,*/
                     min_co2_1, min_co2_2, /*min_co2_3,*/ time_s);
      time_s = time_s + time_step;
    }
  } 

  Serial.println();
  if(loop_counter == read_between_warm){
    for(int i=warming_time; i>=7; i--){
        // здесь же вызов check_pribor
        myArray[0]=i;
        display.print_message(0, myArray);
        display.update_charge();
        delay(delay_in_command0); // это сделать константой, это 1 секунда при прогреве!!
      }
    //delay((warming_time + 1) * 1000);    
  } else if(read_co2 == true){
    delay(delay_between_readings);
  } else{
    // здесь же вызов check_pribor
    if(loop_counter == (read_between_warm + 1)){ // эта задержка в 3 секунды связана с тем, что отправляется команда set_zero_point
      for(int i=6; i>=3; i--){
        myArray[0]=i;
        display.print_message(0, myArray);
        display.update_charge();
        delay(delay_in_command0); // это сделать константой, это 1 секунда при прогреве!!
      }
    }else if(loop_counter == (read_between_warm + 2)){ // эта задержка в 3 секунды связана с тем, что отправляется команда на автокалибровку
    // тут есть затык примерно в 1 секунду, пока предлагаю так оставить (связано, скорее всего, с отправкой сообщения)
      for(int i=3; i>=0; i--){
        myArray[0]=i;
        display.print_message(0, myArray);
        display.update_charge();
        delay(delay_in_command0); // это сделать константой, это 1 секунда при прогреве!!
      }
    } else{
      delay(3000);
    }
  }
  if((reset == true)&&(loop_counter==0)){ // задержка, равная времени, когнда реле разомкнуто
    //data_1[4]=1; // попробовать закомментировать
    //data_2[4]=1;
    delay(rele_open_time); //время, когда реле разомкнуто
  }
  if((reset == true)&&(loop_counter==1)){ // для того, чтобы реле успевало разомкнуться
    delay(1000);
  }
  loop_counter++;
}