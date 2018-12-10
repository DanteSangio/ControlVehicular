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
#include "GSM.h"
#include "UART.h"


/*****************************************************************************
 * Types/enumerations/variables
 ****************************************************************************/


extern SemaphoreHandle_t Semaforo_GSM_Closed;
extern QueueHandle_t Cola_Connect;
extern QueueHandle_t Cola_RX1;

//ThingSpeak
char http_cmd[80];
char url_string[] = "api.thingspeak.com/update?";	//URL
char apiKey[] = "api_key=4IVCTNA39FY9U35C&";		//Write API key from ThingSpeak: 4IVCTNA39FY9U35C
char data1[50] = "field1=";	//
char data2[50] = "field2=";	//
char data3[50] = "field3=";	//
int status;
int datalen;

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* funcion para analizar la trama GSM */
//Analiza la trama GSM
void AnalizarTramaGSM (uint8_t dato)
{
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
				vTaskDelay(150/portTICK_RATE_MS);	//Espero 1s
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

void EnviarTramaGSM (char* latitud, char* longitud, unsigned int rfid)
{
	static uint8_t dato=0;
	char auxRfid[16];
	if(dato==0)
			{
				Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
				vTaskDelay(500/portTICK_RATE_MS);	//Espero 100ms
				Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
				vTaskDelay(500/portTICK_RATE_MS);	//Espero 100ms
				Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
				vTaskDelay(500/portTICK_RATE_MS);	//Espero 100ms

				Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIPSHUT\r", sizeof("AT+CIPSHUT\r") - 1); //
				vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s

				Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIPMUX=0\r", sizeof("AT+CIPMUX=0\r") - 1); //
				vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s

				Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CGATT=1\r", sizeof("AT+CGATT=1\r") - 1); //
				vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s

				Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CSTT=\"FONAnet\"\r", sizeof("AT+CSTT=\"FONAnet\"\r") - 1); //
				vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s

				Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIICR\r", sizeof("AT+CIICR\r") - 1); //
				vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s

				Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIFSR\r", sizeof("AT+CIFSR\r") - 1); //
				vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s
			}

			//envio la latitud
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIPSTART=\"TCP\",\"", sizeof("AT+CIPSTART=\"TCP\",\"") - 1); //
			vTaskDelay(500/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "184.106.153.149\",\"80\"\r", sizeof("184.106.153.149\",\"80\"\r") - 1); //
			vTaskDelay(3000/portTICK_RATE_MS);	//Espero 3s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIPSEND=48\r", sizeof("AT+CIPSEND=48\r") - 1); //44
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "GET ", sizeof("GET ") - 1); //	GET
			vTaskDelay(500/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "/update?", sizeof("/update?") - 1); //
			vTaskDelay(500/portTICK_RATE_MS);	//Espero 100ms


			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, (void*)apiKey, sizeof(apiKey) - 1); //
			vTaskDelay(500/portTICK_RATE_MS);	//Espero 100ms

			strcat(data1,latitud);//le sumo al campo 1 la latitud

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, (void*)data1, sizeof(data1) - 1); //
			vTaskDelay(500/portTICK_RATE_MS);	//Espero 100ms

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "\r\n\r\n", sizeof("\r\n\r\n") - 1); //
			//vTaskDelay(10000/portTICK_RATE_MS);	//Espero 30s

			xSemaphoreTake(Semaforo_GSM_Closed, 10000/portTICK_RATE_MS);

			//envio la longitud
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIPSTART=\"TCP\",\"", sizeof("AT+CIPSTART=\"TCP\",\"") - 1); //
			vTaskDelay(500/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "184.106.153.149\",\"80\"\r", sizeof("184.106.153.149\",\"80\"\r") - 1); //
			vTaskDelay(3000/portTICK_RATE_MS);	//Espero 3s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIPSEND=48\r", sizeof("AT+CIPSEND=48\r") - 1); //44
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "GET ", sizeof("GET ") - 1); //	GET
			vTaskDelay(500/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "/update?", sizeof("/update?") - 1); //
			vTaskDelay(500/portTICK_RATE_MS);	//Espero 100ms


			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, (void*)apiKey, sizeof(apiKey) - 1); //
			vTaskDelay(500/portTICK_RATE_MS);	//Espero 100ms

			strcat(data2,longitud);//le sumo al campo 2 la longitud

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, (void*)data2, sizeof(data2) - 1); //
			vTaskDelay(500/portTICK_RATE_MS);	//Espero 100ms

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "\r\n\r\n", sizeof("\r\n\r\n") - 1); //
			//vTaskDelay(10000/portTICK_RATE_MS);	//Espero 30s

			xSemaphoreTake(Semaforo_GSM_Closed, 10000/portTICK_RATE_MS);

			//envio RFID
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIPSTART=\"TCP\",\"", sizeof("AT+CIPSTART=\"TCP\",\"") - 1); //
			vTaskDelay(500/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "184.106.153.149\",\"80\"\r", sizeof("184.106.153.149\",\"80\"\r") - 1); //
			vTaskDelay(3000/portTICK_RATE_MS);	//Espero 3s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIPSEND=48\r", sizeof("AT+CIPSEND=48\r") - 1); //44
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "GET ", sizeof("GET ") - 1); //	GET
			vTaskDelay(500/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "/update?", sizeof("/update?") - 1); //
			vTaskDelay(500/portTICK_RATE_MS);	//Espero 100ms


			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, (void*)apiKey, sizeof(apiKey) - 1); //
			vTaskDelay(500/portTICK_RATE_MS);	//Espero 100ms

			itoa(rfid,auxRfid,10);
			strcat(data3,auxRfid);//le sumo al campo 3 el rfid

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, (void*)data3, sizeof(data3) - 1); //
			vTaskDelay(500/portTICK_RATE_MS);	//Espero 100ms

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "\r\n\r\n", sizeof("\r\n\r\n") - 1); //
			//vTaskDelay(10000/portTICK_RATE_MS);	//Espero 30s

			xSemaphoreTake(Semaforo_GSM_Closed, 10000/portTICK_RATE_MS);

			xQueuePeek(Cola_Connect, &dato, portMAX_DELAY);			//Para chequear si dio un error



}
