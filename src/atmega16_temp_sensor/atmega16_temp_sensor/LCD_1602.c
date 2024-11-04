/*
 * LCD_1602.c
 *
 * Created: 18-Apr-24 8:12:59 PM
 *  Author: GiangTrinh
 */ 

/************************************************************************/
/* Definitions                                                          */
/************************************************************************/
#define F_CPU 8000000UL // Select the same 8MHz frequency as the external clock source
#include <avr/io.h>     // Support I/O control and AVR register
#include <util/delay.h> // Delay library
#include "LCD_1602.h"   // API for LCD_1602

// Priority C port for LCD
// 4-7 port C pins compatible with D4-D7 LCD pins
#define LCD_PORT PORTC
// pin definition 0 1 2 is RS RW EN
#define LCD_RS 0
#define LCD_RW 1
#define LCD_EN 2
// Note that in order to output signals to port C, it is necessary to set the signals on DDRC to 1.
// This will do in the main.c 'Setup_Pinmode()'

/************************************************************************/
/* Prototypes                                                           */
/************************************************************************/
static void LCD_Enable(void); // 1 Pulse Clock for order execution
static void LCD_Send4Bit(char Data); // send 4bit
static void LCD_Clear(void); // Erase LCD
/************************************************************************/
/* Code                                                                 */
/************************************************************************/
void LCD_Init(void)// Initialize LCD1602
{
	_delay_ms(50);// delay min 15ms after power 4.5v
	LCD_PORT &= ~(1<<LCD_RS); //rs_pin = 0 execute mode, 1 output data mode
	LCD_PORT &= ~(1<<LCD_RW); //rw_pin = 0 write mode, 1 read mode
	
	// steps to boot start LCD
	// send 0x03 3 times to default interface with LCD1602 at 8bit mode
	// 4bit D0 to D3 ignores these pins
	LCD_Send4Bit(0x03);
	_delay_ms(10); // delay 4.1 ms
	LCD_Send4Bit(0x03);
	_delay_ms(10); // delay 100 us
	LCD_Send4Bit(0x03);
	_delay_ms(10); // delay more
	LCD_Send4Bit(0x02); // set up interface 4bit mode
	_delay_ms(10); // delay more
	
	// set function
	LCD_SendCommand(0x28); // set function line, font for 4bit mode
	LCD_SendCommand(0x08); // off screen
	LCD_SendCommand(0x0c); // on display screen and disable display cursor
	LCD_SendCommand(0x06); // auto jump cursor to next coordinate
	
	LCD_Clear();
	return;
}
static void LCD_Enable(void) // order execution
{
	LCD_PORT |=(1<<LCD_EN); //en = 1
	_delay_us(50); // Recommendation to wait for 3 us, here will wait upto 50us
	LCD_PORT &= ~(1<<LCD_EN); //en = 0
	_delay_us(50);
	return;
}
static void LCD_Send4Bit(char Data) // send 4 bit to LCD
{
	LCD_PORT &= 0x0F;
	LCD_PORT |= (Data<<4);
	LCD_Enable();
	return;
}
void LCD_SendCommand(char Command) // send 8bit , 4 high, 4 low
{
	LCD_Send4Bit(Command >> 4); // send 4 bit high
	LCD_Send4Bit(Command); // 4bit low
	_delay_ms(10); // delay 10 ms _ for lcd stable
	return;
}
static void LCD_Clear(void) // clear screen
{	// can delay add 2ms
	// LCD command delay 10ms
	LCD_SendCommand(0x01);
	return;
}
void LCD_Gotoxy(char x, char y) // move cursor to column x, row y
{
	unsigned char address;
	if(!y) address=(0x80+x);
	else address= (0xc0+x);
	LCD_SendCommand(address);
	return;
}
void LCD_PutChar(char Data) // display a char
{
	LCD_PORT |= (1<<LCD_RS); //rs = 0 execute mode, 1 output data mode
	LCD_SendCommand(Data);
	LCD_PORT &= ~(1<<LCD_RS);
	return;
}
void LCD_Puts(char* s) // display string
{
	while(*s) // read string until /0
	{
		LCD_PutChar(*s);
		s++;
	}
	return;
}

void LCD_WriteSymbol(char number,char symbol[8])
{
	number*=8; // shift 8*number bit on CGRAM
	LCD_SendCommand(0x40|number); // move to address on CGRAM
	LCD_PORT |= (1<<LCD_RS);      // change to data mode
	for(int i = 0;i<8;i++)
	{
		LCD_SendCommand(symbol[i]);
	}
	LCD_PORT &=~ (1<<LCD_RS); // change to execute mode
	return;
}