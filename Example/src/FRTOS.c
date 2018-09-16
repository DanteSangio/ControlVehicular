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
#define UART_SELECTION 	LPC_UART1
#define IRQ_SELECTION 	UART1_IRQn
#define HANDLER_NAME 	UART1_IRQHandler
#define TXD1	0,15	//TX UART1	(Pin 13 LPC1769)
#define	RXD1	0,16	//RX UART1	(Pin 14 LPC1769)

#define UART_SRB_SIZE 32	//S:Send - Transmit ring buffer size
#define UART_RRB_SIZE 1024	//R:Receive - Receive ring buffer size

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

STATIC RINGBUFF_T txring, rxring;								//Transmit and receive ring buffers
static uint8_t rxbuff[UART_RRB_SIZE], txbuff[UART_SRB_SIZE];	//Transmit and receive buffers

volatile int HourGPS, MinuteGPS, DayGPS, MonthGPS, YearGPS;		//GPS: Variables que guardan informacion
volatile float LatGPS, LongGPS, Lat1GPS;						//GPS: Variables que guardan informacion
volatile float Lat1GPS, Lat2GPS, Long1GPS, Long2GPS;			//GPS: Variables auxiliares

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
	DEBUGOUT("Configurando pines I/O..\n");	//Imprimo en la consola
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


void UART1_IRQHandler(void)
{
	BaseType_t Testigo=pdFALSE;

	/* Handle transmit interrupt if enabled */
	if (LPC_UART1->IER & UART_IER_THREINT)
	{
		Chip_UART_TXIntHandlerRB(LPC_UART1, &txring);

		/* Disable transmit interrupt if the ring buffer is empty */
		if (RingBuffer_IsEmpty(&txring))
		{
			Chip_UART_IntDisable(LPC_UART1, UART_IER_THREINT);
		}
	}

	/* Handle receive interrupt */
	/* New data will be ignored if data not popped in time */
	if (LPC_UART1->IIR & UART_IIR_INTID_RDA )
	{
		while (Chip_UART_ReadLineStatus(LPC_UART1) & UART_LSR_RDR)
		{
			uint8_t ch = Chip_UART_ReadByte(LPC_UART1);
			RingBuffer_Insert(&rxring, &ch);
			xSemaphoreGiveFromISR(Semaforo_RX, &Testigo);	//Devuelve si una de las tareas bloqueadas tiene mayor prioridad que la actual
		}
		portYIELD_FROM_ISR(Testigo);					//Si testigo es TRUE -> ejecuta el scheduler
	}
}


