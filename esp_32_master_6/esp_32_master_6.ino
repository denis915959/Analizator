#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "Arduino.h"
#include "Wire.h"
#include <FS.h>
#include <SD.h>
#include <sd_defines.h>
#include <sd_diskio.h>
#include <SPI.h>
#include <ESP8266_LCD_1602_RUS.h>
#include <font_LCD_1602_RUS.h>
#include <Preferences.h>
#include <vector>

using std::vector;

#define I2C_DEV_ADDR_1 0x09
#define I2C_DEV_ADDR_2 0x55
//#define I2C_DEV_ADDR_3 0x11

char* path = "/data.txt";
char command = -1;
const int warming_time = 30; //300; // время прогрева(в секундах)
const bool led_between_warm = false;; // true - светодиод включается после проверки устройства на весь период прогрева. false - включается сразу после прогрева
uint32_t led_time = 1800;// 450 // время работы светодиода в секундах
uint32_t measure_time = 780; // время измерения (без работы светодиода) в секундах. Возможно, потом суммировать с временем работы светодиода?

const int rele_open_time = 10;//10000  // это время размыкания реле при ошибке 15
const int first_warming_time = 10; //10 - все работает; время прогрева перед тестом на ошибку 15
int loop_counter = 0;
bool set_zero_flag = true; // если true, то будет использоваться СетЗеро, если false, то не будет 
bool use_autocalibration = false; // если true, то будет использоваться Автокалибровка, если false, то не будет 
bool read_co2 = false; // флаг для работы программы
const int delay_between_readings = 272; //161; //130; //150(добавился экран и это замедляет код); // при задержке 100 мс частота измерений 1.37 Гц.
const int measure_count = 3;//3; // количество измерений для усреднения
const int read_between_warm = measure_count*5; //25; // количество считываний перед проверкой на ошибку 15
const int delay_in_command0 = 960; // 1 секунда в цикле прогрева
bool do_work = true; // true - цикл while запускается, false - цикл останавливается
bool do_measure = false; // true - поступила команда начать измерение(именно измерение, не прогрев и не ошибка 15), false - измерение прерывается и не запускается, пока переменная не станет true
int max_loop_iter = measure_time*measure_count + measure_count*led_time; // число итераций цикла loop с временем свечения светодиода
bool send_last_message = false; // становится true при прерывании измерения, нужен для отправки сообщения об выключении светодиода и вентилятора на ардуино
const int delay_between_loop_iter = 10; // задержка между итерациями цикла loop
const int delay_after_on_off_click = 10; // время задержки после нажатия кнопки старт стоп
const int iter_to_one_second_in_warm = 93;

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


#include <iostream> //библиотеки
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <sstream>
#include <algorithm>

using namespace std;

int number_cycles = 0;
int b;
int b1;
int coord1;

double a_min0;

double a_max0;

double znachen;
double znachen1;
double znachen2;
double plosh;
double volumec = 0.000008305;
double V_S;

double assimil1;
double assimil2;
double assimil3;

double razn01;
double razn02;
double razn03;
std::vector<double> razn1;
std::vector<double> razn2;
std::vector<double> razn3;
std::vector<double> razn0;
std::vector<double> listCO21;
std::vector<double> listCO22;
std::vector<double> listCO23;
//std::vector<double> temp1;
//std::vector<double> temp2;
//std::vector<double> temp3;
//std::vector<double> time1;
std::vector<double> b_begin;
std::vector<double> b_end;
std::vector<std::string> segments;

void assim() {
    razn1.clear();
    razn2.clear();
    razn3.clear();
    razn01 = 0;
    razn02 = 0;
    razn03 = 0;
    int cyclass = listCO21.size();
    for (int i4 = 3; i4 < cyclass; i4++) {
        if ((i4 + 30) < cyclass) {
            razn01 = listCO21[i4 + 30] - listCO21[i4];
            if (abs(razn01) <= 40) {
                razn1.push_back(razn01);
            }
            razn02 = listCO22[i4 + 30] - listCO22[i4];
            if (abs(razn02) <= 40) {
                razn2.push_back(razn02);
            }
            razn03 = listCO23[i4 + 30] - listCO23[i4];
            if (abs(razn03) <= 40) {
                razn3.push_back(razn03);
            }
        }
    }
}

void min_max_val() {
    b1 = razn0.size();
    for (int i7 = 0; i7 < b1; i7++) {
        if (i7 == 0) {
            a_min0 = razn0[i7];
        }
        else {
            if (a_min0 > razn0[i7]) {
                a_min0 = razn0[i7];
            }
        }
    }

    for (int i8 = 0; i8 < b1; i8++) {
        if (i8 == 0) {
            a_max0 = razn0[i8];
        }
        else {
            if (a_max0 < razn0[i8]) {
                a_max0 = razn0[i8];
            }
        }
    }

}

void val() {
    for (int i5 = 0; i5 < 3; i5++) {
        int rvm;
        razn0.clear();
        rvm=razn1.size();
        if (rvm != 0) {
            if (i5 == 0) {
                razn0.assign(razn1.begin(), razn1.end());
                min_max_val();
                assimil1 = V_S * 1000 / 22.4 * (a_max0 - a_min0) / 30;
            }
            if (i5 == 1) {
                razn0.assign(razn2.begin(), razn2.end());
                min_max_val();
                assimil2 = V_S * 1000 / 22.4 * (a_max0 - a_min0) / 30;
            }
            if (i5 == 2) {
                razn0.assign(razn3.begin(), razn3.end());
                min_max_val();
                assimil3 = V_S * 1000 / 22.4 * (a_max0 - a_min0) / 30;
            }
        }
    }
}


struct LedCorrection { // структура данных для хранения аппроксимации падения напряжения от светодода
  float led;
  float k; // коэффициент коррекции
};

struct Parametr{
  int low_border = 0;
  int high_border = 0;
  uint32_t number = 0;
  int k = 0;
};

