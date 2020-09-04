#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
// #define F_CPU 16000000UL
// #define F_CPU  8000000UL
#include <util/delay.h>
#include <avr/eeprom.h>

#include "lcdpcf8574.h"
#include "music.h"



/* mode
0	-	off
1	-	on heater and wait for needed temp
2	-	hold temp needed time
3	-	off heater and wait for time
4	-	off heater and wait for temp
*/

#define  MODE_TYPE_CNT 5

typedef struct mode_work_st {
	uint8_t mode;
	uint8_t temp;
	uint16_t sec;
} mode_work_t;

#define MODE_PARAM_CNT 3


#define MODE_SEL_MAX 4

mode_work_t EEMEM mode_work[MODE_SEL_MAX];

mode_work_t mode_work_cur;




// #define PIN(x) (*(&x - 2))    /* address of input register of port x */

//*** Buttons

#define BTN_1			(1<<PORTD0)
#define BTN_2			(1<<PORTD1)
#define BTN_3			(1<<PORTD2)
#define BTN_4			(1<<PORTD3)
#define BTN_DDR			DDRD
#define BTN_IN			PIND
#define BTN_OUT			PORTD


void btn_init(void)
{
	BTN_DDR&= ~(BTN_1|BTN_2|BTN_3|BTN_4);
//	BTN_OUT&= ~(BTN_1|BTN_2|BTN_3|BTN_4);

	BTN_OUT|= (BTN_1|BTN_2|BTN_3|BTN_4);
}

#define BTN_CNT 4

uint8_t btn_last_state[BTN_CNT];
uint8_t btn_press_ev[BTN_CNT];
uint8_t btn_press[BTN_CNT];

void btn_check(void)
{
	uint8_t btn[BTN_CNT];
	btn[0]=BTN_IN&BTN_1;
	btn[1]=BTN_IN&BTN_2;
	btn[2]=BTN_IN&BTN_3;
	btn[3]=BTN_IN&BTN_4;
	
	for (uint8_t f=0; f<BTN_CNT;f++)
	{
		btn_last_state[f]<<=1;
		if (btn[f]==0)
		{
			btn_last_state[f]|=1;
		}
		else
		{
			btn_last_state[f]&=~1;
		}
		if (btn_last_state[f]==0)
		{
//				if (btn_press[f]!=0)
				{
					btn_press[f]=0;
				}
		}
		else
		{
			if (btn_last_state[f]>=0b00011111)
			{
				if (btn_press[f]==0)
				{
					btn_press_ev[f]=1;
					btn_press[f]=1;
				}
			}
		}
	}
}


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
	btn_check();
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

void print_bin(uint8_t b)
{
     lcd_gotoxy(0, 1);
	 uint8_t f=8;
	 while(1)
	 {
		 f--;
		 if (((b>>f)&1)==0)
		 {
			 lcd_putc('0');
		 }
		 else
		 {
			 lcd_putc('1');
		 }
		 if (f==0)
		 {
			 return;
		 }
	 }
}


uint8_t menu_mode;
uint8_t menu_mode_select;
uint8_t menu_mode_param;

#define MENU_MODE_PARAM_EDIT 3
#define MENU_MODE_PARAM 2
#define MENU_MODE_SEL 1


#define MENU_MODE_SEL_MAX	MODE_SEL_MAX
#define MENU_MODE_SEL_MAXS	(MENU_MODE_SEL_MAX+1)

void print_blank(uint8_t c)
{
	for(uint8_t f=0;f<c;f++)
	{
		lcd_putc(' ');
	}
}


uint8_t sh_menu;
#define SHOW_NONE 0
#define SHOW_MENU_MODE 1
#define SHOW_MENU_MODE_SELECT 2
#define SHOW_MENU_MODE_PARAM 3
#define SHOW_MENU_MODE_PARAM_EDIT 4
#define SHOW_MENU_MODE_PARAM_EDIT_VALUE 5



void menu(void)
{
	switch (sh_menu)
	{
		case SHOW_MENU_MODE:
			sh_menu=SHOW_MENU_MODE_SELECT;
			lcd_gotoxy(0, 0);
			lcd_puts_P("Mode:");
			lcd_gotoxy(0, 1);
			lcd_puts_P("Select");
		break;
		case SHOW_MENU_MODE_SELECT:
			sh_menu=SHOW_NONE;
			lcd_gotoxy(5, 0);
			if (menu_mode_select==MENU_MODE_SEL_MAXS)
			{
				lcd_puts_P("settings");
			}
			else
			{
				lcd_putc('0'+menu_mode_select);
				print_blank(7);
			}
		break;
		case SHOW_MENU_MODE_PARAM:
			sh_menu=SHOW_NONE;
			lcd_gotoxy(0, 1);
			mode_work_cur.mode=11;
			mode_work_cur.temp=22;
			mode_work_cur.sec=33;
			char buff[4];
				switch(menu_mode_param)
				{
					case 1:
					lcd_puts_P("Type:");
					itoa(mode_work_cur.mode,buff,10);
					lcd_puts(buff);
					break;
					case 2:
					lcd_puts_P("Temp:");
					itoa(mode_work_cur.temp,buff,10);
					lcd_puts(buff);
					break;
					case 3:
					lcd_puts_P("Time:");
					itoa(mode_work_cur.sec,buff,10);
					lcd_puts(buff);
					break;
				}
		break;
		case SHOW_MENU_MODE_PARAM_EDIT:
			sh_menu=SHOW_NONE;
			lcd_gotoxy(4, 1);
			lcd_putc('>');
		break;

	}
		




/*	

	*/
}