static void xTaskUART1Config(void *pvParameters)
{
	DEBUGOUT("Configurando la UART1..\n");	//Imprimo en la consola

	/* Setup UART for 9600 */
	Chip_UART_Init(LPC_UART1);
	Chip_UART_SetBaud(LPC_UART1, 9600);
	Chip_UART_ConfigData(LPC_UART1, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_UART1, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_UART1);

	/* Before using the ring buffers, initialize them using the ring buffer init function */
	RingBuffer_Init(&rxring, rxbuff, 1, UART_RRB_SIZE);
	RingBuffer_Init(&txring, txbuff, 1, UART_SRB_SIZE);

	/* Reset and enable FIFOs, FIFO trigger level 3 (14 chars) */
	Chip_UART_SetupFIFOS(LPC_UART1, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
							UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(LPC_UART1, (UART_IER_RBRINT | UART_IER_RLSINT));

	//Habilito interrupcion UART1
	NVIC_EnableIRQ(UART1_IRQn);

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
		Chip_UART_SendRB(LPC_UART1, &txring, &Receive , 1);
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
//Analiza la trama GPS y guarda los datos deseados en las variables globales de GPS
void AnalizarTramaGPS (uint8_t dato)
{
	static int i=0;
	static int EstadoTrama=0;
	static char Trama[100];
	static char HourMinute[4];
	static char Date[6];
	static char Lat1[4];
	static char Lat2[5];
	static char Long1[4];
	static char Long2[5];

	int Aux;
	float Aux1;
	float Aux2;
	bool flagFecha=OFF;

	if(dato=='$')		//Inicio de la trama
	{
		EstadoTrama=0;
	}
	switch(EstadoTrama)
	{
		case INICIO_TRAMA:		//INICIO_TRAMA
			EstadoTrama=1;
			memset(Trama,0,100);
			i=0;
		break;

		case CHEQUEO_FIN:		//CHEQUEO_FIN
			Trama[i]=dato;
			i++;
			if(dato=='*')
			{
				EstadoTrama=2;
			}
		break;

		case CHEQUEO_TRAMA:		//CHEQUEO_TRAMA
			if(Trama[16]=='A' && Trama[29]=='S' && Trama[43]=='W')
			{
				EstadoTrama=3;	//Trama correcta
			}
		break;

		case TRAMA_CORRECTA:		//TRAMA_CORRECTA
			DEBUGOUT("%s",Trama);
			DEBUGOUT("\n");
			EstadoTrama=4;
		break;

		case HORA_FECHA:		//HORA Y FECHA
			//Hora
			for(i=6;i<=9;i++)
			{
				HourMinute[i-6]=Trama[i];
			}
			HourMinute[4]='\0';
			HourGPS=atoi(HourMinute);
			MinuteGPS=HourGPS%100;
			HourGPS=HourGPS/100;
			if(HourGPS>=0 && HourGPS<=2)
			{
				HourGPS=HourGPS+21;
				flagFecha=ON;		//flag correccion de fecha
			}
			else
			{
				HourGPS=HourGPS-3;
				flagFecha=OFF;
			}
			DEBUGOUT("%.2d:%.2d\t",HourGPS,MinuteGPS);

			//Fecha
			for(i=52;i<=57;i++)
			{
				Date[i-52]=Trama[i];
			}
			Date[6]='\0';
			if(flagFecha==ON)
			{
				flagFecha=OFF;
				DayGPS=atoi(Date-1);
			}
			else
			{
				DayGPS=atoi(Date);
			}
			YearGPS=DayGPS%100;
			MonthGPS=(DayGPS/100)%100;
			DayGPS=DayGPS/10000;
			DEBUGOUT("%.2d/%.2d/%.2d\n",DayGPS,MonthGPS,YearGPS);
			EstadoTrama=5;
		break;

		case LAT_LONG: 	//LATITUD Y LONGITUD -> DD = d + (min/60) + (sec/3600)
			//Latitud
			for(i=18;i<=21;i++)
			{
				Lat1[i-18]=Trama[i];
			}
			Lat1[4]='\0';
			for(i=23;i<=27;i++)
			{
				Lat2[i-23]=Trama[i];
			}
			Lat2[5]='\0';
			Aux=atoi(Lat1);
			Lat1GPS=Aux/10000000;
			Lat2GPS=(Aux/100000)%100;
			Aux1=(Aux%100000);
			Aux2=(Aux1/100000);
			Lat2GPS=Lat2GPS+Aux2;
			Lat1GPS=Lat1GPS+(Lat2GPS/60);
			LatGPS=-Lat1GPS;
			//DEBUGOUT("%f\t",LatGPS);  	//-> Tira HardFault si descomento esta linea

			//Longitud
			for(i=32;i<=35;i++)
			{
				Long1[i-32]=Trama[i];
			}
			Long1[4]='\0';
			for(i=37;i<=41;i++)
			{
				Long2[i-37]=Trama[i];
			}
			Long2[5]='\0';
			Aux=atoi(Long1);
			Long1GPS=Aux/10000000;
			Long2GPS=(Aux/100000)%100;
			Aux1=(Aux%100000);
			Aux2=(Aux1/100000);
			Long2GPS=Long2GPS+Aux2;
			Long1GPS=Long1GPS+(Long2GPS/60);
			LongGPS=-Long1GPS;
			//DEBUGOUT("%f\n",LongGPS);	//-> Tira HardFault si descomento esta linea

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
	DEBUGOUT("Inicializando prueba GPS..\n");	//Imprimo en la consola

	uC_StartUp();

	SystemCoreClockUpdate();

	vSemaphoreCreateBinary(Semaforo_RX);			//Creamos el semaforo

	Cola_RX = xQueueCreate(UART_RRB_SIZE, sizeof(uint8_t));	//Creamos una cola

	Cola_TX = xQueueCreate(UART_SRB_SIZE, sizeof(uint8_t));	//Creamos una cola

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
