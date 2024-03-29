#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
// #define F_CPU 16000000UL
// #define F_CPU  8000000UL
#include <util/delay.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <string.h>
#include "conf_io.h"
#include "lcdpcf8574.h"
#include "music.h"


#define  MODE_TYPE_MAX 4

typedef struct conf_st {
	uint8_t sound;
	uint8_t d_up;
	uint8_t d_down;
	uint8_t time;
	int8_t c_30;
	int8_t c_120;
	int8_t heating_mode;
} conf_t;

conf_t conf_cur;
conf_t EEMEM conf_e;

typedef struct mode_work_st {
	uint8_t mode;
	uint8_t temp;
	uint16_t sec;
} mode_work_t;

#define TEMP_MAX_STEP 30
uint8_t termo[TEMP_MAX_STEP];
uint8_t termo_cnt;
uint8_t termo_cur;
uint8_t k1;



#define MODE_PARAM_CNT 3


#define MODE_SEL_MAX 4
#define MODE_SEL_STEPS 4

mode_work_t EEMEM mode_work[MODE_SEL_MAX][MODE_SEL_STEPS];

mode_work_t mode_work_cur;




// #define PIN(x) (*(&x - 2))    /* address of input register of port x */

//*** Buttons
#ifndef  BTN
#define BTN_1			(1<<PORTD0)
#define BTN_2			(1<<PORTD1)
#define BTN_3			(1<<PORTD2)
#define BTN_4			(1<<PORTD3)
#define BTN_DDR			DDRD
#define BTN_IN			PIND
#define BTN_OUT			PORTD
#endif // BTN


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
			btn_last_state[f]=0b00000111;
		}
		else
		{
			btn_last_state[f]>>=1;
			btn_last_state[f]&=~(0b10000000);
		}
		if (btn_last_state[f]==0)
		{
			btn_press[f]=0;
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

#define HTMODE_0_1	1
#define HTMODE_0_2	2
#define HTMODE_0_3	3
#define HTMODE_1_2	4
#define HTMODE_1_3	5
#define HTMODE_2_3	6

#define HTMODE_MAX	6

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


#define PLAY_INTRO_CNT	11
const note_t  m_intro_p[PLAY_INTRO_CNT] PROGMEM=
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

#define PLAY_COLD_CNT 6
const note_t  m_cold_p[PLAY_COLD_CNT] PROGMEM=
{
	{ TONE(A2),O8 },
	{ TONE(A2),O8 },
	
	{ TONE(G2),O8 },
	{ TONE(G2),O8 },
	{ TONE(A2),O4 },
	{ TONE(E2),O4 },
};


#define PLAY_LETO_CNT	67
const note_t m_leto_p[PLAY_LETO_CNT] PROGMEM =
{
	{ TONE(D1),O8 },
	{ TONE(D1),O8 },
	{ TONE(D1),O8 },
	{ TONE(D1),O8 },
	{ TONE(D1),O4 },
	{ TONE(G1),O4 },
	
	{ TONE(D1),O4 },
	{ TONE(A1),O4 },
	{ TONE(D1),O4 },
	{ TONE(A1),O4 },
	
	{ TONE(D1),O8 },
	{ TONE(D1),O8 },
	{ TONE(D1),O8 },
	{ TONE(D1),O8 },
	{ TONE(D1),O4 },
	{ TONE(B1),O4 },


	{ TONE(D1),O4 },
	{ TONE(C2),O4 },
	{ TONE(B1),O4 },
	{ TONE(C2),O4 },

	{ TONE(D2),O8 },
	{ TONE(D2),O8 },
	{ TONE(D2),O8 },
	{ TONE(D2),O8 },
	{ TONE(D2),O4 },
	{ TONE(B1),O4 },

	{ TONE(G1),O4 },
	{ TONE(B1),O4 },
	{ TONE(G1),O4 },
	{ TONE(B1),O4 },
	
	{ TONE(D1),O8 },
	{ TONE(D1),O8 },
	{ TONE(D1),O8 },
	{ TONE(D1),O8 },
	{ TONE(D1),O4 },
	{ TONE(A1),O4 },
	
	{ TONE(G1),O4 },
	{ 0,O2+O4 },
	
	{ TONE(D2),O2 },
	{ TONE(B1),O4 },
	{ TONE(B1),O4 },
	
	{ TONE(D2),O4 },
	{ TONE(D2),O4 },
	{ TONE(B1),O2 },
	
	{ TONE(D2),O4 },
	{ TONE(D2),O8 },
	{ TONE(D2),O8 },
	{ TONE(B1),O4 },
	{ TONE(B1),O4 },
	
	{ TONE(G1),O4 },
	{ TONE(G1),O4 },
	{ TONE(E1),O4 },
	{ TONE(D1),O4 },
	
	{ TONE(B1),O2 },
	{ TONE(F1d),O4 },
	{ TONE(F1d),O4 },

	{ TONE(B1),O4 },
	{ TONE(B1),O4 },
	{ TONE(F1d),O2 },

	{ TONE(G1d),O4 },
	{ TONE(G1d),O8 },
	{ TONE(G1d),O8 },
	{ TONE(G1d),O4 },
	{ TONE(G1d),O4 },
	
	
	{ TONE(A1),O2 },
	{ TONE(D1),O4 },
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
	static uint8_t cnh=0;
		cnh++;
		if (cnh==conf_cur.time)
		{
			cnh=0;
		}
		else
		{
			ticks++;
		}
		if (ticks==30)
		{
			ticks=0;
		}
}


uint8_t print_time(int8_t dir);

uint8_t play_melody;
note_t  sound_buff[3];
uint8_t sound_buff_cnt;

#define PLAY_NONE	0
#define PLAY_INTRO	1
#define PLAY_COLD	2
#define PLAY_TOUCH	3
#define PLAY_LETO	4

void play_check()
{
	switch (play_melody)
	{
		case PLAY_NONE:
		break;
		case PLAY_INTRO:
		mus_play_p(m_intro_p, PLAY_INTRO_CNT,0);
		if (mus_play_stop)
		{
			play_melody=PLAY_NONE;
		}
		break;
		case PLAY_COLD:
		mus_play_p(m_cold_p, PLAY_COLD_CNT,0);
		if (mus_play_stop)
		{
			play_melody=PLAY_NONE;
		}
		break;
		case PLAY_TOUCH:
		mus_play(sound_buff, sound_buff_cnt,0);
		if (mus_play_stop)
		{
			play_melody=PLAY_NONE;
		}
		case PLAY_LETO:
		mus_play_p(m_leto_p, PLAY_LETO_CNT,0);
		if (mus_play_stop)
		{
			play_melody=PLAY_NONE;
		}
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


// uint8_t tenn[3];
uint8_t tenn_on;
uint8_t tenn_cur;

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
#define MENU_MODE_START 7
#define MENU_MODE_FINISH	8

#define MENU_MODE_STEP_MAX	MODE_SEL_STEPS
#define MENU_MODE_SEL_MAX	MODE_SEL_MAX
#define MENU_MODE_SEL_MAXS	(MENU_MODE_SEL_MAX+1)

#define MODE_CONF_MAX 7



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
#define SHOW_MENU_FINISH	9


inline void spi_init(void)
{
   SPI_DDRX |= (1<<SPI_MOSI)|(1<<SPI_SCK)|(1<<SPI_SS)|(0<<SPI_MISO);
   SPI_PORTX |= (1<<SPI_MOSI)|(1<<SPI_SCK)|(1<<SPI_SS)|(1<<SPI_MISO);
   
   SPCR = (1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)|(1<<SPR1)|(0<<SPR0);
   SPSR = (0<<SPI2X);
}

void calc_coef_k1(void)
{
	k1=((90-conf_cur.c_30)+conf_cur.c_120);
}

void eep_load(void)
{
	eeprom_read_block(&mode_work_cur, &mode_work[menu_mode_select-1][menu_mode_select_step-1], sizeof(mode_work_t));
}

inline void eep_save(void)
{
	eeprom_update_block(&mode_work_cur, &mode_work[menu_mode_select-1][menu_mode_select_step-1], sizeof(mode_work_t));
}


inline void eep_load_conf(void)
{
	eeprom_read_block(&conf_cur, &conf_e, sizeof(conf_t));
	conf_cur.heating_mode=conf_cur.heating_mode==0?HTMODE_0_1:(conf_cur.heating_mode>HTMODE_MAX?HTMODE_MAX:conf_cur.heating_mode);
	conf_cur.time=conf_cur.time<2?2:conf_cur.time;
	calc_coef_k1();
}

inline void eep_save_conf(void)
{
	eeprom_update_block(&conf_cur, &conf_e, sizeof(conf_t));
}



int16_t SPI_ReadTemp(void)
{
	int16_t report = 0;
	SPI_PORTX &= ~(1<<SPI_SS);
	SPDR = 0;
	while(!(SPSR & (1<<SPIF)));
	report = SPDR;
	SPDR = 0;
	report <<= 8;
	while(!(SPSR & (1<<SPIF)));
	report |= SPDR;
	SPI_PORTX |= (1<<SPI_SS);
	report >>= 3;
	return report;
}

void print_blank(uint8_t c)
{
	for(uint8_t f=0;f<c;f++)
	{
		lcd_putc(' ');
	}
}

// ***************************************************************************************************************************************************
/* mode
0	-	off/end
1	-	on all heater and wait for needed temp
2	-	hold temp needed time
3	-	off heater and wait for time
4	-	off heater and wait for temp
*/
// ***************************************************************************************************************************************************

#define TEMP_AVG 1
#define TEMP_CUR 2

uint8_t avg_temp(void)
{
	uint16_t tmp_temp=0;
	for (uint8_t f=0;f<TEMP_MAX_STEP;f++)
	{
		tmp_temp+=termo[f];
	}
	return (30+conf_cur.c_30)+(int16_t)(tmp_temp/TEMP_MAX_STEP-30)*k1/90;
}


void print_temp(uint8_t data)
{
	uint8_t val=255;
	char bf[11];
	switch (data)
	{
		case TEMP_AVG:
			val=avg_temp();
			termo_cur=val;
			lcd_gotoxy(12, 1);
		break;
		case TEMP_CUR:
//			LED_PORT^=LED_PIN;
			val=mode_work_cur.temp;
			lcd_gotoxy(12, 0);
		break;
	}
	if (val<100)
	{
		lcd_putc(' ');
		if (val<10)
		{
			lcd_putc(' ');
		}
	}
	if (val!=255)
	{
		itoa(val,bf,10);
		lcd_puts(bf);
		lcd_putc(0xDF);
	}
	else
	{
		print_blank(4);
	}
}

void work_finish(void)
{
	if ((conf_cur.sound))
	{
		play_melody=PLAY_LETO;
		mus_play_p(m_leto_p, PLAY_LETO_CNT,1);
	}
	menu_mode=MENU_MODE_FINISH;
	sh_menu=SHOW_MENU_FINISH;
	tenn_on=0;
	tenn_cur=0;
	TENN_PORT&=~(TENN_PIN1|TENN_PIN2|TENN_PIN3);
}

uint8_t work_up;


void heating_mode_up(void)
{
	work_up=1;
	switch(conf_cur.heating_mode)
	{
		case HTMODE_0_1:
		tenn_on=1;
		break;
		case HTMODE_0_2:
		case HTMODE_1_2:
		tenn_on=2;
		break;
		case HTMODE_0_3:
		case HTMODE_1_3:
		case HTMODE_2_3:
		tenn_on=3;
		break;
	}
}

void heating_mode_down(void)
{
	work_up=0;
	switch(conf_cur.heating_mode)
	{
		case HTMODE_0_1:
		case HTMODE_0_2:
		case HTMODE_0_3:
		tenn_on=0;
		break;
		case HTMODE_1_2:
		case HTMODE_1_3:
		tenn_on=1;
		break;
		case HTMODE_2_3:
		tenn_on=2;
		break;
	}
}


void work_step_next(void)
{
	menu_mode_select_step++;
	if (menu_mode_select_step>MODE_SEL_STEPS)
	{
		work_finish();
	}
	else
	{
		eep_load();
		if (mode_work_cur.mode==0)
		{
			work_finish();
		} 
		else
		{
			if ((conf_cur.sound))
			{
				play_melody=PLAY_COLD;
				mus_play_p(m_cold_p, PLAY_COLD_CNT,1);
			}
			lcd_gotoxy(9, 0);			
			lcd_putc('0'+menu_mode_select_step);
			print_temp(TEMP_CUR);
			switch (mode_work_cur.mode)
			{
				case 1:
					tenn_on=3;
					mode_work_cur.sec=0;
				break;
				case 2:
					heating_mode_up();
				break;
				case 3:
					tenn_on=0;
				break;
				case 4:
					tenn_on=0;
					mode_work_cur.sec=0;
				break;
			}
		}
	}
}


void work_histerezis(void)
{
//	colder
	if (work_up==0)
	{
		if ((mode_work_cur.temp-conf_cur.d_down)>=termo_cur)
		{
			heating_mode_up();
		}
	}
// hotter
	if (work_up==1)
	{
		if ((mode_work_cur.temp+conf_cur.d_up)<=termo_cur)
		{
			heating_mode_down();
		}
	}
}

void work_step_check(void)
{
	switch (mode_work_cur.mode)
	{
		case 0:
			work_finish();
		break;
		case 1:
			if (mode_work_cur.temp<=termo_cur)
			{
				 //wait temp
				 work_step_next();
			}
		break;
		case 2:
			if (mode_work_cur.sec==0)
			{
				work_step_next();
			}
			else
			{
				work_histerezis();
			}
		break;
		case 3:
			if (mode_work_cur.sec==0)
			{
				work_step_next();
			}
		break;
		case 4:
			if (mode_work_cur.temp>=termo_cur)
			{
				//wait temp
				work_step_next();
			}
		break;
	}
}


void work(void)
{
	if (menu_mode==MENU_MODE_START)
	{
		switch (mode_work_cur.mode)
		{
			case 1:
			case 4:
					mode_work_cur.sec++;
			break;
			case 2:
			case 3:
				if (mode_work_cur.sec>0)
				{
					mode_work_cur.sec--;
				}
			break;
			default: mode_work_cur.mode=0;

		}
		lcd_gotoxy(4, 1);
		print_time(0);
		work_step_check();
	}
}


void get_temp(void)
{
	uint16_t tmp_temp;
	tmp_temp=SPI_ReadTemp();
	LED_PORT^=LED_PIN;
	termo_cnt++;
	if (termo_cnt==TEMP_MAX_STEP)
	{
		termo_cnt=0;
	}
	termo[termo_cnt]=(tmp_temp/(4));
}

void print_work(void);

void quick_fn(void)
{
  static uint8_t last_tick=255;
  if (last_tick!=ticks)
  {
	last_tick=ticks;
	if (last_tick==0)
	{
		seconds++;
		print_temp(TEMP_AVG);
		work();
		print_work();
	}
	else
	{
		if (((ticks+2)%4)==0)
		{
			get_temp();
		}
	}
	play_check();
	btn_check();
	wdt_reset();
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


void tenn__on(uint8_t idx,uint8_t pin)
{
	if ((TENN_PIN&pin)==0)
	{
			TENN_PORT|=pin;
			tenn_cur++;
 			lcd_gotoxy(idx, 1);
 			lcd_putc(1);			
	}
}

void tenn_off(uint8_t idx,uint8_t pin)
{
	if ((TENN_PIN&pin)!=0)
	{
			TENN_PORT&=~pin;
			tenn_cur--;
 			lcd_gotoxy(idx, 1);
 			lcd_putc(0);
	}
}

void print_work(void)
{
	static uint8_t i=0;
	uint8_t pin=0;
	if (tenn_cur!=tenn_on)
	{
		switch (i)
		{
			case 0:
				pin=TENN_PIN1;
			break;
			case 1:
				pin=TENN_PIN2;
			break;
			case 2:
				pin=TENN_PIN3;
			break;
		}
		if (tenn_cur>tenn_on)
		{
			tenn_off(i,pin);
		}
		else
		{
			tenn__on(i,pin);
		}
	}
/*
		char b[4];
		lcd_gotoxy(0, 0);
		itoa(tenn_cur,b,10);
		lcd_puts(b);
		itoa(tenn_on,b,10);
		lcd_puts(b);
		itoa(TENN_PORT&(TENN_PIN1|TENN_PIN2|TENN_PIN3),b,10);
		lcd_puts(b);
*/
	i++;
	if(i==3)
	{
		i=0;
	}
}


void print_param(uint8_t n)
{
	char buff[5];
	itoa(n,buff,10);
	lcd_puts(buff);
	print_blank(5-strlen(buff));
}

void print_param_signed(int8_t n)
{
	char buff[5];
	itoa(n,buff,10);
	lcd_puts(buff);
	print_blank(5-strlen(buff));
}

uint8_t print_time(int8_t dir)
{
			uint16_t minut;
			char buff[6];
			minut=mode_work_cur.sec/60;
			uint8_t s=0;
			if (dir==0)
			{
				s=mode_work_cur.sec%60;
			}
			else
			{
				minut+=dir;
			}
			if (minut>540)
			{
				minut=540;
			}
			if ((minut==0)&&(dir!=0))
			{
				minut=1;
			}
			uint8_t l;
			uint8_t h;
			uint8_t m;
			h=minut/60;
			m=minut%60;
//			m=minut-h*60;
			itoa(h,buff,10);
			lcd_puts(buff);
			l=1;
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
			if (menu_mode==MENU_MODE_START)
			{
				lcd_putc(':');
				l++;
				itoa(s,buff,10);
				if (strlen(buff)==1)
				{
					lcd_putc('0');
				}
				lcd_puts(buff);
				l++;
				l++;				
			}
			if (dir!=0)
			{
				mode_work_cur.sec=minut*60;
			}
/*			
			lcd_gotoxy(11, 0);
			itoa(dir,buff,10);
			lcd_puts(buff);
			lcd_putc(' ');
*/
			return l;
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
			print_time(dir);
		}
		break;
	}
}


void print_param_heating(uint8_t n)
{
	lcd_putc(0);
	switch(n)
	{
		case HTMODE_0_1:
		case HTMODE_0_2:
		case HTMODE_0_3:
			lcd_putc('0');
			break;
		case HTMODE_1_2:
		case HTMODE_1_3:
			lcd_putc('1');
			break;
		case HTMODE_2_3:	
			lcd_putc('2');
			break;
	}
	lcd_putc(1);
	switch(n)
	{
		case HTMODE_0_1:
			lcd_putc('1');
		break;
		case HTMODE_0_2:
		case HTMODE_1_2:
			lcd_putc('2');
		break;
		case HTMODE_0_3:
		case HTMODE_1_3:
		case HTMODE_2_3:
			lcd_putc('3');
		break;
	}
}

void edit_conf_param(int8_t dir)
{
	lcd_gotoxy(7, 1);
	switch (menu_mode_conf)
	{
		case 1:
			if (dir!=0)
			{
				conf_cur.sound=!conf_cur.sound;
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
			conf_cur.time=conf_cur.time<2?2:conf_cur.time;
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
		case 5:
		conf_cur.c_30+=dir;
			print_param_signed(conf_cur.c_30);
			calc_coef_k1();			
			break;
		case 6:
			conf_cur.c_120+=dir;
			print_param_signed(conf_cur.c_120);
			calc_coef_k1();
		break;
		case 7:
			conf_cur.heating_mode+=dir;
			conf_cur.heating_mode=conf_cur.heating_mode==0?HTMODE_0_1:(conf_cur.heating_mode>HTMODE_MAX?HTMODE_MAX:conf_cur.heating_mode);
			print_param_heating(conf_cur.heating_mode);
		break;
	}
	print_blank(2);
}


void clear_screen(void)
{
	lcd_clrscr();
	lcd_gotoxy(0, 0);	
}

void show_select(void)
{
	lcd_gotoxy(0, 1);
	lcd_puts_P("Select");
	print_blank(5);
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
				show_select();
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
				lcd_gotoxy(0, 0);
				lcd_putc('M');
				lcd_putc(':');
				lcd_putc('0'+menu_mode_select);
				lcd_putc(' ');
				lcd_puts_P("Step:");
				lcd_putc('0'+menu_mode_select_step);
				if (menu_mode==MENU_MODE_START)
				{
					lcd_putc('0'+mode_work_cur.mode);
					lcd_gotoxy(0, 1);
					lcd_putc(0);
					lcd_putc(0);
					lcd_putc(0);
					lcd_putc(' ');
					print_time(0);
				}
				else
				{
					show_select();					
				}
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
					case 5:
						lcd_puts_P("Cr 30C:");
					break;
					case 6:
						lcd_puts_P("Cr120C:");
					break;
					case 7:
						lcd_puts_P("HtMode:");
					break;
				}
				edit_conf_param(0);
			break;
			case SHOW_MENU_CONF_EDIT:
				sh_menu=SHOW_NONE;
				lcd_gotoxy(6, 1);
				lcd_putc('>');
			break;
			case SHOW_MENU_FINISH:
				sh_menu=SHOW_NONE;
				lcd_gotoxy(4,0);
				lcd_puts_P(" Finish");
				print_blank(5);
				lcd_gotoxy(0, 1);
				lcd_putc(0);
				lcd_putc(0);
				lcd_putc(0);
			break;
			
		}
	}
}

void btn_event_finish(void)
{
	sh_menu=SHOW_MENU_MODE;
	menu_mode=MENU_MODE_SEL;
	clear_screen();
}


void btn_event_release(void)
{
		if (btn_press_ev[btn_left]!=0)
		{
			// left
			btn_press_ev[btn_left]=0;
			
			if (conf_cur.sound)
			{
				sound_buff[0].tone=TONE(C3);
				sound_buff[0].leng=O16;
				sound_buff_cnt=1;
				play_melody=PLAY_TOUCH;
				mus_play(sound_buff, sound_buff_cnt,1);
			}
			
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
				case MENU_MODE_CONF_EDIT:
					edit_conf_param(-1);
				break;
				case MENU_MODE_FINISH:
					btn_event_finish();
				break;
			}
		}
		if (btn_press_ev[btn_right]!=0)
		{
			// right
			btn_press_ev[btn_right]=0;

			if (conf_cur.sound)
			{
				sound_buff[0].tone=TONE(G3);
				sound_buff[0].leng=O16;
				sound_buff_cnt=1;
				play_melody=PLAY_TOUCH;
				mus_play(sound_buff, sound_buff_cnt,1);
			}
			
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
				case MENU_MODE_CONF_EDIT:
					edit_conf_param(1);
				break;
				case MENU_MODE_FINISH:
					btn_event_finish();
				break;
				case MENU_MODE_START:
					work_step_next();
				break;
			}
		}
		if (btn_press_ev[btn_up]!=0)
		{
			// up
			btn_press_ev[btn_up]=0;
			
			if (conf_cur.sound)
			{
				sound_buff[0].tone=TONE(E2);
				sound_buff[0].leng=O16;
				sound_buff[1].tone=TONE(G2);
				sound_buff[1].leng=O16;
				sound_buff_cnt=2;
				play_melody=PLAY_TOUCH;
				mus_play(sound_buff, sound_buff_cnt,1);
			}
			
			switch(menu_mode)
			{
				case MENU_MODE_SEL:
					if (menu_mode_select!=MENU_MODE_SEL_MAXS)
					{
						sh_menu=SHOW_MENU_MODE_STEP;
						menu_mode=MENU_MODE_START;
						menu_mode_select_step=0;
						work_step_next();
					}
				break;
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
					eep_save_conf();
					btn_repeat_lr=0;
				break;
				case MENU_MODE_FINISH:
					btn_event_finish();
				break;
			}
		}
		if (btn_press_ev[btn_down]!=0)
		{
			// down
			btn_press_ev[btn_down]=0;
			
			if (conf_cur.sound)
			{
				sound_buff[0].tone=TONE(E2);
				sound_buff[0].leng=O16;
				sound_buff[1].tone=TONE(C2);
				sound_buff[1].leng=O16;
				sound_buff_cnt=2;
				play_melody=PLAY_TOUCH;
				mus_play(sound_buff, sound_buff_cnt,1);
			}
			
			switch(menu_mode)
			{
				case MENU_MODE_START:
					work_finish();
//					sh_menu=SHOW_MENU_MODE;
//					menu_mode=MENU_MODE_SEL;
				break;
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
				case MENU_MODE_FINISH:
					btn_event_finish();
				break;
			}
		}
}

