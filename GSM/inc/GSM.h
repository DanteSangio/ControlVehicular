#define RST_GSM	2,13	//Expansion 15	(NO CONECTAR)

extern RINGBUFF_T txring1;

#define UART_SELECTION_GSM 	LPC_UART1
#define TX_RING_GSM			txring1
#define RX_COLA_GSM			Cola_RX1
#define APIKEY				"api_key=4IVCTNA39FY9U35C"//apikey donde se encuentran la informacion
#define CHANNEL2			"648303"//channel donde se encuentran las tarjetas
#define APIKEY2				"VRMSAZ0CSI5VVHDM"//apikey donde se encuentran las tarjetas

void AnalizarTramaGSMenvio (uint8_t dato);
void EnviarTramaGSM (char* latitud, char* longitud, char* rfid, int velocidad);
void RecibirTramaGSM(void);
void EnviarMensajeGSM (uint8_t	mensaje);
Tarjetas_RFID* AnalizarTramaGSMrecibido (uint8_t dato);
