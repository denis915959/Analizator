#include <Wire.h>
/*#include <LCD_1602_RUS.h>
#include <font_LCD_1602_RUS.h>*/
#include <ESP8266_LCD_1602_RUS.h>
#include <font_LCD_1602_RUS.h>



byte customBat[8] = { // символ заряда батареи
  0b11111,  // █████
  0b11111,  // █   █
  0b11111,  // █   █
  0b11111,  // █ █ █
  0b11111,  // █ █ █
  0b11111,  // █   █
  0b11111,  // █   █
  0b00000   // █████
};

byte custom_B[8] = {
  /*0b11110,  //  ### 
  0b10000,  // #    
  0b10000,  // #    
  0b11110,  // #### 
  0b10001,  // #   #
  0b10001,  // #   #
  0b10001,  // #   #
  0b11110   //  ### */
  0x1F,
  0x10,
  0x10,
  0x1E,
  0x11,
  0x11,
  0x11,
  0x1E
};

LCD_1602_RUS lcd(0x27, 20, 4); // Адрес I2C 0x27, 16x2

void setup() {
  lcd.init();         // Инициализация
  lcd.backlight();    // Включение подсветки
  lcd.createChar(5, custom_B); // Регистрируем символ под номером 3. не проверял номера 0, 1, 2
  lcd.createChar(7, customBat);
  // Вывод статичного русского текста
  
}

void loop() {
  static int n = 0;
  lcd.clear();
  delay(50);
  lcd.setCursor(0, 0);
  lcd.print(n, DEC);
  lcd.print("% [");
  for (int i=0; i<5; i++){
    lcd.write(byte(7)); // print("|"); 
  }
  lcd.print("]");

  // Динамическое обновление значения
  /*lcd.setCursor(9, 0);
  lcd.print("   ");    // Очистка предыдущего значения
  lcd.setCursor(9, 0);*/
  //lcd.print(n);
  //lcd.print("%");
  if(n%8==0){
    /*lcd.setCursor(0, 0);
  lcd.print("3AP");
    lcd.print("ЯД:");*/
    lcd.setCursor(0, 1);
    lcd.print("П"); // rus
    lcd.print("POBEPKA ");
    lcd.print("OKOH");
    lcd.print("Ч"); // rus
    lcd.print("EHA.");

    // калибровка завершится
    lcd.setCursor(0, 2);
    lcd.print("П"); // rus
    lcd.print("PO");
    lcd.print("Г"); // rus
    lcd.print("PEB 3ABEP"); // ENG
    lcd.print("ШИ"); // rus
    lcd.print("TC"); // ENG
    lcd.print("Я"); // rus


    // через 10 сек
    lcd.setCursor(0, 3);
    lcd.print("Ч"); // rus
    lcd.print("EPE3 100 CEK");
    /*lcd.setCursor(0, 2);
    lcd.print("Прогрев завершится");
    lcd.setCursor(0, 3);
    lcd.print("через");*/
  } 
  if(n%8==1){
    /*lcd.setCursor(0, 0);
    lcd.print("3AP");
    lcd.print("ЯД:");*/
    lcd.setCursor(0, 1);
    lcd.print("П");
    lcd.print("POBEPKA YCTPO");
    lcd.print("Й");
    lcd.print("CTBA");
  }
  if(n%8==2){
    // измерение:
    lcd.setCursor(0, 1);
    lcd.print("И"); // rus
    lcd.print("3MEPEH");
    lcd.print("И"); // rus
    lcd.print("E: ");
    lcd.print("100 CEK");
  }
  if(n%8==3){
    lcd.setCursor(0, 1);
    lcd.print("BCTABbTE SD-KAPTY");
  }
  if(n%8==4){
    lcd.setCursor(0, 1);

    lcd.print("HET COE");
    lcd.print("ДИ"); // rus
    lcd.print("HEH");
    lcd.print("ИЯ"); // rus
    lcd.print(" C");
   
    lcd.setCursor(0, 2);
    /*lcd.write(byte(5));//0xA2);//byte(3));
    lcd.print("БЛ"); // rus // с маленькой б работает
    lcd.print("OKOM CEHCOPOB");*/// работает сейчас!
    
    lcd.print("И"); // rus
    lcd.print("3MEP");
    lcd.print("И"); // rus
    lcd.print("TE");
    lcd.print("Л"); // rus
    lcd.print("bH");
    lcd.print("Ы");
    lcd.print("M");
    /*lcd.print("БЛ"); // rus
    lcd.print("OKOM");
    //lcd.print("Л"); // rus*/
    lcd.setCursor(0, 3);
    lcd.print("MO");
    lcd.print("Д"); // rus
    lcd.print("Y");
    lcd.print("Л"); // rus
    lcd.print("EM");
  }
  if(n%8==5){
    lcd.setCursor(0, 1);
    lcd.print("3AMEH");//COE");
    lcd.print("И"); // rus
    lcd.print("TE ");
    lcd.print("Д"); // rus
    lcd.print("AT");
    lcd.print("ЧИ"); // rus
    lcd.print("K ");//CEHCOP ");
    /*lcd.print("Д"); // rus
    lcd.print("AT");
    lcd.print("ЧИ"); // rus
    lcd.print("K ");*/
    int k=2;
    lcd.print(k, DEC);
  }
  if(n%8==6){
    lcd.setCursor(0, 1);
    lcd.print("HET COE");//COE");
    lcd.print("ДИ"); // rus
    lcd.print("HEH");
    lcd.print("ИЯ "); // rus
    lcd.print("C");
    lcd.setCursor(0, 2);
    lcd.print("Д"); // rus
    lcd.print("AT");//CEHCOP ");
    lcd.print("ЧИ"); // rus
    lcd.print("KOM ");//CEHCOP ");
    int k=1;
    lcd.print(k, DEC);
  }
  if(n%8==7){
    lcd.setCursor(0, 1);
    lcd.print("HET COE");//COE");
    lcd.print("ДИ"); // rus
    lcd.print("HEH");
    lcd.print("ИЯ "); // rus
    lcd.print("C");
    lcd.setCursor(0, 2);
    lcd.print("Д"); // rus
    lcd.print("AT");//CEHCOP ");
    lcd.print("ЧИ"); // rus
    lcd.print("KAM");//CEHCOP ");
    lcd.print("И "); // rus
    int k=1;
    lcd.print(k, DEC);
    lcd.print(", ");
    k=2;
    lcd.print(k, DEC);
  }
  
  n = (n + 1) % 101;  // Цикл 0-100
  delay(3000);        // Задержка 1 сек
}

