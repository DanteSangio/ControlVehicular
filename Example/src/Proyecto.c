/*
===============================================================================
 Name        : FRTOS.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Includes */
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
#include "GUI.h"
#include "LCD_X_SPI.h"
#include "Pantalla.h"
//#include "DIALOG.h"
//#include "GRAPH.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Defines */
#define DEBUGOUT1(...) //printf(__VA_ARGS__)
#define DEBUGOUT(...)  //printf(__VA_ARGS__)
#define DEBUGSTR(...) //printf(__VA_ARGS__)


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Declaraciones globales */
static volatile bool fIntervalReached;
static volatile bool fAlarmTimeMatched;
static volatile bool On0, On1;

MFRC522Ptr_t mfrcInstance;	// RFID structs

int last_balance = 0;
unsigned int last_user_ID;

uint8_t	ReceivePulsadores;
uint8_t j=0;
uint8_t Flag10sPantalla=OFF;

RINGBUFF_T txring1,txring2, rxring1,rxring2;					//Transmit and receive ring buffers
static uint8_t 	rxbuff1[UART_RRB_SIZE], txbuff1[UART_SRB_SIZE],
				rxbuff2[UART_RRB_SIZE], txbuff2[UART_SRB_SIZE];

extern const GUI_BITMAP bmutnlogo; //declare external Bitmap
extern const GUI_BITMAP bmcontrol; //declare external Bitmap


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Semaforos */
SemaphoreHandle_t Semaforo_RFID;
SemaphoreHandle_t Semaforo_RTCgsm;
SemaphoreHandle_t Semaforo_RX1;
SemaphoreHandle_t Semaforo_RX2;
SemaphoreHandle_t Semaforo_GSM_Closed;
SemaphoreHandle_t Semaforo_GSM_Enviado;
SemaphoreHandle_t Semaforo_SSP;
SemaphoreHandle_t Semaforo_RTCsd;
SemaphoreHandle_t Semaforo_YaHayTarj;


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Colas */
QueueHandle_t Cola_RX1,Cola_RX2;
QueueHandle_t Cola_TX1,Cola_TX2;
QueueHandle_t Cola_Pulsadores;
QueueHandle_t Cola_Connect;
QueueHandle_t Cola_SD;
QueueHandle_t Cola_Datos_GPS;
QueueHandle_t Cola_Datos_RFID;
QueueHandle_t Cola_Inicio_Tarjetas;
QueueHandle_t HoraEntrada;


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* showTime
 * Gets and shows the current time and date */
