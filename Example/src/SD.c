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
#include "sdcard.h"


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

/*****************************************************************************
 * Private functions
 ****************************************************************************/
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

/*****************************************************************************
 * Public functions
 ****************************************************************************/


void setupSD(int puertoSS, int pinSS)
{
// Define the pins to use as CS(SS or SSEL) and RST
// // Set pins as GPIO
	SSP_Init_PINES(LPC_SSP1);
	Chip_SSP_Init(LPC_SSP1);
	Chip_SSP_Set_Mode(LPC_SSP1, SSP_MODE_MASTER);
	Chip_SSP_SetFormat(LPC_SSP1, SSP_BITS_8, SSP_FRAMEFORMAT_SPI,SSP_CLOCK_CPHA0_CPOL0);
	Chip_SSP_SetBitRate(LPC_SSP1, MFRC522_BIT_RATE);//misma velocidad que la SD
	Chip_SSP_Enable(LPC_SSP1);

	// Set the chipSelectPin as digital output, do not select the slave yet
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, puertoSS,pinSS);
	Chip_GPIO_SetPinState(LPC_GPIO, puertoSS, pinSS, (bool)true);

	//vTaskDelay( 50 / portTICK_PERIOD_MS );//Delay de 50 mseg


}
static void xTaskSDConfig(void *pvParameters)
{

	uint8_t returnStatus,sdcardType,option,i=0;

	SystemCoreClockUpdate();

	DEBUGOUT1("PRUEBA SD..\n");	//Imprimo en la consola

	///////////////////////////////////////////////////////////////////
	/* PARA COMPROBAR SI LA SD ESTA CONECTADA */

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

	xSemaphoreGive(Semaforo_RFID);//Doy el semaforo para que se inicie la lectura

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
/* main
*/
int main(void)
{
	SystemCoreClockUpdate();

	/* Initializes GPIO */
	Chip_GPIO_Init(LPC_GPIO);

	vSemaphoreCreateBinary(Semaforo_RFID);
	xSemaphoreTake(Semaforo_RFID, portMAX_DELAY); //semaforo de inicializacion de rfid

	xTaskCreate(vTaskRFID, (char *) "vTaskRFID",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);
	xTaskCreate(xTaskRFIDConfig, (char *) "xTaskRFIDConfig",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);


	/* Start the scheduler */
	vTaskStartScheduler();

	/* Nunca debería arribar aquí */
    return 0;
}
