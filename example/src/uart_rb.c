/*
 * @brief UART interrupt example with ring buffers
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2014
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#include "chip.h"
#include "board.h"
#include "string.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

#if defined(BOARD_EA_DEVKIT_1788) || defined(BOARD_EA_DEVKIT_4088)
#define UART_SELECTION 	LPC_UART0
#define IRQ_SELECTION 	UART0_IRQn
#define HANDLER_NAME 	UART0_IRQHandler
#elif defined(BOARD_NXP_LPCXPRESSO_1769)
#define UART_SELECTION 	LPC_UART0
#define IRQ_SELECTION 	UART0_IRQn
#define HANDLER_NAME 	UART0_IRQHandler
#else
#error No UART selected for undefined board
#endif

#define PORT(x) 	((uint8_t) x)
#define PIN(x)		((uint8_t) x)
#define OUTPUT		((uint8_t) 1)
#define INPUT		((uint8_t) 0)

/* Transmit and receive ring buffers */
STATIC RINGBUFF_T txring0, rxring0;
STATIC RINGBUFF_T txring1, rxring1;

/* Transmit and receive ring buffer sizes */
#define UART0_SRB_SIZE 128	/* Send */
#define UART0_RRB_SIZE 32	/* Receive */
#define UART1_SRB_SIZE 128	/* Send */
#define UART1_RRB_SIZE 32	/* Receive */

/* Transmit and receive buffers */
static uint8_t rxbuff0[UART0_RRB_SIZE], txbuff0[UART0_SRB_SIZE];
static uint8_t rxbuff1[UART1_RRB_SIZE], txbuff1[UART1_SRB_SIZE];

const char inst1[] = "Iniciando ejemplo con UART...\r\n";
const char inst2[] = "Presionar cualquier tecla para eco o esc para salir\r\n";
const char enviando[] = "Enviando datos por las UARTs\r\n";

const char atnot[] = "Opcion no valida !\r\n";
const char at0[] = "AT\r\n";
const char at1[] = "AT+IPR=9600\r\n";
const char at2[] = "AT+CMGF=1\r\n";

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	UART 0 interrupt handler using ring buffers
 * @return	Nothing
 */
void UART0_IRQHandler(void)
{
	/* Want to handle any errors? Do it here. */

	/* Use default ring buffer handler. Override this with your own
	   code if you need more capability. */
	Chip_UART_IRQRBHandler(LPC_UART0, &rxring0, &txring0);
}

/**
 * @brief	UART 1 interrupt handler using ring buffers
 * @return	Nothing
 */
void UART1_IRQHandler(void)
{
	/* Want to handle any errors? Do it here. */

	/* Use default ring buffer handler. Override this with your own
	   code if you need more capability. */
	Chip_UART_IRQRBHandler(LPC_UART1, &rxring1, &txring1);
}

/**
 * @brief	Main UART program body
 * @return	Always returns 1
 */
