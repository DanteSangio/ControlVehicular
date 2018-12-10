/*
===============================================================================
 Name        : FRTOS.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/
#include "chip.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "MFRC522.h"
#include "rfid_utils.h"
#include "string.h"
#include "stdlib.h"
#include "ControlVehicular.h"
#include "GPS.h"
#include "UART.h"
#include "RFID.h"
#include "GSM.h"
#include <cr_section_macros.h>
#include "sdcard.h"
#include "fat32.h"




#define DEBUGOUT1(...) printf(__VA_ARGS__)
#define DEBUGOUT(...) printf(__VA_ARGS__)
#define DEBUGSTR(...) printf(__VA_ARGS__)

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

static volatile bool fIntervalReached;
static volatile bool fAlarmTimeMatched;
static volatile bool On0, On1;

// RFID structs
MFRC522Ptr_t mfrcInstance;

int last_balance = 0;
unsigned int last_user_ID;




typedef struct
{
	SemaphoreHandle_t 	Rx;
	RINGBUFF_T 	anillo;
	SemaphoreHandle_t 	cola;
	LPC_USART_T			*uart;
}ANILLO;

ANILLO anillo_struct;

struct Datos_Nube TramaEnvio;



/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

SemaphoreHandle_t Semaforo_RFID;
SemaphoreHandle_t Semaforo_RTC;
SemaphoreHandle_t Semaforo_RX1;
SemaphoreHandle_t Semaforo_RX2;
SemaphoreHandle_t Semaforo_GSM_Closed;
SemaphoreHandle_t Semaforo_Inic_SD;
SemaphoreHandle_t Semaforo_Inic_SD2;
SemaphoreHandle_t Semaforo_SSP;

QueueHandle_t Cola_RX1,Cola_RX2;
QueueHandle_t Cola_TX1,Cola_TX2;
QueueHandle_t Cola_Pulsadores;
QueueHandle_t Cola_Connect;
QueueHandle_t Cola_SD;
QueueHandle_t Cola_Datos_GPS;
QueueHandle_t Cola_Datos_RFID;



RINGBUFF_T txring1,txring2, rxring1,rxring2;								//Transmit and receive ring buffers
static uint8_t rxbuff1[UART_RRB_SIZE], txbuff1[UART_SRB_SIZE],rxbuff2[UART_RRB_SIZE], txbuff2[UART_SRB_SIZE];	//Transmit and receive buffers

//volatile int HourGPS, MinuteGPS, DayGPS, MonthGPS, YearGPS;		//GPS: Variables que guardan informacion
//volatile float LatGPS, LongGPS;									//GPS: Variables que guardan informacion
//volatile float Lat1GPS, Lat2GPS, Long1GPS, Long2GPS;			//GPS: Variables auxiliares



/*****************************************************************************
 * Private functions
 ****************************************************************************/
/* Gets and shows the current time and date */
static void showTime(RTC_TIME_T *pTime)
{
	DEBUGOUT("Time: %.2d:%.2d %.2d/%.2d/%.4d\r\n",
			 pTime->time[RTC_TIMETYPE_HOUR],
			 pTime->time[RTC_TIMETYPE_MINUTE],
			 pTime->time[RTC_TIMETYPE_DAYOFMONTH],
			 pTime->time[RTC_TIMETYPE_MONTH],
			 pTime->time[RTC_TIMETYPE_YEAR]);
}


/*******************************************************************************
 *  System routine functions
 ******************************************************************************/



