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

/*****************************************************************************
 * Defines
 ****************************************************************************/

#define ON			((uint8_t) 1)
#define OFF			((uint8_t) 0)
#define PORT(x) 	((uint8_t) x)
#define PIN(x)		((uint8_t) x)
#define OUTPUT		((uint8_t) 1)
#define INPUT		((uint8_t) 0)
#define DEBUGOUT(...) printf(__VA_ARGS__)

//UART
#define UART_SELECTION_GSM 	LPC_UART2
#define TX_RING_GSM			txring2


#define TXD1	0,15	//RX UART1	(Pin 14 LPC1769) (EXP 18)
#define	RXD1	0,16	//TX UART1	(Pin 13 LPC1769) (EXP 19)
#define TXD2	0,10	//RX UART2	(Pin 41 LPC1769)
#define RXD2	0,11	//TX UART2	(Pin 40 LPC1769)

#define UART_SRB_SIZE 1024	//S:Send - Transmit ring buffer size
#define UART_RRB_SIZE 1024	//R:Receive - Receive ring buffer size

//GSM
#define RST_GSM	1,18	//Expansion 15	(NO CONECTAR)

//GPS
#define	INICIO_TRAMA	0
#define CHEQUEO_FIN		1
#define CHEQUEO_TRAMA	2
#define TRAMA_CORRECTA	3
#define HORA_FECHA		4
#define LAT_LONG		5

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

/*****************************************************************************
 * Types/enumerations/variables
 ****************************************************************************/
SemaphoreHandle_t Semaforo_RX2;
QueueHandle_t Cola_RX;
QueueHandle_t Cola_TX;
QueueHandle_t Cola_Pulsadores;

static RINGBUFF_T txring2, rxring2;								//Transmit and receive ring buffers
static uint8_t rxbuff2[UART_RRB_SIZE], txbuff2[UART_SRB_SIZE];	//Transmit and receive buffers

//ThingSpeak
char http_cmd[80];
char url_string[] = "api.thingspeak.com/update?";	//URL
char apiKey[] = "api_key=4IVCTNA39FY9U35C&";		//Write API key from ThingSpeak: 4IVCTNA39FY9U35C
char data[] = "field1=30";	//
int status;
int datalen;

/*****************************************************************************
 * Functions
 ****************************************************************************/
BaseType_t LeerCola(QueueHandle_t xQueue, uint8_t *Dato, uint8_t cantidad);
void EscribirCola(QueueHandle_t xQueue, uint8_t *Dato, uint8_t cantidad);


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* uC_StartUp */
void uC_StartUp (void)
{
	DEBUGOUT("Configurando pines I/O..\n");	//Imprimo en la consola
	Chip_GPIO_Init (LPC_GPIO);

	//Inicializacion de los pines del GSM
	Chip_GPIO_SetDir (LPC_GPIO, RXD2, INPUT);
	Chip_IOCON_PinMux (LPC_IOCON, RXD2, IOCON_MODE_INACT, IOCON_FUNC1);
	Chip_GPIO_SetDir (LPC_GPIO, TXD2, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, TXD2, IOCON_MODE_INACT, IOCON_FUNC1);
	Chip_GPIO_SetDir (LPC_GPIO, RST_GSM, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, RST_GSM, IOCON_MODE_INACT, IOCON_FUNC0);

	//Inicializacion de los pines de la Placa Infotronic
	Chip_GPIO_SetDir (LPC_GPIO, LED_STICK, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, LED_STICK, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, BUZZER, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, BUZZER, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, RGBB, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, RGBB, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, RGBG, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, RGBG, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, RGBR, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, RGBR, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, LED1, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, LED1, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, LED2, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, LED2, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, LED3, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, LED3, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, LED4, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, LED4, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, SW1, INPUT);
	Chip_IOCON_PinMux (LPC_IOCON, SW1, IOCON_MODE_PULLDOWN, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, SW2, INPUT);
	Chip_IOCON_PinMux (LPC_IOCON, SW2, IOCON_MODE_PULLDOWN, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, SW3, INPUT);
	Chip_IOCON_PinMux (LPC_IOCON, SW3, IOCON_MODE_PULLDOWN, IOCON_FUNC0);

	//Salidas apagadas
	Chip_GPIO_SetPinOutLow(LPC_GPIO, LED_STICK);
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, RGBR);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, RGBG);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, RGBB);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, LED1);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, LED2);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, LED3);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, LED4);
}










