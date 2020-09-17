/*
 * conf_io.h
 *
 * Created: 15.07.2020 19:20:21
 *  Author: Metrolog
 */ 


#ifndef CONF_IO_H_
#define CONF_IO_H_

#define BTN				1
#define BTN_1			(1<<PORTD0)
#define BTN_2			(1<<PORTD1)
#define BTN_3			(1<<PORTD2)
#define BTN_4			(1<<PORTD3)
#define BTN_DDR			DDRD
#define BTN_IN			PIND
#define BTN_OUT			PORTD


#define SPI_PORTX PORTB
#define SPI_DDRX DDRB

#define SPI_MISO 4
#define SPI_MOSI 3
#define SPI_SCK 5
#define SPI_SS PORTB2

#define LED_DDR DDRB
#define LED_PORT PORTB
#define LED_PIN (1<<PORTB0)





#endif /* CONF_IO_H_ */