// этот класс нужен для вынесения логики, связанной с с расчетом процентов и постобработкой данных с делителя 12В, за пределы класса Display
class Charge{ // класс для считывания данных с делителя (с постобработкой данных с делителя) и конвертации усредненного значения в проценты.
  private:
    const int charge_pin_12 = 35; //33; // пин делителя 12В 
    const int charge_pin_led = 33; // пин делителя светодиода 
    int low_border = 1557; //2067; //нижняя граница (6.4 В) /*(8.5 В)*/
    int high_border = 2050; //3075;  //верхняя граница (ее надо увеличить после подключения светодиода)
    const int num_iter_led = 10; // число итераций для усреднения при чтении с делителя светодиода
    bool print_percents = false;
    // ниже 30% падение замедляется, я бы напряжение, соответствующее 30%, приравнял к 40%. то есть не линейная зависимость, а с изломом
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
    //static const int pointsCount = 4; // кол-во точек в led_correction
    //const LedCorrection approximation[pointsCount] = {
   //   {0.0, 0.0},
  //      {1500.0, 0.0038},
    //    {1670.0, 0.009/*0.0115*/},
//     {2410.0, 0.018/*0.0228*/}
  //  };

  public:
  Charge(){
    pinMode(charge_pin_12, INPUT);
    pinMode(charge_pin_led, INPUT);
  }

  int get_delitel_12(){ // вывод данных с делителя 12В, сюда добавить пересчет данных с делителя 12В в зависимости от данных с делителя светодиода
    int charge = analogRead(charge_pin_12); 
    int result = charge;
    return(result);
  }

  int get_delitel_led(){ // вывод данных с делителя 12В, сюда добавить пересчет данных с делителя 12В в зависимости от данных с делителя светодиода
    int charge_sum = 0;
    for(int i=0; i<num_iter_led; i++){
      charge_sum+=analogRead(charge_pin_led); 
    }
    int result = (int)(charge_sum/num_iter_led);
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

  int convert_led_to_result(float medium){ // добавить сюда нормальную конвертацию!!! сначала просто в % по заряду, потом добавить поправку по яркости
    medium = medium-2000;
    const float x[] = {350, 361, 369, 375, 379.5, 388, 395, 402, 410, 416, 424, 430, 433, 443, 451, 
                      459, 464, 470, 480, 485, 490, 495, 507, 513, 521, 530, 535, 539, 
                      545, 550, 556, 562, 569, 577, 581, 594, 600, 608, 615, 622, 630, 
                      637, 645, 650, 655, 662, 666, 672, 674, 680};
    
    const float y[] = {33.8688, 50.803, 59.27, 65.317, 70.156, 84.671, 94.952, 105.839, 115.515, 127.007, 140.312, 148.779, 156.037, 166.923, 187.486, 206.839, 211.678, 223.773, 242.522, 256.432, 264.294, 272.762, 301.187, 313.283, 328.403, 345.942, 353.199, 362.271, 377.996, 388.882, 404.002, 434.846, 437.871, 448.757, 451.78, 467.505, 472.949, 497.745, 514.075, 529.800, 546.129, 562.458, 581.207, 589.673, 601.769, 615.680, 621.728, 635.638, 638.057, 650.0};
    
    const int n = sizeof(x) / sizeof(x[0]);
    bool is_res = false;
    int res;
    if (medium <= x[0]) { // Проверка граничных значений
      res = y[0]; // Нижняя граница
      is_res = true;
    }
    if (medium >= x[n-1]) {
      res = (int)y[n-1];  //верхняя граница, если (int)y[n-1] - то не работает
      is_res = true;
    }
    if(!is_res){ // Поиск интервала, в котором находится medium
      is_res = true;
      for (int i = 0; i < n - 1; i++) {
        if (medium >= x[i] && medium <= x[i+1]) {
          float y_output = y[i] + (medium - x[i]) * (y[i+1] - y[i]) / (x[i+1] - x[i]); // Линейная интерполяция
          res = y_output;  // Преобразование в int
          break;
        }
      }
    }
    if(!is_res){ // Если не нашли интервал (не должно случиться)
      res = y[0];
    }
    if(print_percents){
      res = res/(y[n-1]/100);
    }
    return((int)res);
  }
};


class Display{
  private:
  bool first_print = true; // первая печать на очищенном экране или просто первая печать в программе
  bool first_com0_print = true; // первая печать на экран  коианды 0, так как дальше обновляется только количество секунд
  bool first_com2_print = true; // первая печать на экран  коианды 2, так как дальше обновляется только количество секунд
  bool first_com11_print = true; // первая печать на экран  коианды 11, так как дальше обновляется только количество секунд
  bool first_com12_print = true; // первая печать на экран  коианды 12, так как дальше обновляется только количество секунд
  bool command_10_flag = false; // для перезагрузки экрана в режиме ожидания
  int counter_12 = 0; // счетчик итераций (надо для отображения заряда)
  int counter_led = 0;
  int sum_charge_12 = 0; // сумма показаний с делителя напряжения на 12V в цикле loop
  int sum_led = 0; // сумма показаний с делителя напряжения от светодиода в цикле loop
  int max_iter_12 = 100; //200; // количество измерений для усреднения измерения заряда
  int max_iter_led = 25;
  int percent_12 = 100; // заряд в процентах
  int percent_led = -1; // заряд в процентах  
  Charge charge;

  LCD_1602_RUS lcd;//(0x27, 20, 4); // Адрес I2C 0x27, 20x4
  //::byte customBat[8]
  ::byte customBat[8] = { // символ заряда батареи
    0b11111,  
    0b11111,  
    0b11111,  
    0b11111, 
    0b11111, 
    0b11111,
    0b11111,
    0b00000
  };

  ::byte customLed[8] = { // символ солнышко для яркости светодиода
    0b00000,  
    0b10101,  
    0b01110,  
    0b11011, 
    0b01110, 
    0b10101,
    0b00000,
    0b00000
  };


