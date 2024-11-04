/*
 * LCD_1602.h
 *
 * Created: 18-Apr-24 8:17:12 PM
 *  Author: GiangTrinh
 */ 


#ifndef LCD_1602_H_
#define LCD_1602_H_

void LCD_Init(void);                  // Initialize LCD1602 - mode 4bit
void LCD_SendCommand(char Command);   // send 8bit , 4 high, 4 low
void LCD_Gotoxy(char x,char y);	      // move cursor to column x, row y
void LCD_PutChar(char Data);          // display 1 char
void LCD_Puts(char* s);               // display string
void LCD_WriteSymbol(char number,char symbol[8]); // display symbol, get symbol code from Symbol.h


#endif /* LCD_1602_H_ */