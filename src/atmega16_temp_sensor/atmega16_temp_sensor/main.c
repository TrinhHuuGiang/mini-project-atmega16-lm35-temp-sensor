/*
 * atmega16_temp_sensor.c
 *
 * Created: 17-Apr-24 3:34:29 PM
 *  Author: GiangTrinh
 */ 


// fuse bit can luu y
// fusebit external clock
// fusebit slow startup

// Trong ung dung nay cac PORT se su sung cho cac chuc nang khac nhau
// PORTA: ADC
// PORTB: Cap nguon cho cam bien biet do va opamp
// PORTC: LCD
// PORTD: Button
// cac bit chon clock, sleep can fuse dung cach
#define F_CPU 8000000UL
// tan so cho cac ham delay
#include <avr/io.h>
// thu vien chuan avr
#include <avr/interrupt.h>
// VD: ham ngat tong sei()
#include <avr/sleep.h>
// thu vien sleep
#include <util/delay.h>
// thu vien delay
#include <stdlib.h>
// VD: dtostrf chuyen float sang char
#include "LCD_1602.h"
// Ham giao tiep LCD 4bit
#include "Symbol.h"
// Cac bieu tuong thiet ke cho CGRAM

#define degree_symbol 0xdf
#define LedLCD 3

/*_______________________Khai bao bien________________________________*/
// tinh toan va luu nhiet do
unsigned int level = 0;				//luu gia tri ADC doc duoc
float celsius;						//luu gia tri nhiet do
char Temperature[6];				//luu chuoi gia tri chuyen doi
char _unit_ = 0;					//don vi: 0 1 2 = C K F

// luu gia tri ngat ngoai
char i0 = 0;						//i0 = 1 -> ngat 0 -> Chage Menu
char i1 = 0;						//i1 = 1 -> ngat 1 -> Select
char stm = 0;						//trang thai menu 0 1 2 -> sleep, don vi, light

// luu gia tri ngat timer0
char c0 = 0;						//dem 500ms
char ent = 0;						//cho phep toggle Symbol
char stt = 0;						//trang thai toggle _ co 2 trang thai cho moi symbol

char c1 = 0;						//dem 1s
char enc= 0;						//cho phep kiem tra nhiet do


/*_______________________Ngat ngoai, timer___________________________*/
// ham ngat ngoài, ngat timer
ISR(INT0_vect) // khong goi ham vi gay loi chuong trinh
{
	i0 = 1;
}

ISR(INT1_vect)
{
	i1 = 1;
}

ISR(TIMER0_OVF_vect)
{
	// 8MHz -> su dung bo chia 1024-> delay 500ms -> 15 lan dem tu 0 den 256
	c0 ++;								// dem den 15
	if(c0 > 15)							// 500ms
	{
		c0 = 0;							// reset count
		ent = 1;						// cho phep toggle
		stt = 1-stt;					// trang thai toggle
	}
	c1 ++;								// dem den 30
	if(c1 >30)
	{
		c1 = 0;							// reset count
		enc = 1;						// cho phep do nhiet do
	}
}

// khoi tao ngat, timer
void INT_Init()
{
	// cau hinh ngat ngoai cho int0 và int1 va khong dung int2
	GICR |= ((1<<INT0)|(1<<INT1));			// bat ngat
	MCUCR |= 0x08;
	//00001000 -> ngat muc thap cho int0, ngat suon xuong cho int1
	//ngat muc thap vi dung sleep là powerdown chi khoi dong voi mode nay

	// cau hinh timer0 de toggle symbol
	TCCR0 |= 0x05;							// chia 1024
	TCNT0 = 0x00;							// dem tu 0 den 0xFF
	TIMSK |=(1<<TOIE0); // kich hoat ngat timer khi co dem tran
	
	// cau hình ngat tong
	sei();
	
	return;
}

/*_______________________Bo ADC________________________________________*/
// khoi tao ADC o port A va kenh 0
void ADC_Init(void)
{
	DDRA = 0x00; // ADC port is input
	PORTA |= 0x00; // khong kich hoat pull up port A
	ADCSRA = 0x87; // cai dat tan so adc la  freq / 128 de < 20khz
	ADMUX = 0xC0; //vref = 2.56v, adc channel: 0
	return;
}

