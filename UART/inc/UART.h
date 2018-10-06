/*
 * UART.h
 *
 *  Created on: Oct 5, 2018
 *      Author: Fede
 */

#ifndef EXAMPLE_INC_UART_H_
#define EXAMPLE_INC_UART_H_

//////////////////////
//		UART		//
//////////////////////

//Defines
#define UART_SELECTION 	LPC_UART1
#define IRQ_SELECTION 	UART1_IRQn
#define HANDLER_NAME 	UART1_IRQHandler
#define TXD1	0,15	//RX UART1	(Pin 14 LPC1769) (EXP 18)
#define	RXD1	0,16	//TX UART1	(Pin 13 LPC1769) (EXP 19)
#define TXD2	0,10	//RX UART2	(Pin 41 LPC1769)
#define RXD2	0,11	//TX UART2	(Pin 40 LPC1769)

#define UART_SRB_SIZE 1024	//S:Send - Transmit ring buffer size
#define UART_RRB_SIZE 1024	//R:Receive - Receive ring buffer size


//Funciones
BaseType_t LeerCola(QueueHandle_t xQueue, uint8_t *Dato, uint8_t cantidad);
void EscribirCola(QueueHandle_t xQueue, uint8_t *Dato, uint8_t cantidad);


#endif /* EXAMPLE_INC_UART_H_ */