int main(void)
{
	uint8_t key0, key1;
	int bytes0, bytes1;

	SystemCoreClockUpdate();

	//Board_Init();
	Chip_GPIO_Init(LPC_GPIO);
	Chip_IOCON_Init(LPC_IOCON);
	Chip_IOCON_PinMux (LPC_IOCON , PORT(0) , PIN(22), IOCON_MODE_INACT , IOCON_FUNC0);

	//Board_UART_Init(UART_SELECTION);
	//UART0
	Chip_IOCON_PinMux (LPC_IOCON , PORT(0) , PIN(2), IOCON_MODE_INACT , IOCON_FUNC1);
	Chip_IOCON_PinMux (LPC_IOCON , PORT(0) , PIN(3), IOCON_MODE_INACT , IOCON_FUNC1);
	//UART1
	Chip_IOCON_PinMux (LPC_IOCON , PORT(0) , PIN(15), IOCON_MODE_INACT , IOCON_FUNC1);
	Chip_IOCON_PinMux (LPC_IOCON , PORT(0) , PIN(16), IOCON_MODE_INACT , IOCON_FUNC1);

	//Board_LED_Set(0, false);
	Chip_GPIO_SetPinOutLow (LPC_GPIO , PORT(0) , PIN (22));

	/* Setup UART0 for 9.6K8N1 */
	Chip_UART_Init(LPC_UART0);
	Chip_UART_SetBaud(LPC_UART0, 9600);
	Chip_UART_ConfigData(LPC_UART0, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_UART0, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_UART0);

	/* Setup UART1 for 9.6K8N1 */
	Chip_UART_Init(LPC_UART1);
	Chip_UART_SetBaud(LPC_UART1, 9600);
	Chip_UART_ConfigData(LPC_UART1, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_UART1, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_UART1);

	/* Before using the ring buffers, initialize them using the ring
	   buffer init function */
	RingBuffer_Init(&rxring0, rxbuff0, 1, UART0_RRB_SIZE);
	RingBuffer_Init(&txring0, txbuff0, 1, UART0_SRB_SIZE);

	RingBuffer_Init(&rxring1, rxbuff1, 1, UART1_RRB_SIZE);
	RingBuffer_Init(&txring1, txbuff1, 1, UART1_SRB_SIZE);

	/* Reset and enable FIFOs, FIFO trigger level 3 (14 chars) */
	Chip_UART_SetupFIFOS(LPC_UART0, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
							UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

	Chip_UART_SetupFIFOS(LPC_UART1, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
								UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(LPC_UART0, (UART_IER_RBRINT | UART_IER_RLSINT));

	Chip_UART_IntEnable(LPC_UART1, (UART_IER_RBRINT | UART_IER_RLSINT));

	/* preemption = 1, sub-priority = 1 */
	NVIC_SetPriority(UART0_IRQn, 1);
	NVIC_EnableIRQ(UART0_IRQn);

	NVIC_SetPriority(UART1_IRQn, 1);
	NVIC_EnableIRQ(UART1_IRQn);

	/* Send initial messages */
	Chip_UART_SendRB(LPC_UART0, &txring0, inst1, sizeof(inst1) - 1);
	Chip_UART_SendRB(LPC_UART0, &txring0, inst2, sizeof(inst2) - 1);

	Chip_UART_SendRB(LPC_UART1, &txring1, inst1, sizeof(inst1) - 1);
	Chip_UART_SendRB(LPC_UART1, &txring1, inst2, sizeof(inst2) - 1);

	/* Poll the receive ring buffer for the ESC (ASCII 27) key */
	key0 = key1 = 0;

	while (key0 != 27)
	{
		bytes0 = Chip_UART_ReadRB(LPC_UART0, &rxring0, &key0, 1);
		bytes1 = Chip_UART_ReadRB(LPC_UART1, &rxring1, &key1, 1);

		if (bytes0 > 0)
		{
			switch (key0)
			{

				case 49:
					Chip_UART_SendRB(LPC_UART1, &txring1, at0, sizeof(at0) - 1);
					break;

				case 50:
					Chip_UART_SendRB(LPC_UART1, &txring1, at1, sizeof(at1) - 1);
					break;

				case 51:
					Chip_UART_SendRB(LPC_UART1, &txring1, at2, sizeof(at2) - 1);
					break;

				default:
					Chip_UART_SendRB(LPC_UART1, &txring1, atnot, sizeof(atnot) - 1);
					break;

			}

			/* Wrap value back around */
			if ( (Chip_UART_SendRB(LPC_UART0, &txring0, (const uint8_t *) &key0, 1) != 1) )
			{
				//Board_LED_Toggle(0);/* Toggle LED if the TX FIFO is full */
				Chip_GPIO_SetPinToggle(LPC_GPIO,PORT(0),PIN(22));
			}
		}

		if(bytes1 > 0)
		{
			bytes1 = 0;
			Chip_UART_SendRB(LPC_UART0, &txring0, enviando, sizeof(enviando) - 1);
		}
	}

	/* DeInitialize UART0 peripheral */
	NVIC_DisableIRQ(UART0_IRQn);
	Chip_UART_DeInit(LPC_UART0);

	/* DeInitialize UART1 peripheral */
	NVIC_DisableIRQ(UART1_IRQn);
	Chip_UART_DeInit(LPC_UART1);

	return 1;
}

/**
 * @}
 */