// chon kenh de doc
int ADC_Read(char channel)
{
	ADMUX = 0xC0|(channel&0x07); //vref = 2.56v, kenh duoc chon & 0x07
	ADCSRA|= (1<<ADSC); // start adc conversion
	while(!(ADCSRA&(1<<ADIF))); // wait until end of coversion by polling
	ADCSRA |= (1<<ADIF); // clear interrupt flag
	_delay_ms(1); // wait a little
	return ADCW; // return adc word
}

/*_______________________Them thu vien cho LCD___________________________*/
void Setup_Symbol()
{
	//khoi tao LCD
	LCD_Init();

	//nap ky tu cho LCD
	LCD_WriteSymbol(0,Blue0);	//man hinh 1
	LCD_WriteSymbol(1,Blue1);	//man hinh 2
	LCD_WriteSymbol(2,Blue2);	//hot 1
	LCD_WriteSymbol(3,Blue3);	//hot 2
	LCD_WriteSymbol(4,Blue4);	//cold 1
	LCD_WriteSymbol(5,Blue5);	//cold 2
	LCD_WriteSymbol(6,Blue6);	//exit 1
	LCD_WriteSymbol(7,Blue7);	//exit 2
}

/*_______________________SET_UP pin va cac thanh ghi___________________________*/
void Setup_Pinmode(void)
{
	// cai dat pin cho cam bien va opamp
	DDRB |=0x07;						// kich hoat output 3 chan 0 1 2 cong B lam nguon nuoi lm35, icl7660, op07
	PORTB |=0x07;						// kich hoat muc logic high

	// cau hinh ADC voi vref noi
	ADC_Init();

	// cau hinh LCD mac dinh port C
	DDRC = 0xFF;						// port C output ra lcd
	PORTC |=(1<<LedLCD);				// set bit 3 high de bat den man hinh
	Setup_Symbol();


	// cau hinh Button ngat; pud = 0 & PORTxn = 1 -> pullup
	// o day su dung 2 chan btn deu là ngat ngoai
	DDRD = 0x00;						// portD is input
	SFIOR &=~(1<<2);					// bit pud = 0 -> bat pull up
	PORTD |= ((1<<2)|(1<<3));			// pullup bit 2,3 = 1 ung voi 2 chan int0 va 1

	// cau hinh ngat ngoai, timer
	INT_Init();
}

/*_______________________Cac ham chuc nang__________________________________*/
// cac lua chon
void Setup_Menu(void)
{
	++stm;
	switch(stm)
	{
		case 3: // option 1 sleep
		stm = 0;
		LCD_Gotoxy(3,1);
		LCD_Puts("   Exit   ");
		break;
		case 1: // option 2 convert
		LCD_Gotoxy(3,1);
		LCD_PutChar(degree_symbol);
		LCD_Puts("C > K >");
		LCD_PutChar(degree_symbol);
		LCD_Puts("F");
		break;
		case 2: // option 3 light
		LCD_Gotoxy(3,1);
		LCD_Puts("   Light  ");
		break;
		default:
		break;
	}
	return;
}

//lua chon
void Select_option(void)
{
	switch(stm)
	{
		case 0: // sleep
		// ngat nguon cap cho thiet bi khac
		PORTB &=~0x07;// tat nguon lm35, icl, opamp
		PORTC &=~(1<<LedLCD); // tat led lcd
		LCD_SendCommand(0x08); // tat man hinh
		
		// thu tuc sleep , su dung powerdown mode
		sleep_enable();
		set_sleep_mode(SLEEP_MODE_PWR_DOWN); // power down
		sleep_cpu();
		sleep_disable(); // set flag sleep mode off
		
		// bat nguon cap
		PORTB |=0x07;
		PORTC |=(1<<LedLCD); // bat led lcd
		LCD_SendCommand(0x0c); // bat hien thi va tat con tro
		
		break;
		case 1: // c,k,f
		_unit_++;
		if(_unit_==3)
		_unit_=0;
		break;
		case 2: // led lcd
		PORTC ^= (1<<LedLCD); // toggle led
		break;
		default:
		break;
	}
	return;
}