static void showTime(RTC_TIME_T *pTime)
{
	DEBUGOUT("Time: %.2d:%.2d %.2d/%.2d/%.4d\r\n",
			 pTime->time[RTC_TIMETYPE_HOUR],
			 pTime->time[RTC_TIMETYPE_MINUTE],
			 pTime->time[RTC_TIMETYPE_DAYOFMONTH],
			 pTime->time[RTC_TIMETYPE_MONTH],
			 pTime->time[RTC_TIMETYPE_YEAR]);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* RTC_IRQHandler */
void RTC_IRQHandler(void)
{
	BaseType_t Testigo=pdFALSE;
	static uint8_t i=0;

	/* Interrupcion cada 1 seg */
	if (Chip_RTC_GetIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE))
	{
		/* Clear pending interrupt */
		Chip_RTC_ClearIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE);
		if(Flag10sPantalla==ON)
		{
			j++;
			if(j==10)
			{
				Flag10sPantalla=OFF;
				j=0;
			}
		}
		else
		{
			j=0;
		}
		i++;
		if(i==30)
		{
			i=0;
			xSemaphoreGiveFromISR(Semaforo_RTCgsm, &Testigo);	//Devuelve si una de las tareas bloqueadas tiene mayor prioridad que la actual
			portYIELD_FROM_ISR(Testigo);					//Si testigo es TRUE -> ejecuta el scheduler
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* UART1_IRQHandler */
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


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* xTaskRFIDConfig */
static void xTaskRFIDConfig(void *pvParameters)
{
	//SystemCoreClockUpdate();

	//vTaskDelay(5000/portTICK_RATE_MS);

	xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);

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
	struct Datos_Nube informacion;
	uint16_t	aux;
	char pepe[4];

	SystemCoreClockUpdate();

	//DEBUGOUT("PRUEBA RTC..\n");	//Imprimo en la consola

	Chip_RTC_Init(LPC_RTC);

	/* Seteo el tiempo con la hora brindada por el GPS */

	xQueuePeek(Cola_Datos_GPS, &informacion, portMAX_DELAY);

	FullTime.time[RTC_TIMETYPE_SECOND]  = 0;
	pepe[0] = informacion.hora[3]; pepe[1] = informacion.hora[4]; pepe[2] =0;
	aux = atoi (pepe);
	FullTime.time[RTC_TIMETYPE_MINUTE]  = aux;
	pepe[0] = informacion.hora[0]; pepe[1] = informacion.hora[1]; pepe[2] =0;
	aux = atoi (pepe);
	FullTime.time[RTC_TIMETYPE_HOUR]    = aux;
	pepe[0] = informacion.fecha[0]; pepe[1] = informacion.fecha[1]; pepe[2] =0;
	aux = atoi (pepe);
	FullTime.time[RTC_TIMETYPE_DAYOFMONTH]  = aux;
	FullTime.time[RTC_TIMETYPE_DAYOFWEEK]   = 4;
	FullTime.time[RTC_TIMETYPE_DAYOFYEAR]   = 207;
	pepe[0] = informacion.fecha[3]; pepe[1] = 0; pepe[2] =0;
	aux = atoi (pepe);
	FullTime.time[RTC_TIMETYPE_MONTH]   = aux;
	pepe[0] = informacion.fecha[5]; pepe[1] = informacion.fecha[6]; pepe[2] =0;
	aux = atoi (pepe);
	aux = aux + 2000;
	FullTime.time[RTC_TIMETYPE_YEAR]= aux;

	Chip_RTC_SetFullTime(LPC_RTC, &FullTime);
	//

	/* Set the RTC to generate an interrupt on each second */
	Chip_RTC_CntIncrIntConfig(LPC_RTC, RTC_AMR_CIIR_IMSEC, ENABLE);

	/* Clear interrupt pending */
	Chip_RTC_ClearIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE);

	/* Enable RTC interrupt in NVIC */
	NVIC_EnableIRQ((IRQn_Type) RTC_IRQn);

	/* Enable RTC (starts increase the tick counter and second counter register) */
	Chip_RTC_Enable(LPC_RTC, ENABLE);

	vTaskDelete(NULL);	//Borra la tarea
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* xTaskUART1Config */
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
	uint8_t Testigo=0;

	while (1)
	{
		xSemaphoreTake(Semaforo_RX1, portMAX_DELAY);
		Testigo = RingBuffer_Pop(&rxring1, &Receive);
		if(Testigo)
		{
			xSemaphoreGive(Semaforo_RX1);
			xQueueSendToBack(Cola_RX1, &Receive, portMAX_DELAY);
		}
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
/* vTaskRFID */
static void vTaskRFID(void *pvParameters)
{
	static uint8_t FlagBorrar=0;
	struct Datos_Nube informacion;
	Entrada_RFID Entrada;
	char pepe[4];

	//xSemaphoreTake(Semaforo_GSM_Enviado,portMAX_DELAY);//me aseguro que ya se hayan cargado las tarjetas
	//xSemaphoreGive(Semaforo_GSM_Enviado);
	while (1)
	{
		xSemaphoreTake(Semaforo_YaHayTarj, portMAX_DELAY);
		xSemaphoreGive(Semaforo_YaHayTarj);
		xSemaphoreTake(Semaforo_SSP, portMAX_DELAY); //semaforo de uso de ssp
		// Look for new cards in RFID
		if (PICC_IsNewCardPresent(mfrcInstance))
		{
			// Select one of the cards
			if (PICC_ReadCardSerial(mfrcInstance))
			{
				//int status = writeCardBalance(mfrcInstance, 100000); // used to recharge the card
				userTapIn();
				Chip_GPIO_SetPinOutHigh(LPC_GPIO, RFID_SS);

				/*
				//Grabo hora de entrada
				xQueuePeek(Cola_Datos_GPS, &informacion, portMAX_DELAY);

				pepe[0] = informacion.hora[3]; pepe[1] = informacion.hora[4]; pepe[2] =0;
				Entrada.minutos = atoi (pepe);
				pepe[0] = informacion.hora[0]; pepe[1] = informacion.hora[1]; pepe[2] =0;
				Entrada.hora = atoi (pepe);
				xQueueOverwrite(HoraEntrada,&Entrada);
				*/
			}
		}
		Chip_GPIO_SetPinOutHigh(LPC_GPIO, RFID_SS);
		xSemaphoreGive(Semaforo_SSP);
		vTaskDelay( 500 / portTICK_PERIOD_MS );//Muestreo cada medio seg
	}

	vTaskDelete(NULL);	//Borra la tarea si sale del while
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
			AnalizarTramaGPS(dato); // pasa la struct en una cola dentro de la funcion
			//DEBUGOUT("%c", dato);	//Imprimo en la consola
		}
	}
	vTaskDelete(NULL);	//Borra la tarea
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskEnviarGSM */
static void vTaskEnviarGSM(void *pvParameters)
{
	struct Datos_Nube informacion;
	Tarjetas_RFID informacionRFID;

	xSemaphoreTake(Semaforo_GSM_Enviado,portMAX_DELAY);//me aseguro que ya se hayan cargado las tarjetas
	xSemaphoreGive(Semaforo_GSM_Enviado);
	while(1)
	{

		/*
		xQueueReceive(Cola_Pulsadores, &Receive, portMAX_DELAY);
		Faltaria el semaforo de los pulsadores o algo que tome una decision
		//Para enviar SMS
		EnviarMensajeGSM();
		*/

		//Para enviar datos por GPRS a ThingSpeak
		xSemaphoreTake(Semaforo_RTCgsm,portMAX_DELAY);//hago que envie cada medio min la informacion
		xQueuePeek(Cola_Datos_GPS, &informacion, portMAX_DELAY);
		xQueuePeek(Cola_Datos_RFID, &informacionRFID, portMAX_DELAY);
		EnviarTramaGSM(informacion.latitud,informacion.longitud, informacionRFID.tarjeta, informacion.velocidad);
	}
	vTaskDelete(NULL);	//Borra la tarea
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskTarjetasGSM*/
static void vTaskTarjetasGSM(void *pvParameters)
{
	Tarjetas_RFID* InicioTarjetas=NULL;//Puntero al inicio del vector de tarjetas
	uint8_t dato=0;

	//tarea que llamo al iniciar el programa y luego la borro

    while (InicioTarjetas == NULL)
	{
    	RecibirTramaGSM();
    	vTaskDelay(5000/portTICK_RATE_MS);//espero 10 seg para asegurarme que llego tod o
		while(LeerCola(RX_COLA_GSM,&dato,1))
		{
			InicioTarjetas = AnalizarTramaGSMrecibido(dato);
			//DEBUGOUT("%c", dato);	//Imprimo en la consola
		}
	}

	xQueueOverwrite(Cola_Inicio_Tarjetas,&InicioTarjetas);// envio a una cola el comienzo del vector de tarjetas
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
	vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s
	Chip_GPIO_SetPinOutLow(LPC_GPIO, BUZZER);
	xSemaphoreGive(Semaforo_GSM_Enviado);

	vTaskDelete(NULL);	//Borra la tarea si sale del while 1
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* xTaskPulsadores */
static void xTaskPulsadores(void *pvParameters)
{
	static uint8_t Send=OFF;
	static uint8_t SendAnt=OFF;
	while (1)
	{
		//ReceivePulsadores=0;

		if(ReceivePulsadores == 0)
		{
			if(Chip_GPIO_GetPinState(LPC_GPIO, SW1)==OFF)	//Si se presiona el SW1
			{
				ReceivePulsadores++;	//Envio a la cola que se presiono el SW1
			}
			if(Chip_GPIO_GetPinState(LPC_GPIO, SW2)==OFF)	//Si se presiona el SW2
			{
				ReceivePulsadores=ReceivePulsadores+10;
			}
			while(Chip_GPIO_GetPinState(LPC_GPIO, SW2)==OFF || Chip_GPIO_GetPinState(LPC_GPIO, SW1)==OFF)
			{				vTaskDelay(1000/portTICK_RATE_MS);}
		}
		/*
		if(SendAnt==Send)
		{
			//xQueueOverwrite(Cola_Pulsadores, &Send);
		}
		SendAnt=Send;
		*/
		vTaskDelay(500/portTICK_RATE_MS);	//Espero 1s
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
    fileConfig_st *srcFilePtr = NULL;
    char Receive[100];

    while (1)
	{

        xSemaphoreTake(Semaforo_RTCsd, portMAX_DELAY);//grabo cada medio seg
    	xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);// me fijo que este disponible el canal ssp
        /* PARA ESCRIBIR ARCHIVO */
        do
        {
        	srcFilePtr = FILE_Open("datalog.txt",WRITE,&returnStatus);
        }while(srcFilePtr == 0);

		//xQueuePeek(Cola_SD,&Receive,portMAX_DELAY);	//Para recibir los datos a guardar
        InfoSd(Receive);


		for(i=0;Receive[i];)
		{
		   FILE_PutCh(srcFilePtr,Receive[i++]);
		}
        FILE_PutCh(srcFilePtr,EOF);
        FILE_Close(srcFilePtr);
        for(i=0;Receive[i] != 0 || i<100;i++)//limpio el vector
        {
        	Receive[i]=0;
        }
    	xSemaphoreGive(Semaforo_SSP);
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while 1
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskTFT */
void vTaskTFT(void *pvParameters)
{
	static uint8_t EstadoPantalla=7;
	static uint8_t FlagEstado=ON;
	uint8_t ReceiveRFID=OFF;
	struct Datos_Nube	ReceiveGPS;
	uint32_t dia, mes, ano, hora, minutos, aux;
	static uint8_t minConductor=0;
	static uint8_t horaConductor=0;
	RTC_TIME_T pFullTime;

	/*
	uint8_t op,menu,gananterior; //menu=EST_MEDICION
	static uint8_t PC_config=0; // empieza en el menu de medicion
	char cadena[16];

	GUI_COLOR colorfondoboton;
	BUTTON_Handle botona,botonb,botonc,botond,botone;
	GRAPH_Handle grafico;
	GRAPH_SCALE_Handle escalagrafh,escalagrafv;
	GRAPH_DATA_Handle datagraf;
	*/

	xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);
	GUI_Init();
	/*
	GUI_Clear();
	GUI_DrawBitmap(&bmutnlogo, 0, 0);

	vTaskDelay(1000/portTICK_PERIOD_MS);

	GUI_Clear();
	GUI_DrawBitmap(&bmcontrol, 0, 0);
	 */

	GUI_SetBkColor(0x00000000); //gris claro 0x00D3D3D3
	xSemaphoreGive(Semaforo_SSP);

	/*
	BUTTON_SetDefaultFont(GUI_FONT_COMIC18B_ASCII);
	BUTTON_SetDefaultTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
	GUI_SetFont(GUI_FONT_COMIC18B_ASCII);
	botona=BUTTON_CreateEx(5,145,150,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON0);
	//BUTTON_SetTextAling(botona,GUI_TA_HCENTER | GUI_TA_VCENTER);
	BUTTON_SetText(botona, "FAST/SLOW");
	botonb=BUTTON_CreateEx(165,145,150,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON1);
	BUTTON_SetText(botonb, "A/C");
	//GUI_SetTextAling(botona,GUI_TA_HCENTER | GUI_TA_VCENTER);
	//GUI_SetFont(botona,GUI_FONT_COMIC18B_ASCII);
	botonc=BUTTON_CreateEx(5,195,150,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON2);
	BUTTON_SetText(botonc, "GRAFICO");
	botond=BUTTON_CreateEx(165,195,150,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON3);
	BUTTON_SetText(botond, "OPCIONES");
	BUTTON_SetBkColor(botona,BUTTON_CI_UNPRESSED,0x00FFFF80);*/
	/*
	GUI_Exec();

	colorfondoboton=BUTTON_GetDefaultBkColor(BUTTON_CI_UNPRESSED);

	//borro pantalla
	//GUI_SetBkColor(0x0080FF80);//Verde
	//GUI_SetBkColor(0x00D3D3D3); //gris claro
	//GUI_Clear();

	grafico=GRAPH_CreateEx(0,5,310,180,0,WM_CF_HIDE,0,GUI_ID_GRAPH0);
	*/

	while(1)
	{
		//xQueueReceive(Cola_Pulsadores, &Receive, 300);
		vTaskDelay(50/portTICK_RATE_MS);

		//xQueuePeek(Cola_Datos_GPS,&ReceiveGPS,portMAX_DELAY);

		Chip_RTC_GetFullTime(LPC_RTC, &pFullTime);
		minutos=pFullTime.time[1];
		hora=pFullTime.time[2];
		dia=pFullTime.time[3];
		mes=pFullTime.time[6];
		ano=pFullTime.time[7];

		switch(EstadoPantalla)
		{
			case 0:		//PANTALLA PRINCIPAL CON USUARIO REGISTRADO
				if(FlagEstado==ON)
				{
					xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);

					GUI_Clear();
					aux=minutos;
					FlagEstado=OFF;
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringAt("/     /",127,10);
					GUI_DispDecAt(dia,101,10,2);
					GUI_DispDecAt(mes,137,10,2);
					GUI_DispDecAt(ano,172,10,4);

					GUI_SetFont(GUI_FONT_D80);
					GUI_DispDecAt(hora,32,50,2);
					GUI_DispDecAt(minutos,165,50,2);
					GUI_SetFont(GUI_FONT_32B_ASCII);
					GUI_DispStringAt(".",155,80);
					GUI_DispStringAt(".",155,55);

					GUI_SetFont(GUI_FONT_32B_ASCII);
					GUI_DispStringHCenterAt("JUAN FERNANDEZ",160,150);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("MENU",80,210);
					GUI_DispStringHCenterAt("S.O.S.",240,210);

					GUI_DrawHLine(195,0,320);
					GUI_DrawHLine(200,0,320);
					xSemaphoreGive(Semaforo_SSP);
				}
				if(ReceivePulsadores==1)		//Paso al estado HORAS DE TRABAJO
				{
					EstadoPantalla=1;
					FlagEstado=ON;
					ReceivePulsadores = 0;
				}
				else if(ReceivePulsadores==10)	//Paso al estado CONFIRMAR SMS
				{
					EstadoPantalla=10;
					FlagEstado=ON;
					ReceivePulsadores = 0;
				}
				else
				{
					ReceivePulsadores = 0;
				}
				if(aux!=minutos)
				{
					FlagEstado=ON;
					minConductor++;
					if(minConductor==60)
					{
						minConductor=0;
						horaConductor++;
					}
				}
			break;

			case 1:		//HORAS DE TRABAJO
				if(FlagEstado==ON)
				{
					xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);

					GUI_Clear();
					j=0;
					FlagEstado=OFF;
					Flag10sPantalla=ON;
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("Juan Fernandez",100,10);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispDecAt(hora,240,10,2);
					GUI_DispStringAt(":",264,10);
					GUI_DispDecAt(minutos,271,10,2);

					GUI_DrawHLine(40,0,320);
					GUI_DrawHLine(45,0,320);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("HORAS DE TRABAJO",160,75);
					GUI_SetFont(GUI_FONT_32B_ASCII);
					GUI_DispStringHCenterAt(":",160,120);
					GUI_SetFont(GUI_FONT_D48);
					GUI_DispDecAt(horaConductor,80,115,2);
					GUI_DispDecAt(minConductor,165,115,2);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("SIGUIENTE",80,210);

					GUI_DrawHLine(195,0,320);
					GUI_DrawHLine(200,0,320);
					xSemaphoreGive(Semaforo_SSP);
				}
				if(ReceivePulsadores==1)		//Paso al estado SMS CONTACTO
				{
					EstadoPantalla=2;
					FlagEstado=ON;
					ReceivePulsadores = 0;
				}
				else
				{
					ReceivePulsadores = 0;
				}
				if(Flag10sPantalla==OFF)
				{
					EstadoPantalla=0;
					FlagEstado=ON;
				}
			break;

			case 2:		//SMS CONTACTO
				if(FlagEstado==ON)
				{
					xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);

					GUI_Clear();
					FlagEstado=OFF;
					j=0;
					Flag10sPantalla=ON;
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("Juan Fernandez",100,10);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispDecAt(hora,240,10,2);
					GUI_DispStringAt(":",264,10);
					GUI_DispDecAt(minutos,271,10,2);

					GUI_DrawHLine(40,0,320);
					GUI_DrawHLine(45,0,320);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("PARA ENVIAR UN SMS A",160,75);
					GUI_DispStringHCenterAt("LA CENTRAL, PRESIONE",160,110);
					GUI_DispStringHCenterAt("CONFIRMAR...",160,145);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("SIGUIENTE",80,210);
					GUI_DispStringHCenterAt("CONFIRMAR",240,210);

					GUI_DrawHLine(195,0,320);
					GUI_DrawHLine(200,0,320);
					xSemaphoreGive(Semaforo_SSP);
				}
				if(ReceivePulsadores==1)		//Paso al estado SALIDA CONDUCTOR
				{
					EstadoPantalla=5;
					FlagEstado=ON;
					ReceivePulsadores = 0;
				}
				else if(ReceivePulsadores==10)	//Paso al estado CONFIRMAR SMS
				{
					EstadoPantalla=10;
					FlagEstado=ON;
					ReceivePulsadores = 0;
				}
				else
				{
					ReceivePulsadores = 0;
				}
				if(Flag10sPantalla==OFF)
				{
					EstadoPantalla=0;
					FlagEstado=ON;
				}
			break;

			case 3:		//
			break;

			case 4:		//
			break;

			case 5:		//SALIDA CONDUCTOR
				if(FlagEstado==ON)
				{
					xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);

					GUI_Clear();
					FlagEstado=OFF;
					j=0;
					Flag10sPantalla=ON;
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("Juan Fernandez",100,10);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispDecAt(hora,240,10,2);
					GUI_DispStringAt(":",264,10);
					GUI_DispDecAt(minutos,271,10,2);

					GUI_DrawHLine(40,0,320);
					GUI_DrawHLine(45,0,320);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt(" PARA CONFIRMAR SALIDA",160,75);
					GUI_DispStringHCenterAt(" DE CONDUCTOR, PRESIONE",160,110);
					GUI_DispStringHCenterAt(" CONFIRMAR... ",160,145);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("SIGUIENTE",80,210);
					GUI_DispStringHCenterAt("CONFIRMAR",240,210);

					GUI_DrawHLine(195,0,320);
					GUI_DrawHLine(200,0,320);
					xSemaphoreGive(Semaforo_SSP);
				}
				if(ReceivePulsadores==1)		//Paso al estado PANTALLA PRINCIPAL
				{
					EstadoPantalla=1;
					FlagEstado=ON;
					ReceivePulsadores = 0;
				}
				else if(ReceivePulsadores==10)		//Paso al estado CONFIRMAR SALIDA
				{
					EstadoPantalla=9;
					FlagEstado=ON;
					ReceivePulsadores = 0;
				}
				else
				{
					ReceivePulsadores = 0;
				}
				if(Flag10sPantalla==OFF)
				{
					EstadoPantalla=0;
					FlagEstado=ON;
				}
			break;

			case 6:		//ENVIAR MENSAJE
				xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);

				GUI_Clear();
				GUI_SetFont(GUI_FONT_24B_ASCII);
				GUI_DispStringHCenterAt("Juan Fernandez",100,10);
				GUI_SetFont(GUI_FONT_24B_ASCII);
				GUI_DispDecAt(hora,240,10,2);
				GUI_DispStringAt(":",264,10);
				GUI_DispDecAt(minutos,271,10,2);

				GUI_DrawHLine(40,0,320);
				GUI_DrawHLine(45,0,320);

				GUI_SetFont(GUI_FONT_24B_ASCII);
				GUI_DispStringHCenterAt("MENSAJE ENVIADO",160,110);

				GUI_DrawHLine(195,0,320);
				GUI_DrawHLine(200,0,320);
				xSemaphoreGive(Semaforo_SSP);

				Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
				vTaskDelay(1000/portTICK_PERIOD_MS);
				Chip_GPIO_SetPinOutLow(LPC_GPIO, BUZZER);

				vTaskDelay(2000/portTICK_PERIOD_MS);
				EstadoPantalla=0;

			break;

			case 7:		//PANTALLA PRINCIPAL SIN USUARIO REGISTRADO
				if(FlagEstado==ON)
				{
					xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);

					GUI_Clear();
					FlagEstado=OFF;
					aux=minutos;
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringAt("/     /",127,10);
					GUI_DispDecAt(dia,101,10,2);
					GUI_DispDecAt(mes,137,10,2);
					GUI_DispDecAt(ano,172,10,4);

					GUI_SetFont(GUI_FONT_32B_ASCII);
					GUI_DispStringHCenterAt("POR FAVOR",160,60);
					GUI_DispStringHCenterAt("ACERQUE SU TARJETA",160,100);

					GUI_SetFont(GUI_FONT_D48);
					GUI_DispDecAt(hora,82,155,2);
					GUI_DispDecAt(minutos,170,155,2);
					GUI_SetFont(GUI_FONT_32B_ASCII);
					GUI_DispStringAt(".",158,145);
					GUI_DispStringAt(".",158,170);

					xSemaphoreGive(Semaforo_SSP);
				}
				if(uxQueueMessagesWaiting(Cola_Datos_RFID)!=0)
				{
					FlagEstado=ON;
					EstadoPantalla=0;
					xSemaphoreTake(Semaforo_YaHayTarj, portMAX_DELAY);
					ReceivePulsadores=0;
					Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
					vTaskDelay(250/portTICK_PERIOD_MS);
					Chip_GPIO_SetPinOutLow(LPC_GPIO, BUZZER);
				}
				if(aux!=minutos)
				{
					FlagEstado=ON;
				}
			break;

			case 9:		//CONFIRMAR SALIDA
				if(FlagEstado==ON)
				{
					xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);

					GUI_Clear();
					FlagEstado=OFF;
					j=0;
					Flag10sPantalla=ON;
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("Juan Fernandez",100,10);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispDecAt(hora,240,10,2);
					GUI_DispStringAt(":",264,10);
					GUI_DispDecAt(minutos,271,10,2);

					GUI_DrawHLine(40,0,320);
					GUI_DrawHLine(45,0,320);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt(" PARA CONFIRMAR SALIDA",160,60);
					GUI_DispStringHCenterAt(" DE CONDUCTOR MANTENGA",160,90);
					GUI_DispStringHCenterAt(" PRESIONADOS LOS",160,120);
					GUI_DispStringHCenterAt(" DOS PULSADORES...",160,150);

					GUI_DrawHLine(195,0,320);
					GUI_DrawHLine(200,0,320);
					xSemaphoreGive(Semaforo_SSP);
				}
				if(ReceivePulsadores==11)		//Se presionaron ambos pulsadores
				{
					xQueueReset(Cola_Datos_RFID);
					EstadoPantalla=11; //FALTA SACAR LA TARJETA
					FlagEstado=ON;
					ReceivePulsadores = 0;
				}
				else
				{
					ReceivePulsadores = 0;
				}
				if(Flag10sPantalla==OFF)
				{
					EstadoPantalla=0;
					FlagEstado=ON;
				}
			break;

			case 10:	//CONFIRMAR SMS
				if(FlagEstado==ON)
				{
					xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);

					GUI_Clear();
					FlagEstado=OFF;
					j=0;
					Flag10sPantalla=ON;
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("Juan Fernandez",100,10);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispDecAt(hora,240,10,2);
					GUI_DispStringAt(":",264,10);
					GUI_DispDecAt(minutos,271,10,2);

					GUI_DrawHLine(40,0,320);
					GUI_DrawHLine(45,0,320);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt(" SE ENVIARA UN SMS A LA",160,60);
					GUI_DispStringHCenterAt(" CENTRAL. PARA CONFIRMAR",160,90);
					GUI_DispStringHCenterAt(" MANTENGA APRETADO LOS",160,120);
					GUI_DispStringHCenterAt(" DOS PULSADORES...",160,150);

					GUI_DrawHLine(195,0,320);
					GUI_DrawHLine(200,0,320);
					xSemaphoreGive(Semaforo_SSP);
				}
				if(ReceivePulsadores==11)		//Se presionaron ambos pulsadores
				{
					EstadoPantalla=6;
					FlagEstado=ON;
					ReceivePulsadores = 0;
				}
				else
				{
					ReceivePulsadores = 0;
				}
				if(Flag10sPantalla==OFF)
				{
					EstadoPantalla=0;
					FlagEstado=ON;
				}
			break;

			case 11:	//DESLOGUEO CONFIRMADO
				xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);

				GUI_Clear();
				GUI_SetFont(GUI_FONT_24B_ASCII);
				GUI_DispStringHCenterAt("Juan Fernandez",100,10);
				GUI_SetFont(GUI_FONT_24B_ASCII);
				GUI_DispDecAt(hora,240,10,2);
				GUI_DispStringAt(":",264,10);
				GUI_DispDecAt(minutos,271,10,2);

				GUI_DrawHLine(40,0,320);
				GUI_DrawHLine(45,0,320);

				GUI_SetFont(GUI_FONT_24B_ASCII);
				GUI_DispStringHCenterAt("SE DESLOGUEO",160,100);
				GUI_DispStringHCenterAt("CORRECTAMENTE",160,130);
				GUI_DrawHLine(195,0,320);
				GUI_DrawHLine(200,0,320);
				xSemaphoreGive(Semaforo_SSP);

				Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
				vTaskDelay(1000/portTICK_PERIOD_MS);
				Chip_GPIO_SetPinOutLow(LPC_GPIO, BUZZER);

				vTaskDelay(1000/portTICK_RATE_MS);

				EstadoPantalla=7;
				minConductor=0;
				horaConductor=0;
				FlagEstado=ON;
				xSemaphoreGive(Semaforo_YaHayTarj);
				break;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* main */
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
	vSemaphoreCreateBinary(Semaforo_RTCgsm);			//Semaforo que permite guardar cada 30 seg
	xSemaphoreTake(Semaforo_RTCgsm, portMAX_DELAY);
	vSemaphoreCreateBinary(Semaforo_RTCsd);			//Semaforo que permite guardar cada 30 seg
	xSemaphoreTake(Semaforo_RTCsd, portMAX_DELAY);
	vSemaphoreCreateBinary(Semaforo_GSM_Closed);			//Creamos el semaforo
	xSemaphoreTake(Semaforo_GSM_Closed, portMAX_DELAY);
	vSemaphoreCreateBinary(Semaforo_GSM_Enviado);			//Creamos el semaforo
	xSemaphoreTake(Semaforo_GSM_Enviado, portMAX_DELAY);
	vSemaphoreCreateBinary(Semaforo_SSP);			//Creamos el semaforo
	//xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);
	vSemaphoreCreateBinary(Semaforo_YaHayTarj);


	Cola_RX1 = xQueueCreate(UART_RRB_SIZE, sizeof(uint8_t));	//Creamos una cola
	Cola_TX1 = xQueueCreate(UART_SRB_SIZE, sizeof(uint8_t));	//Creamos una cola
	Cola_RX2 = xQueueCreate(UART_RRB_SIZE, sizeof(uint8_t));	//Creamos una cola
	Cola_TX2 = xQueueCreate(UART_SRB_SIZE, sizeof(uint8_t));	//Creamos una cola
	Cola_Pulsadores = xQueueCreate(1, sizeof(uint32_t));		//Creamos una cola
	Cola_Connect = xQueueCreate(1, sizeof(uint8_t));			//Creamos una cola
	xQueueOverwrite(Cola_Connect, &aux); //cargo con 0 para que realice toda la secuencia por primera vez
	Cola_SD = xQueueCreate(4, sizeof(char) * 100);	//Creamos una cola para mandar una trama completa
	Cola_Datos_GPS = xQueueCreate(1, sizeof(struct Datos_Nube));
	Cola_Datos_RFID = xQueueCreate(1, sizeof(Tarjetas_RFID));
	HoraEntrada = xQueueCreate(1, sizeof(Entrada_RFID));
	Cola_Inicio_Tarjetas = xQueueCreate(1, sizeof(Tarjetas_RFID*));


	xTaskCreate(vTaskTFT, (char *) "vTaskTFT",
					( ( unsigned short ) 250), NULL, (tskIDLE_PRIORITY + 1UL),
							(xTaskHandle *) NULL);

	xTaskCreate(xTaskPulsadores, (char *) "vTaskPulsadores",
			(( unsigned short ) 16), NULL, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);
	/*
	xTaskCreate(xTaskWriteSD, (char *) "xTaskWriteSD",
					configMINIMAL_STACK_SIZE * 2, NULL, (tskIDLE_PRIORITY + 2UL),
					(xTaskHandle *) NULL);

	xTaskCreate(vTaskInicSD, (char *) "vTaskInicSD",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);
 	*/

	xTaskCreate(vTaskRFID, (char *) "vTaskRFID",
			(( unsigned short ) 150), NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskRFIDConfig, (char *) "xTaskRFIDConfig",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 4UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskRTConfig, (char *) "xTaskRTConfig",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);
	/*
	xTaskCreate(vTaskLeerAnillo1, (char *) "vTaskLeerAnillo1",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);
	*/
	xTaskCreate(vTaskLeerAnillo2, (char *) "vTaskLeerAnillo2",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);
	/*
	xTaskCreate(vTaskCargarAnillo1, (char *) "vTaskCargarAnillo1",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskCargarAnillo2, (char *) "vTaskCargarAnillo2",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskUART1Config, (char *) "xTaskUART1Config",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);
	*/
	xTaskCreate(xTaskUART2Config, (char *) "xTaskUART2Config",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);
	/*
	xTaskCreate(vTaskGSMConfig, (char *) "vTaskGSMConfig",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskEnviarGSM, (char *) "vTaskEnviarGSM",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);
	 */
	xTaskCreate(vTaskAnalizarGPS, (char *) "vTaskAnalizarGPS",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);
	/*
	xTaskCreate(vTaskTarjetasGSM, (char *) "vTaskTarjetasGSM",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);
	*/

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Nunca debería arribar aquí */
    return 0;
}