  void print_charge(int charge){ // часть метода print_battery, выводит только заряд в процентах
    lcd.setCursor(0, 0);
    lcd.print("    "); // очистка строки
    lcd.setCursor(0, 0);
    lcd.print(charge, DEC);
    lcd.print("%");
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
      lcd.setCursor(4, 0);
      lcd.print("[");
      for (int i=0; i<num_indicator; i++){
        lcd.write(::byte(7)); // print("|"); // lcd.write(byte(7));
      }
      for (int i=0; i<(5-num_indicator); i++){
        lcd.print(" "); // print("|"); 
      }
      lcd.print("]");
      if(first_print){ // вывод символа солнышко
        lcd.setCursor(14, 0);
        lcd.write(::byte(6));
      }
    }
    first_print = false;
  }

 void print_led(int led){
    lcd.setCursor(16, 0);
    lcd.print("    ");
    lcd.setCursor(16, 0);
    lcd.print(led, DEC);
    lcd.print("%");
  }

  public:
  Display() : lcd(0x27, 20, 4){}

  void begin() {
    lcd.init();
    lcd.backlight();
    lcd.createChar(7, customBat);
    lcd.createChar(6, customLed);

    delay(500);
    int sum_first_12 = 0;
    int sum_first_led = 0;
    int k=75;
    for(int i=0; i<k; i++){
      sum_first_12+=charge.get_delitel_12();
      sum_first_led+=charge.get_delitel_led();
      delay(10);
    }
    float medium_12 = sum_first_12/k;
    float medium_led = sum_first_led/k;    
    percent_12 = charge.convert_charge_to_percent(medium_12); // сделать глобальной
    percent_led = charge.convert_led_to_result(medium_led);
    print_battery(percent_12);
    print_led(percent_led);
  }

  bool command_10_printed(){
    return(command_10_flag);
  }

  void print_message(/*int charge, */int num_message, int arr[]){ // печатает сообщения. На вход номер команды и дополнительная информация (в массиве она лекжит). Некоторые команды (0 и 2) умные и обновляют только секунды, чтобы остальное изображение не мерцало
    if(num_message!=0){
      first_com0_print = true;
    }
    if(num_message!=2){
      first_com2_print = true;
    }
    if(num_message!=11){
      first_com11_print = true;
    }
    if(num_message!=12){
      first_com12_print = true;
    }
    if((first_com0_print)&&(first_com2_print)&&(first_com11_print)&&(first_com12_print)){ // если одна из этих 2 команд уже на экране, то не надо очищать экран
      lcd.clear();
    }
    first_print = true;
    //delay(10);
    int k;
    print_battery(percent_12);
    print_led(percent_led);
    if(num_message==10){
      command_10_flag = true;
    } else{
      command_10_flag = false;
    }
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
      lcd.setCursor(0, 2);
      lcd.print("ИЛИ HAЖMИTE KHOПKY");
      lcd.setCursor(0, 3);
      lcd.print("CTOП");
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
    case 10: // ДЛЯ НАЧАЛА ИЗМЕРЕНИЯ НАЖМИТЕ СТАРТ 
      lcd.setCursor(0, 1);
      lcd.print("ДЛЯ");
      lcd.print(" CTAPTA ");
      lcd.print("И");
      lcd.print("3MEPEH");
      lcd.print("ИЯ");
      lcd.setCursor(0, 2);
      lcd.print("HAЖMИTE KHOПKY CTAPT");
    break;
    case 11: // время свечения 
      if(first_com11_print){ // (arr[0]==-1) добавлено, так как при выходе из настроек надо удалять 
        lcd.setCursor(0, 2);
        lcd.print("   BPEMЯ CBEЧEHИЯ");
      }
      if(arr[0]>-1){
        lcd.setCursor(0, 3);
        lcd.print("   ");
        lcd.print(arr[0], DEC);
        lcd.print(" CEK    ");
      } else{
        lcd.setCursor(0, 3);
        lcd.print("                  ");
      }
      first_com11_print = false;
    break;
    case 12: // ВРЕМЯ ИЗМЕРЕНИЯ
      if(first_com12_print){
        lcd.setCursor(0, 2);
        lcd.print("   BPEMЯ И3MEPEHИЯ");
      }
      if(arr[0]>-1){
        lcd.setCursor(0, 3);
        lcd.print("   ");
        lcd.print(arr[0], DEC);
        lcd.print(" CEK   ");
      } else{
        lcd.setCursor(0, 3);
        lcd.print("                  ");
      }
      first_com12_print = false;
    break;
    case 13: // ВЫХОД
      lcd.setCursor(0, 2);
      lcd.print("       BЫXOД");
    break;
    case 14: // низкий заряд аккумулятора, нельзя начать новое измерение
      lcd.setCursor(0, 1);
      lcd.print("HEЛb3Я HAЧATb И3ME-");
      lcd.setCursor(0, 2);
      lcd.print("PEHИE, AKKYMYЛЯTOP");
      lcd.setCursor(0, 3);
      lcd.print("PA3PЯЖEH");
    break;
    /*case 11: // ДЛЯ НАЧАЛА ИЗМЕРЕНИЯ НАЖМИТЕ СТАРТ 
      lcd.setCursor(0, 1);
      lcd.print("ДЛЯ BXOДA B");
      lcd.setCursor(0, 2);
      lcd.print("HACTPOЙKИ HAЖMИTE OK");
    break;*/
    }  
  }

  void update_charge(){ // этот метод должен вызываться в loop (или любом другом цикле), он считывает данные с делителя 12В и делает усреднение по заранее заданному числу измерений. Если процент меньше текущего, то он автоматически обновляется на экране
    sum_charge_12+=charge.get_delitel_12(); // здесь get_delitel
    sum_led+=charge.get_delitel_led();
    counter_12++;
    counter_led++;
    if(counter_12==max_iter_12){
      float medium_12 = sum_charge_12/max_iter_12;
      int percent_tmp_12 = charge.convert_charge_to_percent(medium_12);
      if(percent_tmp_12 < percent_12){
        percent_12 = percent_tmp_12;
        print_battery(percent_12);
      }
      counter_12=0;
      sum_charge_12=0;
    } 
    if(counter_led==max_iter_led){
      float medium_led = sum_led/max_iter_led;
      int percent_tmp_led = charge.convert_led_to_result(medium_led);      
      if(percent_tmp_led != percent_led){
        percent_led = percent_tmp_led;
        print_led(percent_led);
      }
      counter_led=0;
      sum_led=0;      
    }       
  }

  int get_percents(){ // выводит проценты зарядки
    int result = charge.get_delitel_12();
    result = charge.convert_charge_to_percent(result);
    return(result);
  }
};


