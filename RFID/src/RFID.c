/**
 * Executed every time the card reader detects a user in
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

extern int last_balance;
extern unsigned int last_user_ID;
extern MFRC522Ptr_t mfrcInstance;
extern QueueHandle_t Cola_Datos_RFID;


void userTapIn()

{

	uint32_t	ultTarjeta;

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

	ultTarjeta = last_user_ID;
	xQueueOverwrite(Cola_Datos_RFID,&ultTarjeta);//envio a la cola la tarjeta leida
	Comparar(last_user_ID);

	// Read the user balance NO BORRAR SINO NO DETECTA CUANDO HAY TARJETA NUEVA
	last_balance = readCardBalance(mfrcInstance);

}

void ConvIntaChar(uint32_t informacionRFID,char* auxRfid)
{
	uint32_t aux,dato;
	uint8_t	 i=0,j=0;
	dato = informacionRFID;
	char tarj[11];

	do
	{
		aux = dato % 10;
		itoa(aux,&tarj[i],10);
		dato = dato / 10;
		i++;
	}while(dato != 0);

	for(i=0,j=9;i<10;i++,j--)
	{
		auxRfid[j] = tarj[i];
	}
}

