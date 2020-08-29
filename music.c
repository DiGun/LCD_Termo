/*
 * music.c
 *
 * Created: 25.08.2020 15:27:56
 *  Author: DiGun
 */ 

#include "music.h"



void Timer_Init(void)
{
	// Timer/Counter 0 initialization
	TCCR0=(1<<CS02) | (0<<CS01) | (1<<CS00);
	TCNT0=0x00;

	// Timer/Counter 1 initialization
	TCCR1A=(0<<COM1A1) | (0<<COM1A0) | (0<<COM1B1) | (0<<COM1B0) | (0<<WGM11) | (0<<WGM10);
	TCCR1B=(0<<ICNC1) | (0<<ICES1) | (0<<WGM13) | (0<<WGM12) | (0<<CS12) | (0<<CS11) | (1<<CS10);
	TCNT1H=0x00;
	TCNT1L=0x00;
	ICR1H=0x00;
	ICR1L=0x00;
	OCR1AH=0x00;
	OCR1AL=0x00;
	OCR1BH=0x00;
	OCR1BL=0x00;
	// Timer/Counter 2 initialization
	ASSR=0<<AS2;
	//	TCCR2=(0<<PWM2) | (0<<COM21) | (0<<COM20) | (0<<CTC2) | (1<<CS22) | (1<<CS21) | (1<<CS20);
	TCCR2=(0<<COM21) | (0<<COM20) | (1<<CS22) | (1<<CS21) | (1<<CS20);
	TCNT2=0x00;
	OCR2=0x00;

	// Timer(s)/Counter(s) Interrupt(s) initialization
	TIMSK=(0<<OCIE2) | (0<<TOIE2) | (0<<TICIE1) | (0<<OCIE1A) | (0<<OCIE1B) | (0<<TOIE1) | (1<<TOIE0);
}



void mus_init(void)
{
	SOUND_DDR|=SOUND;
	Timer_Init();
}



ISR(TIMER0_OVF_vect)
{
	if (note_leng)
	{
		note_leng--;
	}
	tick();
}


ISR(TIMER1_OVF_vect)
{
	TCNT1=note_tone;
	SOUND_OUT^=SOUND;
}

uint8_t note_cnt;

#define TONE_OFF TIMSK&=~(1<<TOIE1)
#define TONE_ON TIMSK|=(1<<TOIE1)

void mus_play(note_t n[], uint8_t cnt,uint8_t start)
{
	static uint8_t pause=0;
	if (start==1)
	{
		note_tone=n[0].tone;
		note_leng=n[0].leng;
		TONE_ON;
		note_cnt=1;
		pause=1;
		mus_play_stop=0;
	}
	else
	{
		if (note_leng==0)
		{
			if (note_cnt==0)
			{
				PORTB ^= (1<<PORTB0);
				mus_play_stop=1;
				TONE_OFF;
				return;
			}
			if(pause++&1)
			{
				note_tone=0;
				note_leng=1;
			}
			else
			{
				note_tone=n[note_cnt].tone;
				note_leng=n[note_cnt].leng-1; //minus pause
				note_cnt++;
			}
			if (note_cnt>=cnt)
			{
				note_cnt=0;
			}
			if(note_tone)
			TONE_ON;
			else
			TONE_OFF;
		}
	}
}

void mus_play_p(const note_t* ntp, uint8_t cnt,uint8_t start)
{
	static uint8_t pause=0;
	if (start==1)
	{
		note_t nt;
		memcpy_P(&nt,&ntp[0],sizeof(nt));
		
		note_tone=nt.tone;
		note_leng=nt.leng;
		TONE_ON;
		note_cnt=1;
		pause=1;
		mus_play_stop=0;
	}
	else
	{
		if (note_leng==0)
		{
			if (note_cnt==0)
			{
				PORTB ^= (1<<PORTB0);
				mus_play_stop=1;
				TONE_OFF;
				return;
			}
			if(pause++&1)
			{
				note_tone=0;
				note_leng=1;
			}
			else
			{
				note_t nt;
				memcpy_P(&nt,&ntp[note_cnt],sizeof(nt));

				note_tone=nt.tone;
				note_leng=nt.leng-1; //minus pause
				note_cnt++;
			}
			if (note_cnt>=cnt)
			{
				note_cnt=0;
			}
			if(note_tone)
			TONE_ON;
			else
			TONE_OFF;
		}
	}
}