class Kuler{ // класс для работы с вентилятором охлаждения
  private: 
  int termistor_pin = 34;
  int value_cold = 2175; // 2160
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

  int check_hot(){ // 0 - ничего; 1 - холодно, надо отключить вентилятор; 2 - жарко, включить вентилятор
    int res = 0;
    sum+=get_temperature();
    iter_counter++;
    if(iter_counter == n){
      int temp = (int)(sum/n);
      Serial.print("Temp = ");
      Serial.println(temp);
      if(temp>=value_40){
        res=2;        
      }
      if(temp <= value_cold){
        res=1;        
      }
      sum=0;
      iter_counter=0;
    }
    return(res);
  }
};


class Buttons{ // класс для работы с кнопками
  private:
  QueueHandle_t q;
  QueueHandle_t longPressQueue;
  bool reading_queue = true; // чтобы очередь опрашивалась один раз при нажатии кнопки, а не по числу вызовов методов click
  uint8_t data = -1;
  const int on_off_pin = 25;
  const int ok_pin = 27;
  const int left_pin = 12; //13;
  const int right_pin = 32;
  const int up_pin = 26; // кнопка вверх
  const int down_pin = 14; // кнопка вниз
  const int queue_delay = 50; // в мс
  const unsigned long LONG_PRESS_TIME = 1500;  // 1.5 секунды
  const unsigned long LONG_UP_PRESS_TIME = 270; //возможно, сделать 250

  static void buttonReaderStatic(void* pv) {
    Buttons* instance = static_cast<Buttons*>(pv);
    instance->buttonReader();
  }

  void buttonReader() { // поток работает до конца работы программы
    int b1_prev = 0;
    int b2_prev = 0;
    int b3_prev = 0;
    int b4_prev = 0;
    int b5_prev = 0;
    int b6_prev = 0;
    unsigned long int press_start_time = 0; // для проверки длинного нажатия
    unsigned long int press_up_start_time = 0;
    unsigned long int press_down_start_time = 0;
    bool ok_pressed = false;
    bool up_pressed = false;
    bool down_pressed = false;
    unsigned long current_time; // текущее время
    
    while(1) { // этот цикл будет длиться до конца выполнения программы
        current_time = millis(); // текущее время
        // Кнопка 1
        int b1 = digitalRead(on_off_pin);
        if((b1 == 1)&&(b1_prev == 0)) { // если кнопка нажата
          vTaskDelay(queue_delay / portTICK_PERIOD_MS);
          b1 = digitalRead(on_off_pin);
          if((b1 == 1)&&(b1_prev == 0)){
            uint8_t data = 1; // 1, если кнопка 1 нажата
            xQueueSend(q, &data, 0);
          }
        }
        b1_prev = b1; 
        // Кнопка 2
        int b2 = digitalRead(ok_pin);
        if((b2 == 1)&&(b2_prev == 1)) { // проверка длинного нажатия
          if(ok_pressed && press_start_time > 0) {
            if((current_time - press_start_time) >= LONG_PRESS_TIME) {
              uint8_t data = 7; // 7 - если кнопка ok была долго нажата
              xQueueSend(longPressQueue, &data, 0);
              press_start_time = 0; // Сбрасываем таймер, чтобы не отправлять повторно
              ok_pressed = false;
            }
          }
        }
        if((b2 == 1)&&(b2_prev == 0)) { // если кнопка нажата
          vTaskDelay(queue_delay / portTICK_PERIOD_MS);
          b2 = digitalRead(ok_pin);
          if((b2 == 1)&&(b2_prev == 0)){
            uint8_t data = 2; // 2, если кнопка 2 нажата
            xQueueSend(q, &data, 0);
            press_start_time = current_time; // Начало отсчета длинного нажатия
            ok_pressed = true;
          }
          if(b2 == 0){  // Если кнопка отпущена
            if(ok_pressed) {
              ok_pressed = false;
              press_start_time = 0;
            }
          }
        }
        b2_prev = b2;
        // Кнопка 3
        int b3 = digitalRead(left_pin);
        if((b3 == 1)&&(b3_prev == 0)) { // если кнопка нажата
          vTaskDelay(queue_delay / portTICK_PERIOD_MS);
          b3 = digitalRead(left_pin);
          if((b3 == 1)&&(b3_prev == 0)){
            uint8_t data = 3; // 3, если кнопка 3 нажата
            xQueueSend(q, &data, 0);
          }
        }
        b3_prev = b3; 
        // Кнопка 4
        int b4 = digitalRead(right_pin);
        if((b4 == 1)&&(b4_prev == 0)) { // если кнопка нажата
          vTaskDelay(queue_delay / portTICK_PERIOD_MS);
          b4 = digitalRead(right_pin);
          if((b4 == 1)&&(b4_prev == 0)){
            uint8_t data = 4; // 4, если кнопка 4 нажата
            xQueueSend(q, &data, 0);
          }
        }
        b4_prev = b4; 
        // Кнопка 5
        int b5 = digitalRead(up_pin);
        if((b5 == 1)&&(b5_prev == 1)) { // проверка длинного нажатия
          if(up_pressed && press_up_start_time > 0) {
            if((current_time - press_up_start_time) >= LONG_UP_PRESS_TIME) {
              uint8_t data = 5;
              xQueueSend(q, &data, 0);
              press_up_start_time = current_time; // Сбрасываем таймер, чтобы при долгом нажатии можно было снова считать
              up_pressed = true;
            }
          }
        }
        if((b5 == 1)&&(b5_prev == 0)) { // если кнопка нажата
          vTaskDelay(queue_delay / portTICK_PERIOD_MS);
          b5 = digitalRead(up_pin);
          if((b5 == 1)&&(b5_prev == 0)){
            uint8_t data = 5; // 5, если кнопка 5 нажата
            xQueueSend(q, &data, 0);
            press_up_start_time = current_time; // Начало отсчета возможного длинного нажатия
            up_pressed = true;
          }
        }
        if(b5 == 0){  // Если кнопка отпущена
          if(up_pressed) {
            up_pressed = false;
            press_up_start_time = 0;
          }
        }
        b5_prev = b5;
        // Кнопка 6
        int b6 = digitalRead(down_pin);
        if((b6 == 1)&&(b6_prev == 1)) { // проверка длинного нажатия
          if(down_pressed && press_down_start_time > 0) {
            if((current_time - press_down_start_time) >= LONG_UP_PRESS_TIME) {
              uint8_t data = 6;
              xQueueSend(q, &data, 0);
              press_down_start_time = current_time; // Сбрасываем таймер, чтобы при долгом нажатии можно было снова считать
              down_pressed = true;
            }
          }
        }
        if((b6 == 1)&&(b6_prev == 0)) { // если кнопка нажата
          vTaskDelay(queue_delay / portTICK_PERIOD_MS);
          b6 = digitalRead(down_pin);
          if((b6 == 1)&&(b6_prev == 0)){
            uint8_t data = 6; // 5, если кнопка 5 нажата
            xQueueSend(q, &data, 0);
            press_down_start_time = current_time; // Начало отсчета возможного длинного нажатия
            down_pressed = true;
          }
        }
        if(b6 == 0){  // Если кнопка отпущена
          if(down_pressed) {
            down_pressed = false;
            press_down_start_time = 0;
          }
        }
        b6_prev = b6; 
        vTaskDelay(5 / portTICK_PERIOD_MS); //10 Приостанавливает выполнение текущей задачи на 10 миллисекунд. portTICK_PERIOD_MS - перевод "тиков" в мс
    }
  }
  public:
  Buttons(){
    pinMode(on_off_pin, INPUT);
    pinMode(ok_pin, INPUT);
    pinMode(left_pin, INPUT);
    pinMode(right_pin, INPUT);
    pinMode(up_pin, INPUT);
    pinMode(down_pin, INPUT);
    q = xQueueCreate(5, sizeof(uint8_t)); // 2 - приоритет (выше, чем у loop), это запуск потока
    longPressQueue = xQueueCreate(5, sizeof(uint8_t));
    if(q == NULL) {
      Serial.println("ERROR: Failed to create queue!");
      return;
    }
    xTaskCreate(buttonReaderStatic, "btn", 2048, this, 2, NULL);
  }

