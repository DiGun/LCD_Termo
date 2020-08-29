#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
// #define F_CPU 16000000UL
#include <util/delay.h>

#include "lcdpcf8574.h"
#include "music.h"



// #define PIN(x) (*(&x - 2))    /* address of input register of port x */


const char PROGMEM char_down_p[8]  ={
									0b00100,
									0b00100,
									0b00100,
									0b00100,
									0b00100,
									0b10101,
									0b01110,
									0b00100
								};
const char PROGMEM char_up_p[8]  ={
	0b00100,
	0b01110,
	0b10101,
	0b00100,
	0b00100,
	0b00100,
	0b00100,
	0b00100
};


#define MEL2	11
/*
note_t melod2[MEL2] =
{
	{ TONE(A1),O8 },
	{ TONE(C2),O8 },
	{ TONE(E2),O8 },
	{ TONE(C2),O8 },
	{ TONE(D2),O4 },
	{ TONE(C2),O8 },
	{ TONE(B1),O8 },
	{ TONE(E2),O4 },
	{ TONE(D2),O4 },
	{ TONE(A1),O4 },
	{ 0,O4 }
};
*/

const note_t  melod2_p[MEL2] PROGMEM=
{
	{ TONE(A1),O8 },
	{ TONE(C2),O8 },
	{ TONE(E2),O8 },
	{ TONE(C2),O8 },
	{ TONE(D2),O4 },
	{ TONE(C2),O8 },
	{ TONE(B1),O8 },
	{ TONE(E2),O4 },
	{ TONE(D2),O4 },
	{ TONE(A1),O4 },
	{ 0,O4 }
};


void LoadCustomChar(uint8_t idx,const char fl_zn[])
{
	uint8_t a[8];
	for (uint8_t n=0;n<8;n++)
	{
		a[n]=pgm_read_byte(&fl_zn[n]);
	}
	lcd_create_custom_char (idx, a);
}

volatile uint8_t ticks;
uint16_t seconds;

// Timer settings  30.517578125 Hz (F_CPU=80000000)
inline void  tick(void)
{
		ticks++;
		if (ticks==30)
		{
			ticks=0;
		}
}


uint8_t play_melody;

void play_check()
{
	switch (play_melody)
	{
		case 0:
		break;
		case 1:
//		mus_play(melod2, MEL2,0);
		mus_play_p(melod2_p, MEL2,0);
		if (mus_play_stop)
		{
			play_melody=0;
		}
		break;
	}
/*
		case 2:
		play(melod2, MEL2,0);
		if (play_stop)
		{
			play_mel=0;
		}
		break;
		case 5:
		play(melod5, MEL5,0);
		if (play_stop)
		{
			play_mel=0;
		}
		break;
		case 6:
		play(melod, melod_note_cnt,0);
		if (play_stop)
		{
			if (melod_note_cnt!=0)
			{
				play(melod, melod_note_cnt,1);
			}
			else
			{
				//					LED_OUT|=LED1;
				play_mel=0;
			}

		}
		break;
		case 3:
		play(melod3, MEL3,0);
		if (play_stop)
		{
			if (btn_press[5]==0)
			{
				play(melod3, MEL3,1);
			}
			else
			{
				LED_OUT|=LED2;
				play_mel=0;
			}
		}
		break;
		case 4:
		play(melod4, MEL4,0);
		if (play_stop)
		{
			if (alert)
			{
				play(melod4, MEL4,1);
			}
			else
			{
				//					LED_OUT|=LED1;
				play_mel=0;
			}
		}
		break;
	}
	*/
}

void quick_fn(void)
{
  static uint8_t last_tick=255;
  if (last_tick!=ticks)
  {
	last_tick=ticks;
	if (last_tick==0)
	{
		seconds++;
	}
	play_check();
  }
}


void pause(uint8_t s)
{
	static uint16_t sec_start=0;
	ticks=1;
	sec_start=seconds+s;
	while (sec_start>seconds)
	{
		quick_fn();
	}

}
/*
void test(const note_t* ntp)
{
		uint8_t i;
		note_t nt;
		note_t *ntc;
		lcd_clrscr();
		lcd_home();
		for(i=0; i<MEL2; i++)
		{
			char buf[10];
			ntc=&melod2[i];
			
			itoa(ntc->tone, buf, 10);
			lcd_gotoxy(0, 0);
			lcd_puts(buf);

			itoa(ntc->leng, buf, 10);
			lcd_gotoxy(8, 0);
			lcd_puts(buf);
			
			memcpy_P(&nt,&ntp[i],sizeof(nt));
			
			itoa(nt.tone, buf, 10);
			lcd_gotoxy(0, 1);
			lcd_puts(buf);

			itoa(nt.leng, buf, 10);
			lcd_gotoxy(8, 1);
			lcd_puts(buf);
			
			
			
			_delay_ms(2000);
			lcd_clrscr();

		}

	
}
*/

int main(void)
{
	//init switch
	DDRB |= (1<<PORTB0);
	
	PORTB &= !(1<<PORTB0);
	//enable internal pull-up button up
	play_melody=0;
	seconds=0;
	mus_init();
	
	sei();
	
	//init lcd
	lcd_init(LCD_DISP_ON);

	//lcd go home
	lcd_home();

	uint8_t led = 0;
	lcd_led(led); //set led
	lcd_puts_P("metrolog.org.ua");
	LoadCustomChar(0,char_down_p);
	LoadCustomChar(1,char_up_p);
	lcd_gotoxy(0, 1);
	lcd_putc(0);
	lcd_putc(1);
/*	
	_delay_ms(1000);
	while (1)
	{
		test(melod2_p);
	}
*/	
/*
		int i = 0;
		//		int line = 0;
		lcd_gotoxy(10, 1);
		lcd_puts("i= ");
		for(i=0; i<=99; i++) 
		{
			char buf[10];
			itoa(i, buf, 10);
			if (buf[1]==0)
			{
				buf[1]=' ';
				buf[2]=0;
			}
			//			itoa(i, buf, 10);
			//			lcd_gotoxy(4, line);
			lcd_gotoxy(13, 1);
			lcd_puts(buf);
			//			line++;
			//			line %= 2;
//			_delay_ms(10);
		}
*/		
	uint8_t f=0;
	mus_play_p(melod2_p, MEL2,1);	
	play_melody=1;

	while(1) {

		pause(4);
//		_delay_ms(4000);
		
		lcd_clrscr();
		if (f==16)
		{
			f=0;
		}
		if (f<10)
		{
			lcd_putc('0'+f);
		} 
		else
		{
			lcd_putc('A'+f-10);
		}
		pause(2);
		
//		_delay_ms(2000);		
		lcd_home();
		for (char c='0';c<='9';c++)
		{
			lcd_putc(c);
		}
		for (char c='A';c<='F';c++)
		{
			lcd_putc(c);
		}
		lcd_gotoxy(0, 1);
		for (char c=0;c<=15;c++)
		{
			lcd_putc(c+f*16);
		}
		
//		lcd_led(led); //set led
//		led = !led; //invert led for next loop
		f++;
		//test loop
	}
}

