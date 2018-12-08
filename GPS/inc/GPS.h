/*
 * GPS.h
 *
 *  Created on: Oct 5, 2018
 *      Author: Fede
 */

#ifndef EXAMPLE_INC_GPS_H_
#define EXAMPLE_INC_GPS_H_

//////////////////
//		GPS		//
//////////////////

//Defines
#define	INICIO_TRAMA	0
#define CHEQUEO_FIN		1
#define CHEQUEO_TRAMA	2
#define TRAMA_CORRECTA	3
#define HORA_FECHA		4
#define LAT_LONG		5


#define UART_SELECTION_GPS 	LPC_UART1
#define TX_RING_GPS			txring1
#define TX_COLA_GPS			Cola_RX1



//Placa Infotronic
#define LED_STICK	PORT(0),PIN(22)
#define	BUZZER		PORT(0),PIN(28)
#define	SW1			PORT(2),PIN(10)
#define SW2			PORT(0),PIN(18)
#define	SW3			PORT(0),PIN(11)
#define SW4			PORT(2),PIN(13)
#define SW5			PORT(1),PIN(26)
#define	LED1		PORT(2),PIN(0)
#define	LED2		PORT(0),PIN(23)
#define	LED3		PORT(0),PIN(21)
#define	LED4		PORT(0),PIN(27)
#define	RGBB		PORT(2),PIN(1)
#define	RGBG		PORT(2),PIN(2)
#define	RGBR		PORT(2),PIN(3)
#define EXPANSION0	PORT(2),PIN(7)
#define EXPANSION1	PORT(1),PIN(29)
#define EXPANSION2	PORT(4),PIN(28)
#define EXPANSION3	PORT(1),PIN(23)
#define EXPANSION4	PORT(1),PIN(20)
#define EXPANSION5	PORT(0),PIN(19)
#define EXPANSION6	PORT(3),PIN(26)
#define EXPANSION7	PORT(1),PIN(25)
#define EXPANSION8	PORT(1),PIN(22)
#define EXPANSION9	PORT(1),PIN(19)
#define EXPANSION10	PORT(0),PIN(20)
#define EXPANSION11	PORT(3),PIN(25)
#define EXPANSION12	PORT(1),PIN(27)
#define EXPANSION13	PORT(1),PIN(24)
#define EXPANSION14	PORT(1),PIN(21)
#define EXPANSION15	PORT(1),PIN(18)
#define EXPANSION16	PORT(2),PIN(8)
#define EXPANSION17	PORT(2),PIN(12)
#define EXPANSION18	PORT(0),PIN(16)
#define EXPANSION19	PORT(0),PIN(15)
#define EXPANSION20	PORT(0),PIN(22) //RTS1
#define EXPANSION21	PORT(0),PIN(17) //CTS1
//#define EXPANSION22		PORT
//#define EXPANSION23		PORT
//#define EXPANSION24		PORT
#define ENTRADA_ANALOG0 PORT(1),PIN(31)
#define ENTRADA_DIG1	PORT(4),PIN(29)
#define ENTRADA_DIG2	PORT(1),PIN(26)
#define MOSI1			PORT(0),PIN(9)
#define MISO1 			PORT(0),PIN(8)
#define SCK1			PORT(0),PIN(7)
#define SSEL1			PORT(0),PIN(6)

//Funciones
void AnalizarTramaGPS (uint8_t dato);
void uC_StartUp (void);




#endif /* EXAMPLE_INC_GPS_H_ */
