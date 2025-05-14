#include <ESP8266_LCD_1602_RUS.h>

LCD_1602_RUS lcd(0x27, 16, 2);

void setup()
{
  String str;
  str = ". Hexadecimal";

  lcd.init(); //Инициализация LCD по умолчанию для ESP8266 (4 - SDA, 5 - SCL)
  //lcd.init(0, 2); //ESP8266-01 I2C: 0-SDA 2-SCL

  // Печать сообщения на LCD
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(1, DEC);
  lcd.print(". Десятичная");
  lcd.setCursor(0, 1);
  lcd.print(15, HEX);
  lcd.print(str);
}

void loop()
{
}