void RTC_IRQHandler(void)
{

	BaseType_t Testigo=pdFALSE;

	/* Interrupcion cada 1 minuto */
	if (Chip_RTC_GetIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE)) {
		/* Clear pending interrupt */
		Chip_RTC_ClearIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE);
		xSemaphoreGiveFromISR(Semaforo_RTC, &Testigo);	//Devuelve si una de las tareas bloqueadas tiene mayor prioridad que la actual
		portYIELD_FROM_ISR(Testigo);					//Si testigo es TRUE -> ejecuta el scheduler

	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void UART1_IRQHandler(void)
{
	BaseType_t Testigo=pdFALSE;

	/* Handle transmit interrupt if enabled */
	if (LPC_UART1->IER & UART_IER_THREINT)
	{
		Chip_UART_TXIntHandlerRB(LPC_UART1, &txring1);

		/* Disable transmit interrupt if the ring buffer is empty */
		if (RingBuffer_IsEmpty(&txring1))
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
			RingBuffer_Insert(&rxring1, &ch);
			xSemaphoreGiveFromISR(Semaforo_RX1, &Testigo);	//Devuelve si una de las tareas bloqueadas tiene mayor prioridad que la actual
		}
		portYIELD_FROM_ISR(Testigo);					//Si testigo es TRUE -> ejecuta el scheduler
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* UART2_IRQHandler */
void UART2_IRQHandler(void)
{
	BaseType_t Testigo=pdFALSE;

	/* Handle transmit interrupt if enabled */
	if (LPC_UART2->IER & UART_IER_THREINT)
	{
		Chip_UART_TXIntHandlerRB(LPC_UART2, &txring2);

		/* Disable transmit interrupt if the ring buffer is empty */
		if (RingBuffer_IsEmpty(&txring2))
		{
			Chip_UART_IntDisable(LPC_UART2, UART_IER_THREINT);
		}
	}

	/* Handle receive interrupt */
	/* New data will be ignored if data not popped in time */
	if (LPC_UART2->IIR & UART_IIR_INTID_RDA )
	{
		while (Chip_UART_ReadLineStatus(LPC_UART2) & UART_LSR_RDR)
		{
			uint8_t ch = Chip_UART_ReadByte(LPC_UART2);
			RingBuffer_Insert(&rxring2, &ch);
			xSemaphoreGiveFromISR(Semaforo_RX2, &Testigo);	//Devuelve si una de las tareas bloqueadas tiene mayor prioridad que la actual
		}
		portYIELD_FROM_ISR(Testigo);					//Si testigo es TRUE -> ejecuta el scheduler
	}
}


/*****************************************************************************
 * Public functions
 ****************************************************************************/


static void xTaskRFIDConfig(void *pvParameters)
{
	//SystemCoreClockUpdate();

	//vTaskDelay(5000/portTICK_RATE_MS);

	DEBUGOUT1("PRUEBA RFID..\n");	//Imprimo en la consola

	setupRFID(&mfrcInstance);

	Chip_GPIO_SetPinOutHigh(LPC_GPIO, RFID_SS);

	xSemaphoreGive(Semaforo_SSP);//Doy el semaforo para que se inicie la lectura

	vTaskDelete(NULL);	//Borra la tarea
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskRTConfig */
static void xTaskRTConfig(void *pvParameters)
{
	RTC_TIME_T FullTime;

	SystemCoreClockUpdate();

	DEBUGOUT("PRUEBA RTC..\n");	//Imprimo en la consola

	Chip_RTC_Init(LPC_RTC);

	/* Set current time for RTC 2:00:00PM, 2012-10-05 */


	FullTime.time[RTC_TIMETYPE_SECOND]  = 0;
	FullTime.time[RTC_TIMETYPE_MINUTE]  = 25;
	FullTime.time[RTC_TIMETYPE_HOUR]    = 21;
	FullTime.time[RTC_TIMETYPE_DAYOFMONTH]  = 05;
	FullTime.time[RTC_TIMETYPE_DAYOFWEEK]   = 4;
	FullTime.time[RTC_TIMETYPE_DAYOFYEAR]   = 207;
	FullTime.time[RTC_TIMETYPE_MONTH]   = 10;
	FullTime.time[RTC_TIMETYPE_YEAR]    = 2018;

	Chip_RTC_SetFullTime(LPC_RTC, &FullTime);
	//

	/* Set the RTC to generate an interrupt on each minute */
	Chip_RTC_CntIncrIntConfig(LPC_RTC, RTC_AMR_CIIR_IMMIN, ENABLE);

	/* Clear interrupt pending */
	Chip_RTC_ClearIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE);

	/* Enable RTC interrupt in NVIC */
	NVIC_EnableIRQ((IRQn_Type) RTC_IRQn);

	/* Enable RTC (starts increase the tick counter and second counter register) */
	Chip_RTC_Enable(LPC_RTC, ENABLE);

//
	vTaskDelete(NULL);	//Borra la tarea
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskRFID */
static void vTaskRFID(void *pvParameters)
{
	while (1)
	{
		xSemaphoreTake(Semaforo_SSP, portMAX_DELAY); //semaforo de uso de ssp
		// Look for new cards in RFID2
		if (PICC_IsNewCardPresent(mfrcInstance))
		{
			// Select one of the cards
			if (PICC_ReadCardSerial(mfrcInstance))
			{
//				int status = writeCardBalance(mfrcInstance, 100000); // used to recharge the card
				 userTapIn();
			}
		}
		Chip_GPIO_SetPinOutHigh(LPC_GPIO, RFID_SS);
		xSemaphoreGive(Semaforo_SSP);
		vTaskDelay( 500 / portTICK_PERIOD_MS );//Muestreo cada 1 seg
	}

	vTaskDelete(NULL);	//Borra la tarea si sale del while
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskRTC */
static void vTaskRTC(void *pvParameters)
{
	RTC_TIME_T FullTime;
	while (1)
	{
		xSemaphoreTake(Semaforo_RTC, portMAX_DELAY);
		Chip_RTC_GetFullTime(LPC_RTC, &FullTime);
		showTime(&FullTime);
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////

static void xTaskUART1Config(void *pvParameters)
{
	DEBUGOUT("Configurando la UART1..\n");	//Imprimo en la consola

	//Inicializacion de los pines de la UART1
	Chip_GPIO_SetDir (LPC_GPIO, RXD1, INPUT);
	Chip_IOCON_PinMux (LPC_IOCON, RXD1, IOCON_MODE_INACT, IOCON_FUNC1);
	Chip_GPIO_SetDir (LPC_GPIO, TXD1, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, TXD1, IOCON_MODE_INACT, IOCON_FUNC1);

	/* Setup UART for 9600 */
	Chip_UART_Init(LPC_UART1);
	Chip_UART_SetBaud(LPC_UART1, 9600);
	Chip_UART_ConfigData(LPC_UART1, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_UART1, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_UART1);

	/* Before using the ring buffers, initialize them using the ring buffer init function */
	RingBuffer_Init(&rxring1, rxbuff1, 1, UART_RRB_SIZE);
	RingBuffer_Init(&txring1, txbuff1, 1, UART_SRB_SIZE);

	/* Reset and enable FIFOs, FIFO trigger level 3 (14 chars) */
	Chip_UART_SetupFIFOS(LPC_UART1, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
							UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

	vTaskDelay(5000/portTICK_RATE_MS);

	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(LPC_UART1, (UART_IER_RBRINT | UART_IER_RLSINT));

	//Habilito interrupcion UART1
	NVIC_EnableIRQ(UART1_IRQn);

	vTaskDelete(NULL);	//Borra la tarea
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* xTaskUART2Config */
static void xTaskUART2Config(void *pvParameters)
{
	DEBUGOUT("Configurando la UART2..\n");	//Imprimo en la consola

	//Inicializacion de los pines del GSM
	Chip_GPIO_SetDir (LPC_GPIO, RXD2, INPUT);
	Chip_IOCON_PinMux (LPC_IOCON, RXD2, IOCON_MODE_INACT, IOCON_FUNC1);
	Chip_GPIO_SetDir (LPC_GPIO, TXD2, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, TXD2, IOCON_MODE_INACT, IOCON_FUNC1);


	/* Setup UART for 9600 */
	Chip_UART_Init(LPC_UART2);
	Chip_UART_SetBaud(LPC_UART2, 9600);
	Chip_UART_ConfigData(LPC_UART2, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_UART2, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_UART2);

	/* Before using the ring buffers, initialize them using the ring buffer init function */
	RingBuffer_Init(&rxring2, rxbuff2, 1, UART_RRB_SIZE);
	RingBuffer_Init(&txring2, txbuff2, 1, UART_SRB_SIZE);

	/* Reset and enable FIFOs, FIFO trigger level 3 (14 chars) */
	Chip_UART_SetupFIFOS(LPC_UART2, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
							UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(LPC_UART2, (UART_IER_RBRINT | UART_IER_RLSINT));

	//Habilito interrupcion UART1
	NVIC_EnableIRQ(UART2_IRQn);

	vTaskDelete(NULL);	//Borra la tarea
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskGSMConfig */
static void vTaskGSMConfig(void *pvParameters)
{
	//Inicializacion del reset del GSM
	//Chip_GPIO_SetDir (LPC_GPIO, RST_GSM, INPUT);
	//Chip_IOCON_PinMux (LPC_IOCON, RST_GSM, IOCON_MODE_INACT, IOCON_FUNC0);

	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms

	vTaskDelete(NULL);	//Borra la tarea
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskCargarAnillo */
//Encargada de cargar el anillo a partir de la cola
static void vTaskCargarAnillo1(void *pvParameters)
{
	uint8_t Receive=0;
	while (1)
	{
		xQueueReceive(Cola_TX1, &Receive, portMAX_DELAY);
		Chip_UART_SendRB(LPC_UART1, &txring1, &Receive , 1);
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskLeerAnillo */
//Pasa la informacion del anillo a la cola de recepcion
static void vTaskLeerAnillo1(void *pvParameters)
{
	uint8_t Receive=0;
	uint8_t Testigo=0, dato=0;

	while (1)
	{
		xSemaphoreTake(Semaforo_RX1, portMAX_DELAY);
		Testigo = RingBuffer_Pop(&rxring1, &Receive);
		if(Testigo)
		{
			xSemaphoreGive(Semaforo_RX1);
			xQueueSendToBack(Cola_RX1, &Receive, portMAX_DELAY);
		}
		/*
		//leo la cola de rercepcion y lo muestro en pantalla
		if(LeerCola(Cola_RX1,&dato,1))
		{
			AnalizarTramaGSM(dato);
			//DEBUGOUT("%c", dato);	//Imprimo en la consola
		}
		*/
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskCargarAnillo */
//Encargada de cargar el anillo a partir de la cola
static void vTaskCargarAnillo2(void *pvParameters)
{
	uint8_t Receive=0;
	while (1)
	{
		xQueueReceive(Cola_TX2, &Receive, portMAX_DELAY);
		Chip_UART_SendRB(LPC_UART2, &txring2, &Receive, 1);
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskLeerAnillo */
//Pasa la informacion del anillo a la cola de recepcion
static void vTaskLeerAnillo2(void *pvParameters)
{
	uint8_t Receive=0;
	uint8_t Testigo=0;

	while (1)
	{
		xSemaphoreTake(Semaforo_RX2, portMAX_DELAY);
		Testigo = RingBuffer_Pop(&rxring2, &Receive);
		if(Testigo)
		{
			xSemaphoreGive(Semaforo_RX2);
			xQueueSendToBack(Cola_RX2, &Receive, portMAX_DELAY);
		}
		/*
		//leo la cola de rercepcion y lo muestro en pantalla
		if(LeerCola(Cola_RX2,&dato,1))
		{
			AnalizarTramaGPS(dato);
			//DEBUGOUT("%c", dato);	//Imprimo en la consola
		}
		*/
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskAnalizarGSM */
static void vTaskAnalizarGSM(void *pvParameters)
{
	uint8_t dato=0;

	while (1)
	{
		//leo la cola de rercepcion y lo muestro en pantalla
		if(LeerCola(Cola_RX1,&dato,1))
		{
			AnalizarTramaGSM(dato);
			//DEBUGOUT("%c", dato);	//Imprimo en la consola
		}
	}
	vTaskDelete(NULL);	//Borra la tarea
}




///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskAnalizarGPS */
static void vTaskAnalizarGPS(void *pvParameters)
{
	uint8_t dato=0;
	uint8_t i;

	while (1)
	{
		for(i=0;i<5;i++)
		{
			xQueueReset(RX_COLA_GPS);
			vTaskDelay(1000/portTICK_RATE_MS); //1seg
		}

		while(LeerCola(RX_COLA_GPS,&dato,1))
		{
			AnalizarTramaGPS(dato); // que devuelva una struct que posea distintos campos: lat,long,hora,fecha y vel
			//DEBUGOUT("%c", dato);	//Imprimo en la consola
			//para pasar la informacion colocar una cola de una sola posicion e ir sobreescribiendola
		}
	}
	vTaskDelete(NULL);	//Borra la tarea
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskEnviarGSM */
static void vTaskEnviarGSM(void *pvParameters)
{
	uint8_t Receive=OFF;
	struct Datos_Nube informacion;
	unsigned int informacionRFID;

	while(1)
	{
		/*
		xQueueReceive(Cola_Pulsadores, &Receive, portMAX_DELAY);

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
		*/


		//Para enviar datos por GPRS a ThingSpeak

		xQueuePeek(Cola_Datos_GPS, &informacion, portMAX_DELAY);
		xQueuePeek(Cola_Datos_RFID, &informacionRFID, portMAX_DELAY);
		//informacion.latitud[0] = 'A';
		EnviarTramaGSM(informacion.latitud,informacion.longitud, informacionRFID);

		//}
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
		//xQueuePeek(Cola_Datos_GPS, &dato25, portMAX_DELAY);			//Para chequear si dio un error

		/*
		PRUEBA BUZZER SONANDO CADA 100ms
		Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
		vTaskDelay(100/portTICK_RATE_MS);
		Chip_GPIO_SetPinOutLow(LPC_GPIO, BUZZER);
		vTaskDelay(100/portTICK_RATE_MS);
		Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
		vTaskDelay(100/portTICK_RATE_MS);
		Chip_GPIO_SetPinOutLow(LPC_GPIO, BUZZER);
		*/

		/*if(Chip_GPIO_GetPinState(LPC_GPIO, SW1)==OFF)	//Si se presiona el SW1
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
		}*/
	}
	vTaskDelete(NULL);	//Borra la tarea
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskInicSD */
static void vTaskInicSD(void *pvParameters)
{
    uint8_t returnStatus,sdcardType;

	/* PARA COMPROBAR SI LA SD ESTA CONECTADA */
	xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);
    do //if(returnStatus)
    {
        returnStatus = SD_Init(&sdcardType);
        if(returnStatus == SDCARD_NOT_DETECTED)
        {
        	DEBUGOUT("\n\r SD card not detected..");
        }
        else if(returnStatus == SDCARD_INIT_FAILED)
        {
        	DEBUGOUT("\n\r Card Initialization failed..");
        }
        else if(returnStatus == SDCARD_FAT_INVALID)
        {
        	DEBUGOUT("\n\r Invalid Fat filesystem");
        }
    }while(returnStatus!=0);

    DEBUGOUT("\n\rSD Card Detected!");

	xSemaphoreGive(Semaforo_SSP);

    vTaskDelete(NULL);	//Borra la tarea
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* xTaskWriteSD*/
static void xTaskWriteSD(void *pvParameters)
{
    uint8_t returnStatus,i=0;
    fileConfig_st *srcFilePtr;
    char Receive[100];

    while (1)
	{
    	xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);
        /* PARA ESCRIBIR ARCHIVO */
        do
        {
        	srcFilePtr = FILE_Open("datalog.txt",WRITE,&returnStatus);
        }while(srcFilePtr == 0);

		xQueueReceive(Cola_SD,&Receive,portMAX_DELAY);	//Para recibir los datos a guardar
    	for(i=0;Receive[i];)
		{
		   FILE_PutCh(srcFilePtr,Receive[i++]);
		}
        FILE_PutCh(srcFilePtr,EOF);
        FILE_Close(srcFilePtr);
        for(i=0;Receive[i];i++)//limpio el vector
        {
        	Receive[i]=0;
        }
    	xSemaphoreGive(Semaforo_SSP);
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while 1
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* main
*/
int main (void)
{
	uint8_t aux=0;

	SystemCoreClockUpdate();

	/* Initializes GPIO */
	Chip_GPIO_Init(LPC_GPIO);

	uC_StartUp();

	vSemaphoreCreateBinary(Semaforo_RX1);			//Creamos el semaforo
	vSemaphoreCreateBinary(Semaforo_RX2);			//Creamos el semaforo
	vSemaphoreCreateBinary(Semaforo_RFID);
	xSemaphoreTake(Semaforo_RFID, portMAX_DELAY); 	//semaforo de inicializacion de rfid
	vSemaphoreCreateBinary(Semaforo_RTC);			//Creamos el semaforo
	vSemaphoreCreateBinary(Semaforo_GSM_Closed);			//Creamos el semaforo
	xSemaphoreTake(Semaforo_GSM_Closed, portMAX_DELAY);
	vSemaphoreCreateBinary(Semaforo_SSP);			//Creamos el semaforo
	xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);


	Cola_RX1 = xQueueCreate(UART_RRB_SIZE, sizeof(uint8_t));	//Creamos una cola
	Cola_TX1 = xQueueCreate(UART_SRB_SIZE, sizeof(uint8_t));	//Creamos una cola
	Cola_RX2 = xQueueCreate(UART_RRB_SIZE, sizeof(uint8_t));	//Creamos una cola
	Cola_TX2 = xQueueCreate(UART_SRB_SIZE, sizeof(uint8_t));	//Creamos una cola
	Cola_Pulsadores = xQueueCreate(1, sizeof(uint32_t));		//Creamos una cola
	Cola_Connect = xQueueCreate(1, sizeof(uint8_t));			//Creamos una cola
	xQueueOverwrite(Cola_Connect, &aux);
	Cola_SD = xQueueCreate(4, sizeof(char) * 100);	//Creamos una cola para mandar una trama completa
	Cola_Datos_GPS = xQueueCreate(1, sizeof(struct Datos_Nube));
	Cola_Datos_RFID = xQueueCreate(1, sizeof(unsigned int));

	/*
	xTaskCreate(xTaskWriteSD, (char *) "xTaskWriteSD",
					configMINIMAL_STACK_SIZE * 2, NULL, (tskIDLE_PRIORITY + 2UL),
					(xTaskHandle *) NULL);

	xTaskCreate(vTaskInicSD, (char *) "vTaskInicSD",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);
	*/
	xTaskCreate(vTaskRFID, (char *) "vTaskRFID",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskRFIDConfig, (char *) "xTaskRFIDConfig",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 4UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskRTC, (char *) "vTaskRTC",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskRTConfig, (char *) "xTaskRTConfig",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskLeerAnillo1, (char *) "vTaskLeerAnillo1",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskLeerAnillo2, (char *) "vTaskLeerAnillo2",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskCargarAnillo1, (char *) "vTaskCargarAnillo1",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskCargarAnillo2, (char *) "vTaskCargarAnillo2",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskUART1Config, (char *) "xTaskUART1Config",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskUART2Config, (char *) "xTaskUART2Config",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskGSMConfig, (char *) "vTaskGSMConfig",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskEnviarGSM, (char *) "vTaskEnviarGSM",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskPulsadores, (char *) "vTaskPulsadores",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskAnalizarGPS, (char *) "vTaskAnalizarGPS",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskAnalizarGSM, (char *) "vTaskAnalizarGSM",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Nunca debería arribar aquí */
    return 0;
}
