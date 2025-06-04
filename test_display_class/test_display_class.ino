#include <Wire.h>
#include <ESP8266_LCD_1602_RUS.h>
#include <font_LCD_1602_RUS.h>



/*struct Voltage_Percent {
  float voltage;
  float percent;
};*/

struct LedCorrection {
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
    Serial.println(charge);
    Serial.println(result);
    Serial.println("");
    return(result);
  }


  int convert_charge_to_percent(float medium){ // конвертация данных с делителя в проценты
    /*float voltage = medium/volt_1;
    if (voltage >= voltage_percent_list[0].voltage) return 100;
    if (voltage <= voltage_percent_list[tableSize - 1].voltage) return 0.0;*/
    
    float percent_1 = (high_border - low_border)/100;
    int percents = (int)((medium - low_border)/percent_1);
    Serial.println(percent_1);
    if(percents>100){
      return(100);
    }
    if(percents<0){
      return(0);
    }
    return(percents);

    /*float medium_volts = medium/volt_1; // /3 убрать, если все ок. для этого внедрить в массив структур границы x3
    return(voltage_to_percent(medium_volts));*/

    // Поиск интервала для интерполяции
    /*for (int i = 0; i < tableSize - 1; ++i) {
        if (voltage <= voltage_percent_list[i].voltage && voltage > voltage_percent_list[i + 1].voltage) {
            return ((int)(voltage_percent_list[i].percent + (voltage - voltage_percent_list[i].voltage) * 
                  (voltage_percent_list[i + 1].percent - voltage_percent_list[i].percent) / 
                  (voltage_percent_list[i + 1].voltage - voltage_percent_list[i].voltage)));
        }
    }
    return 0; // На случай ошибки*/
  }
};




class Display{
  private:
  bool first_print = true; // первая печать на очищенном экране или просто первая печать в программе
  bool first_com0_print = true; // первая печать на экран  коианды 0, так как дальше обновляется только количество секунд
  bool first_com2_print = true; // первая печать на экран  коианды 2, так как дальше обновляется только количество секунд
  int counter = 0; // счетчик итераций (надо для отображения заряда)
  int sum_charge_12 = 0; // сумма показаний с делителя напряжения на 12V
  int max_iter = 200; // количество измерений для усреднения
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


















int count=-11;
Display display; // конструктор не принимает параметров, значит скобки не нужны
void setup() {
  display.begin(); 
    Serial.begin(115200);

}

int myArray[] = {3, 3};
void loop() {
  //static int count=-11; // будет инкремент нормально работать со static!
  static int com = 0;
  static int time_warm = 200;
  static int time_izmer = 0;
  static int loop_counter = 0;
  static int sum = 0;
  display.update_charge();


  
  /*if(com==0){
    myArray[0] = time_warm;
  }*/
  /*if(com==2){
    myArray[0] = time_izmer;
  }*/
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
 

  if(loop_counter%5 == 0){ //
  if(com==2){
     time_izmer++;
    myArray[0] = time_izmer;
  }
    if(com==0){
      for(int i=time_warm; i>=0; i--){
        // здесь же вызов check_pribor
        myArray[0]=i;
        display.print_message(com, myArray);
        display.update_charge();
        delay(960); // это сделать константой, это 1 секунда при прогреве!!
      }
    } else{
      display.print_message(com, myArray);
    }
    if(com!=2){
      com++;
    }
  }
  
  if(com>9){
    com=0;
  }
  delay(180);
  loop_counter++;
}