  int get_button_number(){
    if(xQueueReceive(q, &data, 0) == pdTRUE) {
      return(data);
    } else{
      return(0);
    }
  }

  bool ok_long_pressed() {
    uint8_t btnData;
    if(xQueueReceive(longPressQueue, &btnData, 0) == pdTRUE) {
      return (btnData == 7);
    }
    return false;
  }
};


class Settings{ // класс для работы с настройками
  private: 
  Preferences preferences;
  int ok_counter = 0; // счетчик кнопки Ok, нужен для того, чтобы войти в настройки при непрерывном нажатии кнопки Ok в течение секунды
  int delay_between_loop_iter = 0;
  const int ok_click_time = 1500; // время, которое надо зажимать кнопку ok, чтобы войти в настройки
  int ok_num = -1;
  int delay_after_on_off_click = 0;
  static const int parametr_n = 2; // количество параметров (без выхода)
  const int n_delay = 4; // на сколько делить базовый интервал задержки после срабатывания кнопки
  Display& display; // сейчас обьект передается просто по ссылке. возможно, сделать unique ptr
  Buttons& buttons;
  Parametr parametr[parametr_n];
  uint32_t measure_time = 1;
  uint32_t led_time = 1; //в конструкторе сделать чтение из глобальной памяти
  public:
  Settings(int delay_between_loop_iter_, int delay_after_on_off_click_, Display& display_, Buttons& buttons_):display(display_), buttons(buttons_){ //инициализируем ссылку
    delay_between_loop_iter = delay_between_loop_iter_; 
    ok_num = (int)(ok_click_time/delay_between_loop_iter);
    delay_after_on_off_click = delay_after_on_off_click_;
  }

  void begin(){
    parametr[0].low_border = 0; // свечение
    parametr[0].high_border = 180;
    parametr[0].number = read_led_num();
    parametr[0].k = 10;
    parametr[1].low_border = 1; // измерение
    parametr[1].high_border = 180;
    parametr[1].number = read_measure_num();
    parametr[1].k = 10;
    led_time = read_led_time();
    measure_time = read_measure_time();
  }  

  bool check_input_settings(){
    bool res = buttons.ok_long_pressed(); //get_button_number(); //digitalRead(ok_pin);
    return(res);
  }

