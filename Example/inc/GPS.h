/*
 * GPS.h
 *
 *  Created on: Oct 5, 2018
 *      Author: Fede
 */

#ifndef EXAMPLE_INC_GPS_H_
#define EXAMPLE_INC_GPS_H_


//UART
#define UART_SELECTION 	LPC_UART1
#define IRQ_SELECTION 	UART1_IRQn
#define HANDLER_NAME 	UART1_IRQHandler
#define TXD1	0,15	//RX UART1	(Pin 14 LPC1769)
#define	RXD1	0,16	//TX UART1	(Pin 13 LPC1769)

#define UART_SRB_SIZE 32	//S:Send - Transmit ring buffer size
#define UART_RRB_SIZE 1024	//R:Receive - Receive ring buffer size

//GPS
#define	INICIO_TRAMA	0
#define CHEQUEO_FIN		1
#define CHEQUEO_TRAMA	2
#define TRAMA_CORRECTA	3
#define HORA_FECHA		4
#define LAT_LONG		5


#endif /* EXAMPLE_INC_GPS_H_ */
