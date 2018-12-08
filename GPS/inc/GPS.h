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


#define UART_SELECTION_GPS 	LPC_UART2
#define TX_RING_GPS			rxring2
#define RX_COLA_GPS			Cola_RX2


//Funciones
void AnalizarTramaGPS (uint8_t dato);
void uC_StartUp (void);




#endif /* EXAMPLE_INC_GPS_H_ */