///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskCargarAnillo */
//Encargada de cargar el anillo a partir de la cola
static void vTaskCargarAnillo(void *pvParameters)
{
	ANILLO * Buffer;
	Buffer = (ANILLO*) pvParameters;

	uint8_t Receive=0;
	while (1)
	{
		xQueueReceive(Buffer.cola, &Receive, portMAX_DELAY);
		Chip_UART_SendRB(Buffer.uart, Buffer->anillo, &Receive , 1);
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}



typedef struct
{
	SemaphoreHandle_t 	Rx;
	extern RINGBUFF_T 	anillo;
	SemaphoreHandle_t 	cola;
	uint32_t			uart;
}ANILLO;
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskLeerAnillo */
//Pasa la informacion del anillo a la cola de recepcion
static void vTaskLeerAnillo(void *pvParameters)
{

	ANILLO * Buffer;
	Buffer = (ANILLO*) pvParameters;

	uint8_t Receive=0;
	uint8_t Testigo=0, dato=0;

	while (1)
	{
		xSemaphoreTake(Buffer.Rx, portMAX_DELAY);
		Testigo = RingBuffer_Pop(Buffer->anillo, &Receive);
		if(Testigo)
		{
			xSemaphoreGive(Buffer.Rx);
			xQueueSendToBack(Buffer.cola, &Receive, portMAX_DELAY);
		}

		//leo la cola de rercepcion y lo muestro en pantalla
		if(LeerCola(Buffer.cola,&dato,1))
		{
			DEBUGOUT("%c", dato);	//Imprimo en la consola
		}

	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* funcion para escribir la cola */
//
void EscribirCola(QueueHandle_t xQueue, uint8_t *Dato, uint8_t cantidad)
{
	uint8_t i=0;

	for(i=0;i<cantidad;i++)
	{
		xQueueSendToBack(xQueue, &Dato[i], portMAX_DELAY);
	}

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* funcion para leer la cola */
//Devuelve 0 si no estan disponibles la cantidad de items pedidos, caso contrario devuelve 1
BaseType_t LeerCola(QueueHandle_t xQueue, uint8_t *Dato, uint8_t cantidad)
{
	uint8_t i=0;
	UBaseType_t uxNumberOfItems;

	/* How many items are currently in the queue referenced by the xQueue handle? */
	uxNumberOfItems = uxQueueMessagesWaiting( xQueue );

	if(uxNumberOfItems < cantidad)
		return pdFALSE;
	else
	{
		for(i=0;i<cantidad;i++)
		{
			xQueueReceive(xQueue, &Dato[i], portMAX_DELAY);
		}
		return pdTRUE;
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskEnviarGSM */
static void vTaskEnviarGSM(void *pvParameters)
{
	uint8_t Receive=OFF;

	while(1)
	{
		xQueueReceive(Cola_Pulsadores, &Receive, portMAX_DELAY);

		if(Receive==1)		//1: ENVIAR SMS
		{
			Receive=OFF;	//Reestablezco variable

			///////////////////
			//Para enviar SMS

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

		else if(Receive==2)	//2: ENVIAR DATOS POR GPRS A THINGSPEAK
		{
			Receive=OFF;	//Reestablezco variable

			/////////////////// SEGUN 	https://www.youtube.com/watch?v=f-VhitIURlY
			//Para enviar datos por GPRS a ThingSpeak

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms

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
			vTaskDelay(3000/portTICK_RATE_MS);	//Espero 3s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIPSEND=48\r", sizeof("AT+CIPSEND=48\r") - 1); //44
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "GET ", sizeof("GET ") - 1); //	GET
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "/update?", sizeof("/update?") - 1); //
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, apiKey, sizeof(apiKey) - 1); //
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, data, sizeof(data) - 1); //
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "\r\n\r\n", sizeof("\r\n\r\n") - 1); //
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s
		}
	}
	vTaskDelete(NULL);	//Borra la tarea
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* xTaskPulsadores */
static void xTaskPulsadores(void *pvParameters)
{
	uint8_t Send=OFF;

	while (1)
	{
		if(Chip_GPIO_GetPinState(LPC_GPIO, SW1)==OFF)	//Si se presiona el SW1
		{
			Send=1;	//Para enviar SMS
			xQueueSendToBack(Cola_Pulsadores, &Send, portMAX_DELAY);
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s
		}
		if(Chip_GPIO_GetPinState(LPC_GPIO, SW2)==OFF)	//Si se presiona el SW2
		{
			Send=2;	//Para enviar datos por GPRS a ThingSpeak
			xQueueSendToBack(Cola_Pulsadores, &Send, portMAX_DELAY);
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s
		}
		if(Chip_GPIO_GetPinState(LPC_GPIO, SW3)==OFF)	//Si se presiona el SW3
		{
			Send=3;	//Para enviar datos por GPRS a ThingSpeak
			xQueueSendToBack(Cola_Pulsadores, &Send, portMAX_DELAY);
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* main
*/
int main(void)
{
	DEBUGOUT("Inicializando prueba GSM..\n");	//Imprimo en la consola

	uC_StartUp();

	SystemCoreClockUpdate();

	vSemaphoreCreateBinary(Semaforo_RX2);			//Creamos el semaforo de la Uart 2

	Cola_RX = xQueueCreate(UART_RRB_SIZE, sizeof(uint8_t));	//Creamos una cola

	Cola_TX = xQueueCreate(UART_SRB_SIZE, sizeof(uint8_t));	//Creamos una cola

	Cola_Pulsadores = xQueueCreate(1, sizeof(uint32_t));	//Creamos una cola

	xTaskCreate(vTaskLeerAnillo, (char *) "vTaskLeerAnillo",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskCargarAnillo, (char *) "vTaskCargarAnillo",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskUART1Config, (char *) "xTaskUART1Config",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskGSMConfig, (char *) "vTaskGSMConfig",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskEnviarGSM, (char *) "vTaskEnviarGSM",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskPulsadores, (char *) "vTaskPulsadores",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Nunca debería arribar aquí */
    return 0;
}

