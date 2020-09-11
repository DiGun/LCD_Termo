#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
// #define F_CPU 16000000UL
// #define F_CPU  8000000UL
#include <util/delay.h>
#include <avr/eeprom.h>
#include <string.h>
#include "lcdpcf8574.h"
#include "music.h"



/* mode
0	-	off/end
1	-	on heater and wait for needed temp
2	-	hold temp needed time
3	-	off heater and wait for time
4	-	off heater and wait for temp
*/

#define  MODE_TYPE_MAX 4

typedef struct conf_st {
	uint8_t sound;
	uint8_t d_up;
	uint8_t d_down;
	uint8_t time;
} conf_t;

conf_t conf_cur;

typedef struct mode_work_st {
	uint8_t mode;
	uint8_t temp;
	uint16_t sec;
} mode_work_t;

#define MODE_PARAM_CNT 3


#define MODE_SEL_MAX 4
#define MODE_SEL_STEPS 4

mode_work_t EEMEM mode_work[MODE_SEL_MAX][MODE_SEL_STEPS];

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
uint8_t btn_press_long[BTN_CNT];
uint8_t btn_press_ev[BTN_CNT];
uint8_t btn_press[BTN_CNT];
uint8_t	btn_repeat_lr;
uint8_t	btn_repeat_ud;

#define btn_left	0
#define btn_right	1
#define btn_up		2
#define btn_down	3


#define BTN_LONG_PRES_WAIT 40
#define BTN_LONG_PRES_REPEAT 4