//animation
void Puts_a_Symbol(char x, char y, char command) // hang y cot x
{
	LCD_Gotoxy(x,y);
	LCD_PutChar(command);
	return;
}
void Toggle_Symbol(void)
{
	// toggle symbol hang 1
	if(level)				// neu level != 0
	{
		if(level < 150)
		{
			if(stt)
			{
				Puts_a_Symbol(3,0,4);
				Puts_a_Symbol(12,0,4);
			}
			else
			{
				Puts_a_Symbol(3,0,5);
				Puts_a_Symbol(12,0,5);
			}
		}
		else if (level > 400)
		{
			if(stt)
			{
				Puts_a_Symbol(3,0,2);
				Puts_a_Symbol(12,0,2);
			}
			else
			{
				Puts_a_Symbol(3,0,3);
				Puts_a_Symbol(12,0,3);
			}
		}
		else
		{
			Puts_a_Symbol(3,0,0xFF);
			Puts_a_Symbol(12,0,0xFF);
		}
		
	}
	else
	{
		if(stt)
		{
			Puts_a_Symbol(3,0,0x21);
			Puts_a_Symbol(12,0,0x21);
		}
		else
		{
			Puts_a_Symbol(3,0,0x20);
			Puts_a_Symbol(12,0,0x20);
		}
	}
	
	// toggle symbol hang 2
	switch(stm)
	{
		case 0: // sleep
		if(stt)
		{
			Puts_a_Symbol(3,1,6);
			Puts_a_Symbol(12,1,6);
		}
		else
		{
			Puts_a_Symbol(3,1,7);
			Puts_a_Symbol(12,1,7);
		}
		break;
		case 1: // convert
		break;
		case 2: // light
		if(PORTC & (1<<LedLCD)) // neu khac 0 thi dang bat man hinh
		{
			Puts_a_Symbol(3,1,1);
			Puts_a_Symbol(12,1,1);
		}
		else // tat man hinh
		{
			Puts_a_Symbol(3,1,0);
			Puts_a_Symbol(12,1,0);
		}
		break;
		default:
		break;
	}
	return;
}

/*__________________________Do luong________________________*/
void Tinhnhietdo(void)
{
	// doc kenh adc 0
	// step 0.1 C = 1mV -> khuyech dai == 25mV == 1 step
	//℃ * 1.8000 + 32.00 = độ F
	//℃ + 273.15 = độ K
	level = ADC_Read(0);			// luu lai gia tri de khong can doc nhieu lan
	switch(_unit_)
	{
		case 0: //C
		celsius = level*0.1;
		break;
		case 1: //F
		celsius = level*0.1 + 273.15;
		break;
		case 2: //K
		celsius = level*0.18 + 32;
		break;
		default:
		break;
	}
	return;
}

void CheckConnect(void)
{
	if(level)
	{
		dtostrf(celsius,6,2,Temperature);
		LCD_Gotoxy(4,0);
		LCD_Puts(Temperature);
	}
	else
	{
		LCD_Gotoxy(4,0);
		LCD_Puts("   NaN");
	}
	return;
}

void ThemDonvi(void)
{
	switch(_unit_)
	{
		case 0: //C
		LCD_PutChar(degree_symbol);
		LCD_Puts("C");
		break;
		case 1: //K
		LCD_Puts(" K");
		break;
		case 2: //F
		LCD_PutChar(degree_symbol);
		LCD_Puts("F");
		break;
		default:
		break;
	}
	return;
}
/*____________________________MAIN_____________________________________*/

int main(void)
{
	Setup_Pinmode();
	
	// display load lcd
	LCD_Gotoxy(0,0);
	for(int i = 0;i<16;i++)
	{
		LCD_PutChar(0xFF);
		_delay_ms(30);
	}
	
	LCD_Gotoxy(0,1);
	LCD_Puts("<     Exit     >");
	LCD_Gotoxy(3,1);
	LCD_PutChar(6);
	LCD_Gotoxy(12,1);
	LCD_PutChar(6);
	
	while (1)
	{
		// Kiem tra ngat ngoai 0 _ Menu
		if(i0)
		{
			i0 = 0;
			Setup_Menu();
		}
		
		// Kiem tra ngat ngoai 1 _ Select
		if(i1)
		{
			i1 = 0;
			Select_option();
		}
		
		// Kiem tra timer0 dem 500ms _ Animation
		if(ent)
		{
			ent = 0;
			Toggle_Symbol();
		}
		
		// Kiêm tra timer0 dem 1s_ tinh nhiet do
		if(enc)
		{
			enc = 0;
			Tinhnhietdo();
			CheckConnect();
			ThemDonvi();
		}
		_delay_ms(200);// chong doi phim bam
	}
}