/*#include <Wire.h>                 // Подключаем библиотеку Wire // хоть как-то работает, но надо нормально символы прописывать
#include <LiquidCrystal_I2C.h>    // Подключаем библиотеку LiquidCrystal_I2C
 
byte bukva_B[8]   = {B11110,B10000,B10000,B11110,B10001,B10001,B11110,B00000,}; // Буква "Б"
byte bukva_G[8]   = {B11111,B10001,B10000,B10000,B10000,B10000,B10000,B00000,}; // Буква "Г"
byte bukva_D[8]   = {B01111,B00101,B00101,B01001,B10001,B11111,B10001,B00000,}; // Буква "Д"
byte bukva_ZH[8]  = {B10101,B10101,B10101,B11111,B10101,B10101,B10101,B00000,}; // Буква "Ж"
byte bukva_Z[8]   = {B01110,B10001,B00001,B00010,B00001,B10001,B01110,B00000,}; // Буква "З"
byte bukva_I[8]   = {B10001,B10011,B10011,B10101,B11001,B11001,B10001,B00000,}; // Буква "И"
byte bukva_IY[8]  = {B01110,B00000,B10001,B10011,B10101,B11001,B10001,B00000,}; // Буква "Й"
byte bukva_L[8]   = {B00011,B00111,B00101,B00101,B01101,B01001,B11001,B00000,}; // Буква "Л"
byte bukva_P[8]   = {B11111,B10001,B10001,B10001,B10001,B10001,B10001,B00000,}; // Буква "П"
byte bukva_Y[8]   = {B10001,B10001,B10001,B01010,B00100,B01000,B10000,B00000,}; // Буква "У"
byte bukva_F[8]   = {B00100,B11111,B10101,B10101,B11111,B00100,B00100,B00000,}; // Буква "Ф"
byte bukva_TS[8]  = {B10010,B10010,B10010,B10010,B10010,B10010,B11111,B00001,}; // Буква "Ц"
byte bukva_CH[8]  = {B10001,B10001,B10001,B01111,B00001,B00001,B00001,B00000,}; // Буква "Ч"
byte bukva_Sh[8]  = {B10101,B10101,B10101,B10101,B10101,B10101,B11111,B00000,}; // Буква "Ш"
byte bukva_Shch[8]= {B10101,B10101,B10101,B10101,B10101,B10101,B11111,B00001,}; // Буква "Щ"
byte bukva_Mz[8]  = {B10000,B10000,B10000,B11110,B10001,B10001,B11110,B00000,}; // Буква "Ь"
byte bukva_IYI[8] = {B10001,B10001,B10001,B11001,B10101,B10101,B11001,B00000,}; // Буква "Ы"
byte bukva_Yu[8]  = {B10010,B10101,B10101,B11101,B10101,B10101,B10010,B00000,}; // Буква "Ю"
byte bukva_Ya[8]  = {B01111,B10001,B10001,B01111,B00101,B01001,B10001,B00000,}; // Буква "Я"
 
LiquidCrystal_I2C lcd(0x27,16,2); // Задаем адрес и размерность дисплея.
 
void setup()
{
  lcd.init();                     // Инициализация lcd дисплея
  lcd.backlight();                // Включение подсветки дисплея
}
 int loop_counter=0;
void loop()
{
 loop_counter=loop_counter+1;
 lcd.createChar(1, bukva_P);      // Создаем символ под номером 1
 lcd.createChar(2, bukva_I);      // Создаем символ под номером 2
 

//Для наглядности, вывод символом указал отдельно, 
//можно минимизировать: lcd.print("\1P\2BET M\2P");

 

 lcd.setCursor(3, 0);             // Устанавливаем курсор на 1 строку ячейку 3
 lcd.write(bukva_Z);                 // Выводим букву "П"
 lcd.setCursor(4, 0);             // Устанавливаем курсор на 1 строку ячейку 4
 lcd.print("P");                  // Выводим букву "P"
 lcd.setCursor(5, 0);             // Устанавливаем курсор на 1 строку ячейку 5
 lcd.print("\2");                 // Выводим букву "И"
 lcd.setCursor(6, 0);             // Устанавливаем курсор на 1 строку ячейку 6
 lcd.print("B");                  // Выводим букву "B"
 lcd.setCursor(7, 0);             // Устанавливаем курсор на 1 строку ячейку 7
 lcd.print("E");                  // Выводим букву "E"
 lcd.setCursor(8, 0);             // Устанавливаем курсор на 1 строку ячейку 8
 lcd.print("T");                  // Выводим букву "T"
 lcd.setCursor(10, 0);            // Устанавливаем курсор на 1 строку ячейку 10
 lcd.print("M");                  // Выводим букву "М"
 lcd.setCursor(11, 0);            // Устанавливаем курсор на 1 строку ячейку 11
 lcd.print("\2");                 // Выводим букву "И"
 lcd.setCursor(12, 0);            // Устанавливаем курсор на 1 строку ячейку 12
 lcd.print("P");                  // Выводим букву "P"


 if(loop_counter%2==0){
 lcd.setCursor(3, 1);             // Устанавливаем курсор на 1 строку ячейку 3
 lcd.write(bukva_Z);                 // Выводим букву "П"
 lcd.setCursor(4, 1);             // Устанавливаем курсор на 1 строку ячейку 4
 lcd.print("P");                  // Выводим букву "P"
 lcd.setCursor(5, 1);             // Устанавливаем курсор на 1 строку ячейку 5
 lcd.print("\2");                 // Выводим букву "И"
 lcd.setCursor(6, 1);             // Устанавливаем курсор на 1 строку ячейку 6
 lcd.print("B");                  // Выводим букву "B"
 lcd.setCursor(7, 1);             // Устанавливаем курсор на 1 строку ячейку 7
 lcd.print("E");                  // Выводим букву "E"
 lcd.setCursor(8, 1);             // Устанавливаем курсор на 1 строку ячейку 8
 lcd.print("T");                  // Выводим букву "T"
 lcd.setCursor(10, 1);            // Устанавливаем курсор на 1 строку ячейку 10
 lcd.print("M");                  // Выводим букву "М"
 lcd.setCursor(11, 1);            // Устанавливаем курсор на 1 строку ячейку 11
 lcd.print("\2");                 // Выводим букву "И"
 lcd.setCursor(12, 1);            // Устанавливаем курсор на 1 строку ячейку 12
 lcd.print("P"); 
 }
 else{
lcd.setCursor(3, 1);             // Устанавливаем курсор на 1 строку ячейку 3
 lcd.write(bukva_Z);                 // Выводим букву "П"
 lcd.setCursor(4, 1);             // Устанавливаем курсор на 1 строку ячейку 4
 lcd.write(bukva_Z);                  // Выводим букву "P"
 lcd.setCursor(5, 1);             // Устанавливаем курсор на 1 строку ячейку 5
 lcd.write(bukva_Z);                 // Выводим букву "И"
 lcd.setCursor(6, 1);             // Устанавливаем курсор на 1 строку ячейку 6
 lcd.write(bukva_Z);                  // Выводим букву "B"
 lcd.setCursor(7, 1);             // Устанавливаем курсор на 1 строку ячейку 7
 lcd.write(bukva_Z);                  // Выводим букву "E"
 lcd.setCursor(8, 1);             // Устанавливаем курсор на 1 строку ячейку 8
 lcd.write(bukva_Z);                  // Выводим букву "T"
 lcd.setCursor(10, 1);            // Устанавливаем курсор на 1 строку ячейку 10
 lcd.write(bukva_Z);                  // Выводим букву "М"
 lcd.setCursor(11, 1);            // Устанавливаем курсор на 1 строку ячейку 11
 lcd.write(bukva_Z);                 // Выводим букву "И"
 lcd.setCursor(12, 1);            // Устанавливаем курсор на 1 строку ячейку 12
 lcd.write("P"); 

 }
 delay(2000);
}*/



