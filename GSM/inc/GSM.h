#define RST_GSM	2,13	//Expansion 15	(NO CONECTAR)

extern RINGBUFF_T txring1;

#define UART_SELECTION_GSM 	LPC_UART1
#define TX_RING_GSM			txring1
#define TX_COLA_GSM			Cola_RX1


void AnalizarTramaGSM (uint8_t dato);
void EnviarTramaGSM (char* latitud, char* longitud, unsigned int rfid);