  void input_settings(){
    if(check_input_settings()==false){
      return;
    } else{
      int parametr_number = 0; // номер параметра, выбранного для изменения
      int myArray[] = {-1, -1};
      display.print_message((11+parametr_number), myArray);
      delay(delay_after_on_off_click); // возможно, увеличить
      bool choice_paramter = false; // если пользователь выбрал параметр, то true, если снова вернулся в выбор параметров - false
      while(true){
        display.update_charge();
        // здесь чтение данных с кнопок
        /*int ok = digitalRead(ok_pin);
        int left = digitalRead(left_pin);
        int right = digitalRead(right_pin);    
        int down = digitalRead(down_pin);
        int up = digitalRead(up_pin);*/ // удалить потом
        int button_num = buttons.get_button_number();
        if(choice_paramter == false){
          if(button_num == 4){// правая кнопка нажата
            parametr_number++;
            parametr_number = parametr_number%(parametr_n + 1);
            display.print_message((11+parametr_number), myArray);
            //delay((int)(delay_after_on_off_click/n_delay));
          }else if(button_num == 3){// левая кнопка нажата
            parametr_number--;
            if(parametr_number == -1){
              parametr_number = 2;
            }
            display.print_message((11+parametr_number), myArray);
            //delay((int)(delay_after_on_off_click/n_delay));
          }else if(button_num == 2){ // кнопка ok нажата
            if(parametr_number == parametr_n){
              save_parametrs(parametr[0].number, parametr[1].number);
              display.print_message(10, myArray);// вывод на экран сообщения 10
              //delay(delay_after_on_off_click);
              break;
            } else{
              myArray[0] = parametr[parametr_number].number*parametr[parametr_number].k;
              display.print_message((11+parametr_number), myArray);
              //delay((int)(delay_after_on_off_click/n_delay));
            }
            choice_paramter = true;
          }
        } else{
          if(button_num==2){ // выход из изменения параметра
            choice_paramter = false;
            myArray[0] = -1;
            display.print_message((11+parametr_number), myArray); 
            //delay((int)(delay_after_on_off_click/n_delay));
          } else if(button_num==5){
            uint32_t tmp = parametr[parametr_number].number;
            tmp++;
            if(tmp > parametr[parametr_number].high_border){
              tmp--;
            }
            parametr[parametr_number].number = tmp;
            myArray[0] = parametr[parametr_number].number*parametr[parametr_number].k;
            display.print_message((11+parametr_number), myArray); 
            //delay((int)(delay_after_on_off_click/(n_delay))); // возможно, 
          } else if(button_num == 6){
            uint32_t tmp = parametr[parametr_number].number;
            tmp--;
            if(tmp < parametr[parametr_number].low_border){
              tmp++;
            }
            parametr[parametr_number].number = tmp;
            myArray[0] = parametr[parametr_number].number*parametr[parametr_number].k;
            display.print_message((11+parametr_number), myArray); 
            //delay((int)(delay_after_on_off_click/(n_delay)));              
          }
        }
        delay(delay_between_loop_iter);
      }
    }
    led_time = read_led_time();
    measure_time = read_measure_time();
  }

  void save_parametrs(uint32_t led, uint32_t measure) { // Функция для сохранения двух переменных типа char
    preferences.begin("appChars", false);
    preferences.putUInt("led_number", led);
    preferences.putUInt("measure_number", measure);
    preferences.end();
  }

  uint32_t read_led_time() {
    preferences.begin("appChars", true);
    uint32_t tmp = preferences.getUInt("led_number", 0);
    uint32_t res = tmp*parametr[0].k;
    preferences.end();
    return res;
  }

  uint32_t read_measure_time() {
    preferences.begin("appChars", true);
    uint32_t tmp = preferences.getUInt("measure_number", 0);
    uint32_t res = tmp*parametr[1].k;
    preferences.end();
    return res;
  }

  uint32_t read_led_num() {
    preferences.begin("appChars", true);
    uint32_t res = preferences.getUInt("led_number", 0);
    preferences.end();
    return res;
  }