/*#include <ESP8266_LCD_1602_RUS.h>
#include <font_LCD_1602_RUS.h>

LCD_1602_RUS lcd(0x27, 20, 4);

void setup()
{
  String str;
  str = ". Hexadecimal";
  
 
  lcd.init(); //Инициализация LCD по умолчанию для ESP8266 (4 - SDA, 5 - SCL)
  //lcd.init(0, 2); //ESP8266-01 I2C: 0-SDA 2-SCL

  // Печать сообщения на LCD
  lcd.backlight();

  lcd.setCursor(0, 0);         // Курсор: 0-й столбец, 1-я строка
  lcd.print("Заряд: ");        // Выводим текст
  //lcd.print("", DEC);
  //lcd.setCursor(7, 0);
  //lcd.print(n, DEC);
  lcd.setCursor(0, 1);
  lcd.print(15, HEX);
  lcd.print(str);
  lcd.setCursor(0, 2);
  lcd.print(1, DEC);
  lcd.print(". Десятичная");
  lcd.setCursor(0, 3);
  lcd.print(15, HEX);
  lcd.print(str);
}

int n=0;
void loop()
{
  n = (n + 1) % 101;  // Пример изменения n от 0 до 100
  if(n==0){
    lcd.setCursor(7, 0);         // Позиция после "Заряд: "
    lcd.print("    "); // затирает старую надпись
  }
  
  // Выводим обновленное значение на LCD
 
  
  //String n_arr= String(n);
  //lcd.print(n_arr);
  //lcd.print("%");
  lcd.setCursor(7, 0);         // Позиция после "Заряд: "
  lcd.print(n, DEC);           // Явное указание формата DEC
  lcd.print("%");              // Вывод знака %
  delay(300);  // Задержка 1 сек для наглядности
}*/