int main(void)
{
	//init switch
	LED_DDR|=LED_PIN;	
	LED_PORT &= ~LED_PIN;
	TENN_DDR|=(TENN_PIN1|TENN_PIN2|TENN_PIN3);
	TENN_PORT &= ~(TENN_PIN1|TENN_PIN2|TENN_PIN3);
	seconds=0;
	spi_init();
	mus_init();
	btn_init();
	btn_repeat_lr=0;
	btn_repeat_ud=0;
	sh_menu=SHOW_MENU_MODE;
	menu_mode=MENU_MODE_SEL;
	menu_mode_select=1;
	menu_mode_param=1;
	sei();

	eep_load_conf();
	if (conf_cur.sound)
	{
		mus_play_p(m_intro_p, PLAY_INTRO_CNT,1);
		play_melody=PLAY_INTRO;
	}
	
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
	memset(termo,20,sizeof(termo));
	tenn_on=0;
	tenn_cur=0;

	if (conf_cur.sound)
	{
		while(play_melody!=0)
		{
			play_check();
		}
	}
	else
	{
		_delay_ms(1000);
	}
	wdt_enable(WDTO_1S);

	lcd_clrscr();
	lcd_gotoxy(0, 0);

	while(1) 
	{
		quick_fn();
		btn_event_release();
		menu();
	}
}
