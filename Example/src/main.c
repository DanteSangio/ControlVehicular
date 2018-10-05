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



//#include "RegsLPC1769.h"



#define DEBUGOUT1(...) printf(__VA_ARGS__)
#define DEBUGOUT(...) printf(__VA_ARGS__)
#define DEBUGSTR(...) printf(__VA_ARGS__)

#define MIN_BALANCE 300


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

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

SemaphoreHandle_t Semaforo_RFID;
SemaphoreHandle_t Semaforo_RTC;

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

/**
 * Executed every time the card reader detects a user in
 */
void userTapIn()

{

//	show card UID
	DEBUGOUT("\nCard uid bytes: ");
	for (uint8_t i = 0; i < mfrcInstance->uid.size; i++) {
		DEBUGOUT(" %X", mfrcInstance->uid.uidByte[i]);
	}
	DEBUGOUT("\n\r");


	// Convert the uid bytes to an integer, byte[0] is the MSB
	last_user_ID =
		(int)mfrcInstance->uid.uidByte[3] |
		(int)mfrcInstance->uid.uidByte[2] << 8 |
		(int)mfrcInstance->uid.uidByte[1] << 16 |
		(int)mfrcInstance->uid.uidByte[0] << 24;

	DEBUGOUT("\nCard Read user ID: %u \n\r",last_user_ID);

	Comparar(last_user_ID);

	// Read the user balance NO BORRAR SINO NO DETECTA CUANDO HAY TARJETA NUEVA
	last_balance = readCardBalance(mfrcInstance);

}

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


/*****************************************************************************
 * Public functions
 ****************************************************************************/


static void xTaskRFIDConfig(void *pvParameters)
{
	SystemCoreClockUpdate();

	DEBUGOUT1("PRUEBA RFID..\n");	//Imprimo en la consola

	setupRFID(&mfrcInstance);

	xSemaphoreGive(Semaforo_RFID);//Doy el semaforo para que se inicie la lectura

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

	/*
	FullTime.time[RTC_TIMETYPE_SECOND]  = 0;
	FullTime.time[RTC_TIMETYPE_MINUTE]  = 05;
	FullTime.time[RTC_TIMETYPE_HOUR]    = 19;
	FullTime.time[RTC_TIMETYPE_DAYOFMONTH]  = 26;
	FullTime.time[RTC_TIMETYPE_DAYOFWEEK]   = 4;
	FullTime.time[RTC_TIMETYPE_DAYOFYEAR]   = 207;
	FullTime.time[RTC_TIMETYPE_MONTH]   = 07;
	FullTime.time[RTC_TIMETYPE_YEAR]    = 2018;

	Chip_RTC_SetFullTime(LPC_RTC, &FullTime);
	//*/

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
/* vTaskInicTimer */
static void vTaskRFID(void *pvParameters)
{
	xSemaphoreTake(Semaforo_RFID, portMAX_DELAY); //semaforo de inicializacion de rfid
	while (1)
	{

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
		vTaskDelay( 1000 / portTICK_PERIOD_MS );//Muestreo cada 1 seg
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
/* main
*/
int main(void)
{
	SystemCoreClockUpdate();

	/* Initializes GPIO */
	Chip_GPIO_Init(LPC_GPIO);

	vSemaphoreCreateBinary(Semaforo_RFID);
	xSemaphoreTake(Semaforo_RFID, portMAX_DELAY); //semaforo de inicializacion de rfid
	vSemaphoreCreateBinary(Semaforo_RTC);			//Creamos el semaforo

	xTaskCreate(vTaskRFID, (char *) "vTaskRFID",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskRFIDConfig, (char *) "xTaskRFIDConfig",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskRTC, (char *) "vTaskRTC",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskRTConfig, (char *) "xTaskRTConfig",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Nunca debería arribar aquí */
    return 0;
}