/*#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

hd44780_I2Cexp lcd(0x27, 20, 4); // Адрес I2C, 20 столбцов, 4 строки

void setup() {
  // Инициализация с указанием размеров (20x4)
  lcd.begin(20, 4);
  
  // Включаем подсветку (если есть)
  lcd.backlight();
  
  // Выводим текст
  lcd.print("заряд: 100%");
}

void loop() {}*/




/*#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4); // Адрес I2C, 20x4

void setup() {
  lcd.init();
  lcd.backlight();
  
  // Вывод "заряд: 100%" в кодировке CP866
  lcd.setCursor(0, 0);
  lcd.print("\xE7\xE0\xF0\xFF\xE4: 100%"); // з а р я д
}

void loop() {}*/

// Инициализация дисплея (адрес I2C обычно 0x27 или 0x3F для таких дисплеев)
/*LiquidCrystal_I2C lcd(0x27, 20, 4); // Адрес 0x27, 20 столбцов, 4 строки

void setup() {
  // Инициализация LCD
  lcd.init();
  lcd.backlight(); // Включение подсветки

  // Вывод текста на дисплей
  lcd.setCursor(0, 0); // Установка курсора в начало первой строки
  lcd.print("заряд: 100%"); // Печать текста
}

void loop() {
  // Ничего не делаем в цикле
}*/







/*void setup() { // сканирование устройств на I2c
  Wire.begin();
  Serial.begin(115200);
  Serial.println("I2C Scanner");
}

void loop() {
  byte error, address;
  int nDevices = 0;
  Serial.println("Scanning...");
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println();
      nDevices++;
    }
  }
  if (nDevices == 0) Serial.println("No I2C devices found");
  delay(5000);
}*/
