/*
 * music.h
 *
 * Created: 25.08.2020 15:26:53
 *  Author: Master
 */ 


#ifndef MUSIC_H_
#define MUSIC_H_

#define SOUND			(1<<PORTC0)
#define SOUND_DDR       DDRC
#define SOUND_OUT		PORTC



#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>



volatile uint16_t note_tone;
volatile uint16_t note_leng;
uint8_t melod_note_cnt;
uint8_t mus_play_stop;




#define TONE(n) (uint16_t)(65536-(F_CPU/n))

#define E4 	 5274.0
#define D4d  4978.0
#define D4 	 4698.4
#define C4d  4434.8
#define C4 	 4186.0
#define B3 	 3951.0
#define A3d  3729.2
#define A3 	 3440.0
#define G3d  3332.4
#define G3 	 3136.0
#define F3d  2960.0
#define F3 	 2793.8
#define E3 	 2637.0
#define D3d  2489.0
#define D3 	 2349.2
#define C3d  2217.4
#define C3 	 2093.0
#define B2 	 1975.5
#define A2d  1864.6
#define A2 	 1720.0
#define G2d  1661.2
#define G2 	 1568.0
#define F2d  1480.0
#define F2 	 1396.9
#define E2 	 1318.5
#define D2d  1244.5
#define D2 	 1174.6
#define C2d  1108.7
#define C2 	 1046.5
#define B1 	 987.75
#define A1d  932.32
#define A1 	 880.00
#define G1d  830.60
#define G1 	 784.00
#define F1d  739.98
#define F1 	 698.46
#define E1 	 659.26
#define D1d  622.26
#define D1 	 587.32
#define C1d  554.36
#define C1 	 523.25
#define B0 	 493.88
#define A0d  466.16
#define A0 	 440.00
#define G0d  415.30
#define G0 	 392.00
#define F0d  369.99
#define F0 	 349.23
#define E0 	 329.63
#define D0d  311.13
#define D0 	 293.66
#define C0d  277.18
#define C0 	 261.63
#define Bs 	 246.96
#define Asd  233.08
#define As 	 220.00
#define Gsd  207.00
#define Gs 	 196.00
#define Fsd  185.00
#define Fs 	 174.62



#define TEMPO	3
#define O16		TEMPO
#define O8		TEMPO*2
#define O4		TEMPO*4
#define O2		TEMPO*8
#define O1		TEMPO*16




typedef struct
{
	uint16_t tone;
	uint8_t leng;
}  note_t;





void mus_init(void);
void mus_play(note_t n[], uint8_t cnt,uint8_t start);
void mus_play_p(const note_t* ntp, uint8_t cnt,uint8_t start);
void tick(void);

#endif /* MUSIC_H_ */