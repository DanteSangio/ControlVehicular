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
//#define DEBUGOUT(...) printf(__VA_ARGS__)


extern SemaphoreHandle_t Semaforo_GSM_Closed;
extern QueueHandle_t Cola_Connect;
extern QueueHandle_t Cola_RX1;

//ThingSpeak
char http_cmd[80];
char url_string[] = "api.thingspeak.com/update?";	//URL
char apiKey[] = "api_key=4IVCTNA39FY9U35C";		//Write API key from ThingSpeak: 4IVCTNA39FY9U35C
char data1Original[10] = "&field1=";	//
char data2Original[10] = "&field2=";	//
char data3Original[10] = "&field3=";	//
int status;
int datalen;

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* funcion para analizar la trama GSM */
//Analiza la trama GSM para la verificacion de envio
void AnalizarTramaGSMenvio (uint8_t dato)
{
	static int EstadoTrama=10;
	static char Trama[256];
	static int i;
	uint8_t aux;

	if(dato=='C')		//Inicio de la trama para closed o connect
	{
		EstadoTrama=0;
	}
	else if(dato=='E') 	//inicio de la trama para error
	{
		EstadoTrama=0;
	}
	switch(EstadoTrama)
	{
		case INICIO_TRAMA:		//INICIO_TRAMA
			EstadoTrama=1;
			memset(Trama,0,256);
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

void EnviarMensajeGSM (void)
{
	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms

	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CNMI=2,2,0,0\r", sizeof("AT+CNMI=2,2,0,0\r") - 1); //No guardo los mensajes en memoria, los envio directamente por UART cuando llegan
	vTaskDelay(5000/portTICK_RATE_MS);	//Espero 5s

	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CMGF=1\r", sizeof("AT+CMGF=1\r") - 1); //Activo modo texto. ALT+CMGF = 1 (texto). ALT + CMGF = 0 (PDU)
	vTaskDelay(5000/portTICK_RATE_MS);	//Espero 5s

	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CSCS=\"GSM\"\r", sizeof("AT+CSCS=\"GSM\"\r") - 1);
	vTaskDelay(3000/portTICK_RATE_MS);	//Espero 3s

	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CMGS=\"+5491137863836\"\r", sizeof("AT+CMGS=\"+5491137863836\"\r") - 1);
	vTaskDelay(3000/portTICK_RATE_MS);	//Espero 3s
	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "EMERGENCIA\032", sizeof("EMERGENCIA\032") - 1);
}

void EnviarTramaGSM (char* latitud, char* longitud, char* rfid)
{
	static uint8_t dato=0;
	char auxRfid[16],data1[50],data2[50],data3[50];

	//if(dato==0)
			//{
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
			//}

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIPSTART=\"TCP\",\"", sizeof("AT+CIPSTART=\"TCP\",\"") - 1); //
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "184.106.153.149\",\"80\"\r", sizeof("184.106.153.149\",\"80\"\r") - 1); //
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 3s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIPSEND=92\r", sizeof("AT+CIPSEND=92\r") - 1); //44 // 92 para los 3
			vTaskDelay(500/portTICK_RATE_MS);	//Espero 1s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "GET ", sizeof("GET ") - 1); //	GET
			vTaskDelay(200/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "/update?", sizeof("/update?") - 1); //
			vTaskDelay(200/portTICK_RATE_MS);	//Espero 100ms


			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, (void*)apiKey, sizeof(apiKey) - 1); //
			vTaskDelay(200/portTICK_RATE_MS);	//Espero 100ms

			//latitud
			strcpy(data1, data1Original);
			strcat(data1,latitud);//le sumo al campo 1 la latitud
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, (void*)data1, strlen(data1)); //
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms


			//envio longitud
			strcpy(data2, data2Original);
			strcat(data2,longitud);
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, (void*)data2, strlen(data2)); //
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms


			//rfid
			strcpy(data3, data3Original);
			strcat(data3,rfid);
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, (void*)data3, strlen(data3)); //
			vTaskDelay(200/portTICK_RATE_MS);	//Espero 100ms


			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "\r\n\r\n", sizeof("\r\n\r\n") - 1); //
			//vTaskDelay(10000/portTICK_RATE_MS);	//Espero 30s


			//analizo para verificar si hubo error, connect y/o closed

			while(LeerCola(RX_COLA_GSM,&dato,1))
			{
				AnalizarTramaGSMenvio(dato);
				//DEBUGOUT("%c", dato);	//Imprimo en la consola
			}

			xSemaphoreTake(Semaforo_GSM_Closed, 10000/portTICK_RATE_MS);


			xQueuePeek(Cola_Connect, &dato, portMAX_DELAY);			//Para chequear si dio un error
}

