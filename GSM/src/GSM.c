/*
===============================================================================
 Name        : FRTOS.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

/*****************************************************************************
 * Includes
 ****************************************************************************/

#include "stdlib.h"
#include "chip.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "string.h"
#include <cr_section_macros.h>
#include "ControlVehicular.h"
#include "GPS.h"
#include "UART.h"


/*****************************************************************************
 * Types/enumerations/variables
 ****************************************************************************/


extern SemaphoreHandle_t Semaforo_GSM_Closed;
extern QueueHandle_t Cola_Connect;
extern QueueHandle_t Cola_RX1;

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* funcion para analizar la trama GSM */
//Analiza la trama GSM
void AnalizarTramaGSM (uint8_t dato)
{
	BaseType_t Testigo=pdFALSE;
	static int EstadoTrama=10;
	static char Trama[100];
	static int i;
	uint8_t aux;

	if(dato=='C')		//Inicio de la trama
	{
		EstadoTrama=0;
	}
	else if(dato=='E')
	{
		EstadoTrama=0;
	}
	switch(EstadoTrama)
	{
		case INICIO_TRAMA:		//INICIO_TRAMA
			EstadoTrama=1;
			memset(Trama,0,100);
			i=0;
			Trama[i]=dato;
			i++;
		break;

		case CHEQUEO_FIN:		//CHEQUEO_FIN
			Trama[i]=dato;
			i++;
			if(dato=='N')		//Para buscar CONNECT
			{
				EstadoTrama=2;
			}
			else if(dato=='L')	//Para buscar CLOSED
			{
				EstadoTrama=2;
			}
			else if(dato=='R')	//Para buscar ERROR
			{
				EstadoTrama=2;
			}
		break;

		case CHEQUEO_TRAMA:		//CHEQUEO_TRAMA
			Trama[i]=dato;
			i++;
			if(Trama[0]=='C' && Trama[1]=='O' && Trama[2]=='N')			//CONNECT
			{
				aux=1;
				xQueueOverwrite(Cola_Connect, &aux);
				EstadoTrama=6;	//Trama correcta
			}
			else if(Trama[0]=='C' && Trama[1]=='L' && Trama[2]=='O')	//CLOSED
			{
				xSemaphoreGive(Semaforo_GSM_Closed);
				EstadoTrama=6;	//Trama correcta
				Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
				vTaskDelay(250/portTICK_RATE_MS);	//Espero 1s
				Chip_GPIO_SetPinOutLow(LPC_GPIO, BUZZER);
			}
			else if(Trama[0]=='E' && Trama[1]=='R' && Trama[2]=='R')	//ERROR
			{
				aux=0;
				xQueueOverwrite(Cola_Connect, &aux);
				EstadoTrama=6;	//Trama correcta
			}
			EstadoTrama=6;	//Trama correcta
		break;

		case 6: 	//
		break;

		default:
		break;
	}
}
