#include <Wire.h>
#include <ESP8266_LCD_1602_RUS.h>
#include <font_LCD_1602_RUS.h>


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

Display display; // конструктор не принимает параметров, значит скобки не нужны
void setup() {
  display.begin(); 
}
int myArray[] = {3, 3};
void loop() {
  static int charge=100; // будет инкремент нормально работать со static!
  static int com = 0;
  static int time_warm = 200;
  static int time_izmer = 0;
  // put your main code here, to run repeatedly:
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
 display.print_message(charge, com, myArray);//(int charge, int num_message){
    charge --;
    time_warm--;
    time_izmer++;

  
  if(charge==0){
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
  delay(3000);
}