void RecibirTramaGSM(void)
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

	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIPSTART=\"TCP\",\"", sizeof("AT+CIPSTART=\"TCP\",\"") - 1); //
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "184.106.153.149\",\"80\"\r", sizeof("184.106.153.149\",\"80\"\r") - 1); //
	vTaskDelay(1000/portTICK_RATE_MS);	//Espero 3s

	//61  GET /channels/648303/fields/1.json?api_key=VRMSAZ0CSI5VVHDM
	//58  GET /channels/648303/feeds.json?api_key=VRMSAZ0CSI5VVHDM
	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIPSEND=58\r", sizeof("AT+CIPSEND=58\r") - 1); //44
	vTaskDelay(500/portTICK_RATE_MS);	//Espero 1s

	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "GET ", sizeof("GET ") - 1); //	GET
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "/channels/", sizeof("/channels/") - 1); //
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, CHANNEL2, sizeof(CHANNEL2) - 1); //
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "/feeds.json?api_key=", sizeof("/feeds.json?api_key=") - 1); //
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, APIKEY2, sizeof(APIKEY2) - 1); //
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms

	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "\r\n\r\n", sizeof("\r\n\r\n") - 1); //
	//vTaskDelay(10000/portTICK_RATE_MS);	//Espero 30s

}
//https://api.thingspeak.com/channels/648303/fields/1.json?api_key=VRMSAZ0CSI5VVHDM&results=2 url que hay que mandar para leer 2 resultados de tarjetas
//https://api.thingspeak.com/update?api_key=BKHDQTKSFJ6YONWX&field1=4266702969&field2=Pepe comando para cargar los campos online



///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* funcion para analizar la trama GSM de tarjetas*/
//Analiza la trama GSM para la verificacion de envio
Tarjetas_RFID* AnalizarTramaGSMrecibido (uint8_t dato)
{
	static int EstadoTrama=10;
	static char Trama[2048];
	static uint16_t i, k=0;
	static uint16_t j ,z;
	static Tarjetas_RFID tarjetas[30];//creo 100 struct de tarjetas

	if(dato=='[')		//Inicio de la trama de datos de tarjetas
	{
		EstadoTrama=0;
	}

	if(dato==']')		//Fin de la trama de datos de tarjetas
	{
		EstadoTrama=6;
	}

	switch(EstadoTrama)
	{
		case INICIO_TRAMA:		//INICIO_TRAMA
			EstadoTrama=CHEQUEO_TRAMA;
			memset(Trama,0,2048);
			i=0;
		break;

		case CHEQUEO_TRAMA:		//Voy guardando la trama a partir de un "{" , guardo 70 caracteres que son los de las tarjetas
			Trama[i]=dato;

			if (dato == '}')
			{

				for(j=0,z=0;z != ',';j++)//Busco donde estan la coma de que termino la tarjeta
				{
					z = Trama[i-j];
				}
				z = i-j;
				for(j=0;j<10;j++)
				{
					tarjetas[k].tarjeta[j] = Trama[z-10+j]; //guardo desde la posicion de la coma 10 lugares atras
				}
				tarjetas[k].tarjeta[10]=0;//le pongo un null para indicar el fin de la trama



				for(j=0,z=0;z != '"';j++)//Busco donde estan las comillas de inicio del nombre
				{
					z = Trama[i-j-2];
				}
				z = i-j;
				for(j=0; z<i-1 ;z++,j++)
				{
					tarjetas[k].nombre[j] = Trama[z]; //guardo desde las comillas hasta un lugar menos del "}"
				}
				tarjetas[k].nombre[j] =0;//pongo un null en el ultimo caracter
				k++;
			}
			i++;
		break;


		case 6:
			for(j=0;j<11;j++) //me aseguro que la ultima tarjeta tenga todas las posiciones en 0
			{
				tarjetas[k].tarjeta[j]=0;
			}
			return(tarjetas);//devuelvo la direccion de la primer tarjeta, solo lo hago en caso de que las haya cargado
		break;

		default:
		break;
	}
	return (NULL);//me aseguro que si no finalizo
}