  uint32_t read_measure_num() {
    preferences.begin("appChars", true);
    uint32_t res = preferences.getUInt("measure_number", 0);
    preferences.end();
    return res;
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



int sensor_1[7];
int sensor_2[7];
int common[7];
Display display; // конструктор не принимает параметров, значит скобки не нужны
Buttons buttons;
Kuler kuler;
void setup() {
  //pinMode(on_off_pin, INPUT);
  display.begin(); 
  kuler.begin();
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Wire.begin();

  if(led_between_warm){
    led_time_counter = warming_time;
  }
  for(int i=0; i<7; i++){
  sensor_1[i]=1;
  sensor_2[i]=1;
  common[i]=1;
}
}

Settings settings(delay_between_loop_iter, delay_after_on_off_click, display, buttons);
int data_1[5];
int data_2[5];

int n=0;
int myArray[] = {3, 3};
bool reset = false;
bool warm_completed = false; // нужен, чтобы если кнопка зажата при включении, не было наложения алгоритма нажатой кнопки на алгоритм проверки и прогрева  датчиков
double k = 0; // коэффициэнт коррекции при отключении датчика из-за ошибки 15
bool first_loop = true; // флаг первой итерации
bool low_percent_message = false; // true, если выведено сообщение о малом заряде аккумулятора


void loop() { // данные не пишутся на флешку перед прогревом. попробовать писать данные на флешку
  int local_loop_counter = 0; // локальный счетчик итераций цикла loop (обнуляется после конца каждого измерения)
  int on = buttons.get_button_number();//  digitalRead(on_off_pin);
  int izmer_counter = 0;
  bool stop_flag = false;
  int arr_counter = 0; // счетчик для заполнеия массивов с данными датчиов
  int reboot_display_counter = 0;
  settings.input_settings(); // флаг добавить?
  led_time = settings.read_led_time();
  measure_time = settings.read_measure_time();
  display.update_charge();
  if(first_loop){
    settings.begin();
    first_loop = false;
  }
  /*if((on==1)&&(low_percent_message==true)){ // печать сообщения Чтобы начать измерение, нажмите старт при повторном нажатии кнопки старт/стоп
    low_percent_message = false;
    display.print_message(10, myArray);
  }*/
  max_loop_iter = measure_time*measure_count + measure_count*led_time;
  if((on==1)&&(warm_completed)&&(display.get_percents()==0)) {
    if(low_percent_message==false){
      display.print_message(14, myArray);
    }
    low_percent_message = true;
  } else if((on==1)&&(warm_completed)){ // 1
    int stop = 2;
    bool first_iteration = true; // флаг нужен, чтобы выводить на экран 1 раз, иначе мерцание возникает
    int counter_for_charge = 0;
    while(!SD.begin()){
      for(int i=0; i<38; i++){
        display.update_charge();////
      }
      
      if(first_iteration == true){
        first_iteration = false;
        display.print_message(3, myArray);
      }
      stop = buttons.get_button_number();
      if(stop==1){
        break;
      }
      //counter_for_charge++;
    }
    if(stop!=1){
      do_measure = true;
      do_work = true;
      appendFile(SD, path, "CO2_1;CO2_2;CO2_medium;Temp_1;Temp_2;Temp_medium;Accuracy_1;Accuracy_2;Min_CO2_1;Min_CO2_2; time"/*"CO2_1;CO2_2;CO2_3;CO2_medium;Temp_1;Temp_2;Temp_3;Temp_medium;Accuracy_1;Accuracy_2;Accuracy_3;Min_CO2_1;Min_CO2_2;Min_CO2_3; time"*/, true);
      delay(100);  // 500
    }
    if(stop == 1){
      display.print_message(10, myArray);
      delay(delay_after_on_off_click);
    }
  }
  while(do_work){
    if(display.command_10_printed()){
      reboot_display_counter++;
      if(reboot_display_counter==2000){ // раз в минуту, заменить на рассчет 
        reboot_display_counter = 0;
        display.print_message(10, myArray);
      }
    }
    //static int izmer_counter = 0; // счетчик количества измерений (для вывода на экран)
    bool first_reset = false;
    bool second_reset = false;
    bool no_print_display=false; // флаг, чтобы сообщение "проверка устройства" не переизображалось несколько раз

    bool hot = false;
    if(led_rele == 2){
      int check = kuler.check_hot();
      if(kuler_rele == 2){ 
        if(check == 1){//kuler.check_cold()){ // радиатор достаточно охладился
          kuler_rele = 1;
        }
      }
      if(check == 2){//kuler.check_hot()){ // радиатор достаточно прогрелся?
        kuler_rele = 2;
      }
    }


    if((reset == true)&&(loop_counter==2)){ // т.е. ошибка 15 отработана реле // loop_counter
      no_print_display = true;
      reset=false;
      loop_counter=0;
    }
    if(led_time_counter == led_time){ // нужное время прошло и реле надо отключить
      led_rele = 1; // 1 - выключено
      kuler_rele = 1;
    }
    if(((loop_counter==(read_between_warm + 3))&&(led_between_warm==false))||((do_measure==true)&&(local_loop_counter==0))){ // включение реле после прогрева. если read_between_warm + 2, то включается за 3 секунды до конца прогрева  // loop_counter
      led_rele = 2;
    }
    if(loop_counter == 0){// этот блок вызывается только на первой итерации цикла loop  // loop_counter
      led_time = settings.read_led_time(); // ????
      measure_time = settings.read_measure_time();
      max_loop_iter = measure_time*measure_count + measure_count*led_time;
      if(no_print_display==false){
        display.print_message(1, myArray);
      }
      int k = first_warming_time*100;
      for(int i=0; i<k; i++){
        display.update_charge();
        delay(9);
        delayMicroseconds(250);
      }
    }
    if(loop_counter == read_between_warm){  // loop_counter
      if(data_1[4]==15){
        first_reset = true;
        reset = true;
      }
      if(data_2[4]==15){
        second_reset = true;
        reset = true;
      }
      if(reset == true){ // здесь добавить блок отправки команды на ардуино, команда 11 
        loop_counter = 0; 
        if(first_reset==true){
          sensor_rele = 20;
        } else{
          sensor_rele = 10;
        }
        if(second_reset==true){
          sensor_rele = sensor_rele + 2;
        } else{
          sensor_rele = sensor_rele + 1;
        }
      } else{
        write_sd_flag = true;
      }
    }



    switch(loop_counter) { // если loop_counter<9, читаем данные. если loop_counter = 10, то command = 5
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
          if((izmer_counter%measure_count)==0){
            display.update_charge();
            myArray[0] = (int)(izmer_counter/measure_count);
            display.print_message(2, myArray);
            led_time_counter++; 
          }
          izmer_counter++;
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
    //конец блока, который надо бы в класс засунуть

    if(send_last_message == true){
      do_measure = false;
      do_work = false;
      stop_flag = true;
      send_last_message = false;
      break;
    }

    // Усреднение данных
    if((counter_for_medium_1 >= measure_count) && (counter_for_medium_2 >= measure_count)) {
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

      ppm_common = (ppm_1_medium + ppm_2_medium/* + ppm_3_medium*/) / 2;//3;
      temp_common = (temp_1_medium + temp_2_medium/* + temp_3_medium*/) / 2;//3;
      if(do_measure == true){
        if((ppm_1_medium == 15)||(ppm_2_medium == 15)||((ppm_1_medium > 0)&&(ppm_1_medium < 150))){
          if(((ppm_1_medium == 15)&&(ppm_2_medium != 15)) || ((ppm_1_medium > 0)&&(ppm_1_medium < 150))){ // датчик 1 выдал ошибку 15 (блок ИЛИ убрать потом, это эрзац-решение для испытаний в начале октября!)       
            if(k==0){
              int common_sum = 0;
              int sensor_2_sum = 0;
              for(int i = 0; i<7; i++){
                int arr_counter_tmp = arr_counter;
                if(arr_counter_tmp==0){
                  arr_counter_tmp = 6;
                }else{
                  arr_counter_tmp--; // проверить, верно ли это работает
                }
                if((i!=arr_counter)&&(i!=arr_counter_tmp)){ // сделать проверку на  arr_counter%7>0
                  common_sum += common[i];
                  sensor_2_sum += sensor_2[i];
                }
              }
              if(sensor_2_sum==0){
                sensor_2_sum=1;
              }
              k = (double)common_sum/(double)sensor_2_sum;
            }
            ppm_common = k*ppm_2_medium;
          }
          if((ppm_2_medium == 15)&&(ppm_1_medium != 15)){ // датчик 2 выдал ошибку 15
            if(k==0){
              int common_sum = 0;
              int sensor_1_sum = 0;
              for(int i =0; i<7; i++){
                int arr_counter_tmp = arr_counter;
                if(arr_counter_tmp==0){
                  arr_counter_tmp = 6;
                }else{
                  arr_counter_tmp--;
                }
                if((i!=arr_counter)&&(i!=arr_counter_tmp)){
                  common_sum += common[i];
                  sensor_1_sum += sensor_1[i];
                }
              }
              if(sensor_1_sum==0){
                sensor_1_sum=1;
              }
              k = (double)common_sum/(double)sensor_1_sum;
            }
            ppm_common = k*ppm_1_medium;
          }  
        } else{
          sensor_1[arr_counter] = ppm_1_medium;
          sensor_2[arr_counter] = ppm_2_medium;
          common[arr_counter] = ppm_common;
        }
      }
    
      if(loop_counter < read_between_warm){
        data_1[(int)(loop_counter/measure_count)] = ppm_1_medium;
        data_2[(int)(loop_counter/measure_count)] = ppm_2_medium;
      }
      if(write_sd_flag){
        write_data_to_sd(ppm_1_medium, ppm_2_medium, ppm_common, temp_1_medium, temp_2_medium, temp_common,
                     accuracy_1, accuracy_2, min_co2_1, min_co2_2, time_s);
      time_s = time_s + time_step;
      }
    } 

    Serial.println();
    if(loop_counter == read_between_warm){
      for(int i=warming_time; i>=7; i--){
          // здесь же вызов check_pribor
          myArray[0]=i;
          display.print_message(0, myArray);
          for(int j=0; j<iter_to_one_second_in_warm; j++){ // 93, а не 100, так как update_charge тратит время на выполнение себя 
            display.update_charge();
            delay(delay_between_loop_iter); //(int)delay_in_command0/100);
          }
        }
      //delay((warming_time + 1) * 1000);    
    } else if(read_co2 == true){
      //int cpu_time = (1/measure_count) - delay_between_readings;
      //int step = (int)(delay_between_readings/27); // delay_between_loop_iter
      //delay(cpu_time);
      int off[1];
      bool button = false;
      if(warm_completed){
        off[0] = buttons.get_button_number();
        if(off[0]==1){
          button = true;
        }
        for(int i=1; i<27; i++){ // иначе не откалибровать точно
          display.update_charge();
          delay(9);
          delayMicroseconds(425);
          off[0] = buttons.get_button_number();
          if(off[0]==1){
            button = true;
          }
        }
      }
      if((local_loop_counter>=measure_count*1)&&(button==true/*(off[0]==1)||(off[1]==1)||(off[2]==1)||(off[3]==1)||(off[4]==1)||(off[5]==1)||(off[6]==1)*/)){ // local_loop_counter>=9 сделано для того, чтобы первые 3 секунды кнопка не считывалась, это надо, чтобы не было такого, что нажал кнопку старт и тут же все остановилдось 
        send_last_message = true;
        led_rele = 1; // 0 - не инициализировано, 1 - выключено, 2 - включено
        kuler_rele = 1; // 0 - не инициализировано, 1 - выключено, 2 - включено
      }
    } else{
      // здесь же вызов check_pribor
      if(loop_counter == (read_between_warm + 1)){ // эта задержка в 3 секунды связана с тем, что отправляется команда set_zero_point
        for(int i=6; i>=3; i--){
          myArray[0]=i;
          display.print_message(0, myArray);
          for(int j=0; j<iter_to_one_second_in_warm; j++){ // 93, а не 100, так как update_charge тратит время на выполнение себя 
            display.update_charge();
            delay(delay_between_loop_iter); //(int)delay_in_command0/100);
          }
        }
      }else if(loop_counter == (read_between_warm + 2)){ // эта задержка в 3 секунды связана с тем, что отправляется команда на автокалибровку
        // тут есть затык примерно в 1 секунду, пока предлагаю так оставить (связано, скорее всего, с отправкой сообщения)
        for(int i=3; i>=0; i--){
          myArray[0]=i;
          display.print_message(0, myArray);
          for(int j=0; j<iter_to_one_second_in_warm; j++){ // 93, а не 100, так как update_charge тратит время на выполнение себя 
            display.update_charge();
            delay(delay_between_loop_iter); //(int)delay_in_command0/100);
          }
        }
        do_measure = false;
        do_work = false;
        warm_completed = true; 
        display.print_message(10, myArray);
        SD.end();
      } else{
        delay(3000);
      }
    }
    if((reset == true)&&(loop_counter==0)){ // задержка, равная времени, когнда реле разомкнуто
      int k = rele_open_time*100;
      for(int i=0; i<k; i++){
        display.update_charge();
        delay(9);
        delayMicroseconds(250);
      }
    }
    if((reset == true)&&(loop_counter==1)){ // для того, чтобы реле успевало разомкнуться
      delay(1000);
    }
    delete []message;
    //начало блока, который блокируется при нажатии кнопки стоп
    loop_counter++;
    local_loop_counter++;
    if(local_loop_counter%measure_count==0){
      arr_counter++;
      arr_counter = arr_counter%7;
    }
    if((local_loop_counter == max_loop_iter)){
      do_measure=false;
      do_work = false;
      display.print_message(10, myArray);
      SD.end();
    }
    }
    local_loop_counter=0;
    led_time_counter = 0;
    time_s = 0;
    sensor_rele = 0;
    led_rele = 0; // 0 - не инициализировано, 1 - выключено, 2 - включено
    kuler_rele = 0; // 0 - не инициализировано, 1 - выключено, 2 - включено
    if(stop_flag==true){
      display.print_message(10, myArray);
      SD.end();
      delay(delay_after_on_off_click); // задержка от повторного считывания кнопки старт/стоп
    } else{
      delay(delay_between_loop_iter);
    }
  }