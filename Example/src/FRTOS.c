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
#define UART_SELECTION 	LPC_UART1
#define IRQ_SELECTION 	UART1_IRQn
#define HANDLER_NAME 	UART1_IRQHandler
#define TXD1	0,15	//TX UART1
#define	RXD1	0,16	//RX UART1

#define UART_SRB_SIZE 32	//S:Send - Transmit ring buffer size
#define UART_RRB_SIZE 1024	//R:Receive - Receive ring buffer size

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
#define EXPANSION18	PORT(0),PIN(16) //RX1 No se puede usar
#define EXPANSION19	PORT(0),PIN(15) //TX1 No se puede usar
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
SemaphoreHandle_t Semaforo_RX;
QueueHandle_t Cola_RX;
QueueHandle_t Cola_TX;

STATIC RINGBUFF_T txring, rxring;	//Transmit and receive ring buffers
static uint8_t rxbuff[UART_RRB_SIZE], txbuff[UART_SRB_SIZE];	//Transmit and receive buffers

volatile int HourGPS, MinuteGPS, DayGPS, MonthGPS, YearGPS;

/*****************************************************************************
 * Functions
 ****************************************************************************/
BaseType_t LeerCola(QueueHandle_t xQueue, uint8_t *Dato, uint8_t cantidad);
void EscribirCola(QueueHandle_t xQueue, uint8_t *Dato, uint8_t cantidad);
void AnalizarTramaGPS (uint8_t dato);

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* uC_StartUp */
void uC_StartUp (void)
{
	Chip_GPIO_Init (LPC_GPIO);
	//Inicializacion de los pines de la UART1
	Chip_GPIO_SetDir (LPC_GPIO, RXD1, INPUT);
	Chip_IOCON_PinMux (LPC_IOCON, RXD1, IOCON_MODE_INACT, IOCON_FUNC1);
	Chip_GPIO_SetDir (LPC_GPIO, TXD1, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, TXD1, IOCON_MODE_INACT, IOCON_FUNC1);

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


void HANDLER_NAME(void)
{
	BaseType_t Testigo=pdFALSE;

	/* Handle transmit interrupt if enabled */
	if (UART_SELECTION->IER & UART_IER_THREINT)
	{
		Chip_UART_TXIntHandlerRB(UART_SELECTION, &txring);

		/* Disable transmit interrupt if the ring buffer is empty */
		if (RingBuffer_IsEmpty(&txring))
		{
			Chip_UART_IntDisable(UART_SELECTION, UART_IER_THREINT);
		}
	}

	/* Handle receive interrupt */
	/* New data will be ignored if data not popped in time */
	if (UART_SELECTION->IIR & UART_IIR_INTID_RDA )
	{
		while (Chip_UART_ReadLineStatus(UART_SELECTION) & UART_LSR_RDR)
		{
			uint8_t ch = Chip_UART_ReadByte(UART_SELECTION);
			RingBuffer_Insert(&rxring, &ch);
			xSemaphoreGiveFromISR(Semaforo_RX, &Testigo);	//Devuelve si una de las tareas bloqueadas tiene mayor prioridad que la actual
		}
		portYIELD_FROM_ISR(Testigo);					//Si testigo es TRUE -> ejecuta el scheduler
	}
}


static void xTaskUART1Config(void *pvParameters)
{
	DEBUGOUT("PRUEBA UART..\n");	//Imprimo en la consola

	/* Setup UART for 9600 */
	Chip_UART_Init(UART_SELECTION);
	Chip_UART_SetBaud(UART_SELECTION, 9600);
	Chip_UART_ConfigData(UART_SELECTION, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(UART_SELECTION, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(UART_SELECTION);

	/* Before using the ring buffers, initialize them using the ring buffer init function */
	RingBuffer_Init(&rxring, rxbuff, 1, UART_RRB_SIZE);
	RingBuffer_Init(&txring, txbuff, 1, UART_SRB_SIZE);

	/* Reset and enable FIFOs, FIFO trigger level 3 (14 chars) */
	Chip_UART_SetupFIFOS(UART_SELECTION, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
							UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(UART_SELECTION, (UART_IER_RBRINT | UART_IER_RLSINT));

	//Habilito interrupcion UART1
	NVIC_EnableIRQ(IRQ_SELECTION);

	vTaskDelete(NULL);	//Borra la tarea
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskCargarAnillo */
//Encargada de cargar el anillo a partir de la cola
static void vTaskCargarAnillo(void *pvParameters)
{
	uint8_t Receive=0;
	while (1)
	{
		xQueueReceive(Cola_TX, &Receive, portMAX_DELAY);
		Chip_UART_SendRB(UART_SELECTION, &txring, &Receive , 1);
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskLeerAnillo */
//Pasa la informacion del anillo a la cola de recepcion
static void vTaskLeerAnillo(void *pvParameters)
{
	uint8_t Receive=0;
	uint8_t Testigo=0, dato=0;

	while (1)
	{
		xSemaphoreTake(Semaforo_RX, portMAX_DELAY);
		Testigo = RingBuffer_Pop(&rxring, &Receive);
		if(Testigo)
		{
			xSemaphoreGive(Semaforo_RX);
			xQueueSendToBack(Cola_RX, &Receive, portMAX_DELAY);
		}

		//leo la cola de rercepcion y lo muestro en pantalla
		if(LeerCola(Cola_RX,&dato,1))
		{
			AnalizarTramaGPS(dato);
			//DEBUGOUT("%c", dato);	//Imprimo en la consola
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
/* funcion para analizar la trama GPS */
//
void AnalizarTramaGPS (uint8_t dato)
{
	static int i=0;
	static int c=0;
	static int EstadoTrama=0;
	static char Trama[100];
	static bool DatoCorrecto=OFF;
	static char HourMinute[4];
	static char Date[6];
	static char Lat[10];
	static char Long[10];

	if(dato=='$')		//Inicio de la trama
	{
		EstadoTrama=0;
	}
	switch(EstadoTrama)
	{
		case 0:		//INICIO_TRAMA
			EstadoTrama=1;
			memset(Trama,0,100);
			i=0;
		break;

		case 1:		//CHEQUEO_FIN
			Trama[i]=dato;
			i++;
			if(dato=='*')
			{
				EstadoTrama=2;
			}
		break;

		case 2:		//CHEQUEO_TRAMA
			if(Trama[16]=='A' && Trama[29]=='S' && Trama[43]=='W')
			{
				EstadoTrama=3;	//Trama correcta
			}
		break;

		case 3:		//TRAMA_CORRECTA
			DEBUGOUT("%s",Trama);
			DEBUGOUT("\n");
			EstadoTrama=4;
		break;

		case 4:		//HORA Y FECHA
			for(i=6;i<=9;i++)
			{
				HourMinute[i-6]=Trama[i];
			}
			HourMinute[4]='\0';
			DEBUGOUT("%s\t",HourMinute);
			for(i=52;i<=57;i++)
			{
				Date[i-52]=Trama[i];
			}
			Date[6]='\0';
			DEBUGOUT("%s\n",Date);
			EstadoTrama=5;
		break;

		case 5: 	//LATITUD Y LONGITUD
			for(i=18;i<=27;i++)
			{
				Lat[i-18]=Trama[i];
			}
			Lat[10]='\0';
			DEBUGOUT("%s\t",Lat);
			for(i=32;i<=41;i++)
			{
				Long[i-32]=Trama[i];
			}
			Long[10]='\0';
			DEBUGOUT("%s",Long);
			DEBUGOUT("\n");

			EstadoTrama=6;
		break;

		case 6: 	//
		break;

		default:
		break;
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* main
*/
int main(void)
{
	//const char inst1[] = "Hola manco\r\n";

	uC_StartUp();

	SystemCoreClockUpdate();

	vSemaphoreCreateBinary(Semaforo_RX);			//Creamos el semaforo

	Cola_RX = xQueueCreate(UART_RRB_SIZE, sizeof(uint8_t));	//Creamos una cola

	Cola_TX = xQueueCreate(UART_SRB_SIZE, sizeof(uint8_t));	//Creamos una cola

	//EscribirCola(Cola_TX,inst1,5);	//Envio 5 datos por TX del string inst1

	xTaskCreate(vTaskLeerAnillo, (char *) "vTaskLeerAnillo",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskCargarAnillo, (char *) "vTaskCargarAnillo",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskUART1Config, (char *) "xTaskUART1Config",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Nunca debería arribar aquí */
    return 0;
}
