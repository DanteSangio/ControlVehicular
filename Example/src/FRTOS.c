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
#include "delay.h"
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

SemaphoreHandle_t Semaforo_RTC;

/*****************************************************************************
 * Private functions
 ****************************************************************************/
/*******************************************************************************
 *  System routine functions
 ******************************************************************************/

/**
 * Executed every time the card reader detects a user in
 */
void userTapIn() {

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

	DEBUGOUT("\nCard Read user ID: %u ",last_user_ID);


		// Read the user balance
		last_balance = readCardBalance(mfrcInstance);

		if (last_balance == (-999)) {
			// Error handling, the card does not have proper balance data inside
		} else {
			// Check for minimum balance
			if (last_balance < MIN_BALANCE) {
				DEBUGOUT(", Insufficient balance\n");
			} else {
				DEBUGOUT(", Balance OK\n");
			}
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

	vTaskDelete(NULL);	//Borra la tarea
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskInicTimer */
static void vTaskRFID(void *pvParameters)
{
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