void edit_mode_param(int8_t dir)
{
	lcd_gotoxy(5, 1);
	char buff[4];
	switch(menu_mode_param)
				{
					case 1:
					mode_work_cur.mode+=dir;
					if (mode_work_cur.mode==0)
					{
						mode_work_cur.mode=
					}
					itoa(mode_work_cur.mode,buff,10);
					lcd_puts(buff);
					break;
					case 2:
					mode_work_cur.temp+=dir;
					itoa(mode_work_cur.temp,buff,10);
					lcd_puts(buff);
					break;
					case 3:
					mode_work_cur.sec+=dir;
					itoa(mode_work_cur.sec,buff,10);
					lcd_puts(buff);
					break;
				}
}


void btn_event_release(void)
{
		if (btn_press_ev[0]!=0)
		{
			// left
			btn_press_ev[0]=0;
			switch(menu_mode)
			{
				case MENU_MODE_SEL:
				sh_menu=SHOW_MENU_MODE_SELECT;
				menu_mode_select--;
				if (menu_mode_select==0)
				{
					menu_mode_select=MENU_MODE_SEL_MAXS;
				}
				break;
				case MENU_MODE_PARAM:
				sh_menu=SHOW_MENU_MODE_PARAM;
				menu_mode_param--;
				if (menu_mode_param==0)
				{
					menu_mode_param=MODE_PARAM_CNT;
				}
				break;
				case MENU_MODE_PARAM_EDIT:
				sh_menu=SHOW_MENU_MODE_PARAM_EDIT;
				edit_mode_param(-1);
				break;
				
			}
		}
		if (btn_press_ev[1]!=0)
		{
			// right
			btn_press_ev[1]=0;
			switch(menu_mode)
			{
				case MENU_MODE_SEL:
				sh_menu=SHOW_MENU_MODE_SELECT;
				menu_mode_select++;
				if (menu_mode_select>(MENU_MODE_SEL_MAXS))
				{
					menu_mode_select=1;
				}
				break;
				case MENU_MODE_PARAM:
				sh_menu=SHOW_MENU_MODE_PARAM;
				menu_mode_param++;
				if (menu_mode_param>(MODE_PARAM_CNT))
				{
					menu_mode_param=1;
				}
				break;
				case MENU_MODE_PARAM_EDIT:
				sh_menu=SHOW_MENU_MODE_PARAM_EDIT;
				edit_mode_param(1);
				break;
			}
		}
		if (btn_press_ev[2]!=0)
		{
			// up
			btn_press_ev[2]=0;
			switch(menu_mode)
			{
				case MENU_MODE_PARAM:
					sh_menu=SHOW_MENU_MODE;
					menu_mode=MENU_MODE_SEL;
					break;
				case MENU_MODE_PARAM_EDIT:
					sh_menu=SHOW_MENU_MODE_PARAM;
					menu_mode=MENU_MODE_PARAM;
				break;

			}
		}
		if (btn_press_ev[3]!=0)
		{
			// down
			btn_press_ev[3]=0;
			switch(menu_mode)
			{
				case MENU_MODE_SEL:
					sh_menu=SHOW_MENU_MODE_PARAM;
					menu_mode=MENU_MODE_PARAM;
					menu_mode_param=1;
				break;
				case MENU_MODE_PARAM:
					sh_menu=SHOW_MENU_MODE_PARAM_EDIT;
					menu_mode=MENU_MODE_PARAM_EDIT;
				break;

			}

			
		}
	
}

int main(void)
{
	//init switch
	DDRB |= (1<<PORTB0);
	
	PORTB &= !(1<<PORTB0);
	//enable internal pull-up button up
	play_melody=0;
	seconds=0;
	mus_init();
	btn_init();
	sh_menu=SHOW_MENU_MODE;
	menu_mode=MENU_MODE_SEL;
	menu_mode_select=1;
	menu_mode_param=1;
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
	
	_delay_ms(1000);
/*
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
//	uint8_t f=255;
//	mus_play_p(melod2_p, MEL2,1);	
//	play_melody=1;


			lcd_clrscr();
			lcd_gotoxy(0, 0);
//			print_blank(16);

	while(1) 
	{
		quick_fn();
		btn_event_release();
		menu();
/*		
		if (btn_press_ev[0]!=0)
		{
			// left
			lcd_gotoxy(f, 0);
			lcd_putc(0x7F);
			print_bin(BTN_IN);
			btn_press_ev[0]=0;	
			f++;
		}
		if (btn_press_ev[1]!=0)
		{
			// right
			lcd_gotoxy(f, 0);
			lcd_putc(0x7E);
			print_bin(BTN_IN);
			btn_press_ev[1]=0;
			f++;
		}
		if (btn_press_ev[2]!=0)
		{
			// up
			lcd_gotoxy(f, 0);
			lcd_putc(0x01);
			print_bin(BTN_IN);
			btn_press_ev[2]=0;
			f++;
		}
		if (btn_press_ev[3]!=0)
		{
			// down
			lcd_gotoxy(f, 0);
			lcd_putc(0x00);
			print_bin(BTN_IN);
			btn_press_ev[3]=0;
			f++;
		}
		
*/		
/*
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
		*/
	}
}


