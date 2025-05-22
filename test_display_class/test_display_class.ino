#include <Wire.h>
#include <ESP8266_LCD_1602_RUS.h>
#include <font_LCD_1602_RUS.h>

const int voltage_div = 33; // пин делителя напряжения


class Display{
  private:
  bool first_print = true; // первая печать на очищенном экране или просто первая печать в программе
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

  void print_charge(int charge){ // часть метода print_battery
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
  public:
  Display() : lcd(0x27, 20, 4){}

  void print_battery(int charge){ // заряд в процентах
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
    first_print = false;
  }

  void begin() {
    lcd.init();
    lcd.backlight();
    lcd.createChar(7, customBat);
  }

  void print_message(int charge, int num_message, int arr[]){
    lcd.clear();
    first_print = true;
    delay(10);
    int k;
    print_battery(charge);
    switch(num_message) {
    case 0: // проверка окончена. прогрев завершится через n сек
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
    break;
    case 1: // проверка устройства
      lcd.setCursor(0, 1);
      lcd.print("П");
      lcd.print("POBEPKA YCTPO");
      lcd.print("Й");
      lcd.print("CTBA");
    break;
    case 2: // измерение: 
      lcd.setCursor(0, 1);
      lcd.print("И"); // rus
      lcd.print("3MEPEH");
      lcd.print("И"); // rus
      lcd.print("E: ");
      lcd.print(arr[0], DEC);
      lcd.print(" CEK");
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
};

int contrast_border(float medium){ // сравнение с порогом
  float percent_border[100];
  int low_border = 2067; //нижняя граница (8.5 В)
  int high_border = 3065;//3050;//3111; //верхняя граница (ее надо увеличить после подключения светодиода) (4.2)
  float volt_1 = (high_border - low_border)/4.1; // 3.9
  float medium_volts = medium/volt_1; // /3 убрать, если все ок. для этого внедрить в массив структур границы x3
  return(voltage_to_percent(medium_volts));
  
  //float step = (high_border - low_border)/99; // 100?
  /*for(int i=0; i<100; i++){ // это в метод begin или конструктор
    percent_border[i]= low_border + step*i;
  }*/
  /*int num =(int)((medium - low_border)/step) + 1; // номер процента. возможно, убрат +1?
  if(num<=100){
    return(num);
  } else {
    return(100);
  }*/
}

struct Voltage_Percent {
    float voltage;
    float percent;
};

const Voltage_Percent voltage_percent_list[] = {
    {12.6, 100.0}, // 12.6
    {11.8,  80.0}, // 12
    {11.2,  60.0},
    {10.8,  40.0},
    {10.3,  20.0},
    {8.5,   0.0}
};
const int tableSize = 6;//sizeof(voltage_percent_list) / sizeof(Voltage_Percent[0]);

int voltage_to_percent(float voltage) {
    // Граничные случаи
    if (voltage >= voltage_percent_list[0].voltage) return 100;
    if (voltage <= voltage_percent_list[tableSize - 1].voltage) return 0.0;

    // Поиск интервала для интерполяции
    for (int i = 0; i < tableSize - 1; ++i) {
        if (voltage <= voltage_percent_list[i].voltage && voltage > voltage_percent_list[i + 1].voltage) {
            return ((int)(voltage_percent_list[i].percent + (voltage - voltage_percent_list[i].voltage) * 
                  (voltage_percent_list[i + 1].percent - voltage_percent_list[i].percent) / 
                  (voltage_percent_list[i + 1].voltage - voltage_percent_list[i].voltage)));
        }
    }

    return 0; // На случай ошибки
}


int medium_iter=200; // количество усреднений
int charge=-11;
Display display; // конструктор не принимает параметров, значит скобки не нужны
void setup() {
  pinMode(voltage_div, INPUT);
  display.begin(); 
  delay(500);
  int sum_first = 0;
  for(int i=0; i<75; i++){
    sum_first+=analogRead(voltage_div);
    delay(10);
  }
  float medium = sum_first/75;
  charge = contrast_border(medium); // сделать глобальной
  display.print_battery(charge);
  Serial.begin(115200);

}

int myArray[] = {3, 3};
void loop() {
  //static int charge=-11; // будет инкремент нормально работать со static!
  static int com = 0;
  static int time_warm = 200;
  static int time_izmer = 0;
  static int loop_counter = 0;
  static int sum = 0;

  if(loop_counter%medium_iter == 0){
    if(loop_counter==0){
      /*int sum_first = 0;
      for(int i=0; i<75; i++){
        sum_first+=analogRead(voltage_div);
        delay(10);
      }
      float medium = sum_first/75;
      charge = contrast_border(medium); // сделать глобальной
      display.print_battery(charge);*/
      int k=0;
    }
    else{
      float medium = sum/medium_iter;
      int charge_tmp = contrast_border(medium);
      if(charge_tmp < charge){
        charge = charge_tmp;
        display.print_battery(charge);
      }
      sum = 0;
    }
  }
  sum+=analogRead(voltage_div);

  /*Serial.println(voltage_to_percent(4.2)); // 100%
  Serial.println(voltage_to_percent(4.1)); // 90%
  Serial.println(voltage_to_percent(4.0)); // 80%
  Serial.println(voltage_to_percent(3.9)); // 73%
  Serial.println(voltage_to_percent(3.8)); // 66%
  Serial.println(voltage_to_percent(3.7)); // 60%
  Serial.println(voltage_to_percent(3.6)); // 50%
  Serial.println(voltage_to_percent(3.5)); // 40%
  Serial.println(voltage_to_percent(3.4)); // 30%
  Serial.println(voltage_to_percent(3.3)); // 20%
  Serial.println(voltage_to_percent(2.9)); // 10% // надо сделать диапазон 2.9-3.3 вместо 2.5-3.3
  delay(10000);*/
  
  if(com==0){
    myArray[0] = time_warm;
  }
  if(com==2){
    myArray[0] = time_izmer;
  }
  if(com==5){
    myArray[0] = 1;
  }
  if(com==6){
    myArray[0] = 2;
  }
  if(com==7){
    myArray[0] = 3;
    myArray[1] = 3;
  }
 
    /*charge --;
    time_warm--;
    time_izmer++;*/

  if(loop_counter%30 == 0){// раскомментировать!
    display.print_message(charge, com, myArray);
    com++;
  }
  
  /*if(charge==0){
  charge = 100;}
  for(int i=0; i<7; i++){ // 9
    delay(1000);
    display.print_battery(charge);
    charge--;
  } 
  if(charge<0){
    charge=100;
  }
  com ++;
  com = com%10;
  delay(3000);*/
  if(com>9){
    com=0;
  }
  delay(160);
  loop_counter++;
}