void btn_check(void)
{
	uint8_t btn[BTN_CNT];
	btn[0]=BTN_IN&BTN_1;
	btn[1]=BTN_IN&BTN_2;
	btn[2]=BTN_IN&BTN_3;
	btn[3]=BTN_IN&BTN_4;
	
	for (uint8_t f=0; f<BTN_CNT;f++)
	{
		if (btn[f]==0)
		{
//			btn_last_state[f]|=1;
			btn_last_state[f]=0b00000111;
		}
		else
		{
			btn_last_state[f]>>=1;
//			btn_last_state[f]&=~1;
			btn_last_state[f]&=~(0b10000000);
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
			if (btn_last_state[f]>=0b00000111)
			{
				if (btn_press[f]==0)
				{
					btn_press_ev[f]=1;
					btn_press[f]=1;
					btn_press_long[f]=0;
				}
				else
				{
					if (((btn_repeat_lr!=0)&&((f==btn_left)||(f==btn_right)))||((btn_repeat_ud!=0)&&((f==btn_up)||(f==btn_down))))
					{
						btn_press_long[f]++;
						if (btn_press_long[f]>=BTN_LONG_PRES_WAIT)
						{
							btn_press_long[f]=BTN_LONG_PRES_WAIT-BTN_LONG_PRES_REPEAT;
							btn_press_ev[f]=1;
						}
					}
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
uint8_t menu_mode_select_step;
uint8_t menu_mode_param;
uint8_t menu_mode_conf;

#define MENU_MODE_PARAM_EDIT 3
#define MENU_MODE_PARAM 2
#define MENU_MODE_STEP 4
#define MENU_MODE_SEL 1
#define MENU_MODE_CONF	5
#define MENU_MODE_CONF_EDIT	6

#define MENU_MODE_STEP_MAX	MODE_SEL_STEPS
#define MENU_MODE_SEL_MAX	MODE_SEL_MAX
#define MENU_MODE_SEL_MAXS	(MENU_MODE_SEL_MAX+1)

#define MODE_CONF_MAX 4


void eep_load(void)
{
	eeprom_read_block(&mode_work_cur, &mode_work[menu_mode_select-1][menu_mode_select_step-1], sizeof(mode_work_t));
}

void eep_save(void)
{
	eeprom_update_block(&mode_work_cur, &mode_work[menu_mode_select-1][menu_mode_select_step-1], sizeof(mode_work_t));
}

void print_blank(uint8_t c)
{
	for(uint8_t f=0;f<c;f++)
	{
		lcd_putc(' ');
	}
}

void print_param(uint8_t n)
{
	char buff[5];
	itoa(n,buff,10);
	lcd_puts(buff);
	print_blank(5-strlen(buff));
}

void edit_mode_param(int8_t dir)
{
	lcd_gotoxy(5, 1);
	switch(menu_mode_param)
	{
		case 1:
		mode_work_cur.mode+=dir;
		if (mode_work_cur.mode==255)
		{
			mode_work_cur.mode=MODE_TYPE_MAX;
		}
		if (mode_work_cur.mode>MODE_TYPE_MAX)
		{
			mode_work_cur.mode=0;
		}
		print_param(mode_work_cur.mode);
		break;
		case 2:
		mode_work_cur.temp+=dir;
		print_param(mode_work_cur.temp);
		break;
		case 3:
		{
			uint16_t minut;
			char buff[4];
			minut=mode_work_cur.sec/60;
			minut+=dir;
			if (minut>1024)
			{
			minut=1024;
			}
			if (minut==0)
			{
				minut=1;
			}
			uint8_t l;
			uint8_t h;
			uint8_t m;
			h=minut/60;
			m=minut-h*60;
			itoa(h,buff,10);
			lcd_puts(buff);
			l=strlen(buff);
			lcd_putc(':');
			l++;
			itoa(m,buff,10);
			if (strlen(buff)==1)
			{
				lcd_putc('0');
			}
			lcd_puts(buff);
			l++;
			l++;
			print_blank(5-l);
			mode_work_cur.sec=minut*60;
		}
		break;
	}
}

void edit_conf_param(int8_t dir)
{
	lcd_gotoxy(7, 1);
	switch (menu_mode_conf)
	{
		case 1:
			conf_cur.sound+=dir;
			if (conf_cur.sound>1)
			{
				conf_cur.sound=0;
			}
			else
			{
				if (conf_cur.sound==255)
				{
					conf_cur.sound=1;
				}
			}
			if (conf_cur.sound!=0)
			{
				lcd_puts_P("On");
			}
			else
			{
				lcd_puts_P("Off");
			}
		break;
		case 2:
			conf_cur.time+=dir;
			print_param(conf_cur.time);
		break;
		case 3:
			conf_cur.d_up+=dir;
			print_param(conf_cur.d_up);
		break;
		case 4:
			conf_cur.d_down+=dir;
			print_param(conf_cur.d_down);
		break;
	}
}


uint8_t sh_menu;
#define SHOW_NONE 0
#define SHOW_MENU_MODE 1
#define SHOW_MENU_MODE_SELECT 2
#define SHOW_MENU_MODE_STEP 3
#define SHOW_MENU_MODE_PARAM 4
#define SHOW_MENU_MODE_PARAM_EDIT 5
#define SHOW_MENU_MODE_PARAM_EDIT_VALUE 6
#define SHOW_MENU_CONF 7
#define SHOW_MENU_CONF_EDIT	8

void clear_screen(void)
{
	lcd_clrscr();
	lcd_gotoxy(0, 0);	
}


void menu(void)
{
	if (sh_menu!=SHOW_NONE)
	{
		switch (sh_menu)
		{
			case SHOW_MENU_MODE:
				sh_menu=SHOW_MENU_MODE_SELECT;
				lcd_gotoxy(0, 0);
				lcd_puts_P("Mode:");
				lcd_gotoxy(0, 1);
				lcd_puts_P("Select");
				print_blank(4);
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
			case SHOW_MENU_MODE_STEP:
				sh_menu=SHOW_NONE;
				clear_screen();
				lcd_putc('M');
				lcd_putc(':');
				lcd_putc('0'+menu_mode_select);
				lcd_putc(' ');
				lcd_puts_P("Step:");
				lcd_putc('0'+menu_mode_select_step);
			break;

			case SHOW_MENU_MODE_PARAM:
				sh_menu=SHOW_NONE;
				lcd_gotoxy(0, 1);
				eep_load();			
				switch(menu_mode_param)
				{
					case 1:
						lcd_puts_P("Type:");
					break;
					case 2:
						lcd_puts_P("Temp:");
					break;
					case 3:
						lcd_puts_P("Time:");
					break;
				}
				edit_mode_param(0);
			break;
			case SHOW_MENU_MODE_PARAM_EDIT:
				sh_menu=SHOW_NONE;
				lcd_gotoxy(4, 1);
				lcd_putc('>');
			break;
			case SHOW_MENU_CONF:
				sh_menu=SHOW_NONE;
				lcd_gotoxy(0, 1);
				switch (menu_mode_conf)
				{
					case 1:
						lcd_puts_P("Sound :");
					break;
					case 2:
						lcd_puts_P("Time-+:");
					break;
					case 3:
						lcd_puts_P("Delta+:");
					break;
					case 4:
						lcd_puts_P("Delta-:");
					break;
				}
				edit_conf_param(0);
			break;
			case SHOW_MENU_CONF_EDIT:
				sh_menu=SHOW_NONE;
				lcd_gotoxy(6, 1);
				lcd_putc('>');
			break;		
		}
	}
}


void btn_event_release(void)
{
		if (btn_press_ev[btn_left]!=0)
		{
			// left
			btn_press_ev[btn_left]=0;
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
				case MENU_MODE_STEP:
					sh_menu=SHOW_MENU_MODE_STEP;
					menu_mode_select_step--;
					if (menu_mode_select_step==0)
					{
						menu_mode_select_step=MENU_MODE_STEP_MAX;
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
				case MENU_MODE_CONF:
					sh_menu=SHOW_MENU_CONF;
					menu_mode_conf--;
					if (menu_mode_conf==0)
					{
						menu_mode_conf=MODE_CONF_MAX;
					}
				break;
				
			}
		}
		if (btn_press_ev[btn_right]!=0)
		{
			// right
			btn_press_ev[btn_right]=0;
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
				case MENU_MODE_STEP:
					sh_menu=SHOW_MENU_MODE_STEP;
					menu_mode_select_step++;
					if (menu_mode_select_step>(MENU_MODE_STEP_MAX))
					{
						menu_mode_select_step=1;
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
				case MENU_MODE_CONF:
					sh_menu=SHOW_MENU_CONF;
					menu_mode_conf++;
					if (menu_mode_conf>(MODE_CONF_MAX))
					{
						menu_mode_conf=1;
					}
				break;
			}
		}
		if (btn_press_ev[btn_up]!=0)
		{
			// up
			btn_press_ev[btn_up]=0;
			switch(menu_mode)
			{
				case MENU_MODE_STEP:
				case MENU_MODE_CONF:
					sh_menu=SHOW_MENU_MODE;
					menu_mode=MENU_MODE_SEL;
				break;
				case MENU_MODE_PARAM:
					sh_menu=SHOW_MENU_MODE_STEP;
					menu_mode=MENU_MODE_STEP;
				break;
				case MENU_MODE_PARAM_EDIT:
					sh_menu=SHOW_MENU_MODE_PARAM;
					menu_mode=MENU_MODE_PARAM;
					eep_save();
					btn_repeat_lr=0;
				break;
				case MENU_MODE_CONF_EDIT:
					sh_menu=SHOW_MENU_CONF;
					menu_mode=MENU_MODE_CONF;
					eep_save();
					btn_repeat_lr=0;
				break;
				
			}
		}
		if (btn_press_ev[btn_down]!=0)
		{
			// down
			btn_press_ev[btn_down]=0;
			switch(menu_mode)
			{
				case MENU_MODE_SEL:
					if (menu_mode_select==MENU_MODE_SEL_MAXS)
					{
						sh_menu=SHOW_MENU_CONF;
						menu_mode=MENU_MODE_CONF;
						menu_mode_conf=1;
					}
					else
					{
						sh_menu=SHOW_MENU_MODE_STEP;
						menu_mode=MENU_MODE_STEP;
						menu_mode_select_step=1;
					}
				break;
				case MENU_MODE_STEP:
					sh_menu=SHOW_MENU_MODE_PARAM;
					menu_mode=MENU_MODE_PARAM;
					menu_mode_param=1;
				break;
				case MENU_MODE_PARAM:
					btn_repeat_lr=1;
					sh_menu=SHOW_MENU_MODE_PARAM_EDIT;
					menu_mode=MENU_MODE_PARAM_EDIT;
				break;
				case MENU_MODE_CONF:
					btn_repeat_lr=1;
					sh_menu=SHOW_MENU_CONF_EDIT;
					menu_mode=MENU_MODE_CONF_EDIT;
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
	btn_repeat_lr=0;
	btn_repeat_ud=0;
	sh_menu=SHOW_MENU_MODE;
	menu_mode=MENU_MODE_SEL;
	menu_mode_select=1;
	menu_mode_param=1;
	sei();
	
	//init lcd
	lcd_init(LCD_DISP_ON);

	//lcd go home
	lcd_home();
/*
	char s[5];
	
	mode_work_cur.mode=3;
	mode_work_cur.temp=5;
	mode_work_cur.sec=44;
	itoa(mode_work_cur.mode, s, 10);
	lcd_puts(s);
	lcd_putc(' ');
	itoa(mode_work_cur.temp, s, 10);
	lcd_puts(s);
	lcd_putc(' ');
	itoa(mode_work_cur.sec, s, 10);
	lcd_puts(s);
	eep_save();
	eep_load();
	lcd_gotoxy(0, 1);
	itoa(mode_work_cur.mode, s, 10);
	lcd_puts(s);
	lcd_putc(' ');
	itoa(mode_work_cur.temp, s, 10);
	lcd_puts(s);
	lcd_putc(' ');
	itoa(mode_work_cur.sec, s, 10);
	lcd_puts(s);

	while (1);
*/	
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
	}
}


