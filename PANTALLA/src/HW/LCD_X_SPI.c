/****************************************************************************
 * @file     LCD_X_SPI.c
 * @brief    FlexColor SPI driver
 * @version  1.0
 * @date     09. May. 2012
 *
 * @note
 * Copyright (C) 2012 NXP Semiconductors(NXP), All rights reserved.
 */

#include "GUI.h"
#include "LCD_X_SPI.h"
#include "chip.h"
#include "genmacro.h"


uint32_t A=0,B=0,C=0;

// FIXME: SSP0 not working on LPCXpresso LPC1769.  There seems to be some sort
//        of contention on the MISO signal.  The contention originates on the
//		  LPCXpresso side of the signal.
//
#define LPC_SSP           LPC_SSP1			//LPC_SSP0
#define SSP_IRQ           SSP1_IRQn			//SSP0_IRQn
#define SSPIRQHANDLER     SSP1_IRQHandler	//SSP0_IRQHandler

#define BUFFER_SIZE                         (0x100)
#define SSP_DATA_BITS                       (SSP_BITS_8)
#define SSP_DATA_BIT_NUM(databits)          (databits + 1)
#define SSP_DATA_BYTES(databits)            (((databits) > SSP_BITS_8) ? 2 : 1)
#define SSP_LO_BYTE_MSK(databits)           ((SSP_DATA_BYTES(databits) > 1) ? 0xFF : (0xFF >> \
																					  (8 - SSP_DATA_BIT_NUM(databits))))
#define SSP_HI_BYTE_MSK(databits)           ((SSP_DATA_BYTES(databits) > 1) ? (0xFF >> \
																			   (16 - SSP_DATA_BIT_NUM(databits))) : 0)

#define SSP_MODE_SEL                        (0x31)
#define SSP_TRANSFER_MODE_SEL               (0x32)
#define SSP_MASTER_MODE_SEL                 (0x31)
#define SSP_SLAVE_MODE_SEL                  (0x32)
#define SSP_POLLING_SEL                     (0x31)
#define SSP_INTERRUPT_SEL                   (0x32)
#define SSP_DMA_SEL                         (0x33)

/* Tx buffer */


/* Rx buffer */


static SSP_ConfigFormat ssp_format;
//static Chip_SSP_DATA_SETUP_T xf_setup;
static volatile uint8_t  isXferCompleted = 0;

/*********************** Hardware specific configuration **********************/



//#define PIN_CS      (1 << 6)

/*--------------- Graphic LCD interface hardware definitions -----------------*/

/* Pin CS setting to 0 or 1 */
//#define LCD_CS(x)   ((x) ? (LPC_GPIO0->FIOSET = PIN_CS)    : (LPC_GPIO0->FIOCLR = PIN_CS))




#define SPI_START   (0x70)              /* Start byte for SPI transfer */
#define SPI_RD      (0x01)              /* WR bit 1 within start */
#define SPI_WR      (0x00)              /* WR bit 0 within start */
#define SPI_DATA    (0x02)              /* RS bit 1 within start byte */
#define SPI_INDEX   (0x00)              /* RS bit 0 within start byte */

/* local functions */
static inline void wr_cmd (unsigned char cmd);						/* Write command to LCD */
static inline void wr_dat (unsigned short dat);						/* Write data to LCD */
static inline unsigned char spi_tran (unsigned char byte);	/* Write and read a byte over SPI */
static inline void spi_tran_fifo (unsigned char byte);		/* Only write a byte over SPI (faster) */



/*******************************************************************************
* Put byte in SSP1 Tx FIFO. Used for faster SPI writing                        *
*   Parameter:    byte:   byte to be sent                                      *
*   Return:                                                                    *
*******************************************************************************/

static inline void spi_tran_fifo (unsigned char byte)
{
  while (!(LPC_SSP1->SR & (1<<1))); //LPC_SSP0						/* wait until TNF set */
  LPC_SSP1->DR = byte; //LPC_SSP0
}

/*******************************************************************************
* Write a command the LCD controller                                           *
*   Parameter:    cmd:    command to be written                                *
*   Return:                                                                    *
*******************************************************************************/
static inline void wr_cmd (unsigned char cmd)
{
  LCD_CS_LOW;
  spi_tran_fifo(SPI_START | SPI_WR | SPI_INDEX);		/* Write : RS = 0, RW = 0 */
  spi_tran_fifo(0);
  spi_tran_fifo(cmd);
  while(LPC_SSP1->SR & (1<<4));							/* wait until done */
  LCD_CS_HIGH;
}

/*******************************************************************************
* Write data to the LCD controller                                             *
*   Parameter:    dat:    data to be written                                   *
*   Return:                                                                    *
*******************************************************************************/
static inline void wr_dat (unsigned short dat)
{
  LCD_CS_LOW;
  spi_tran_fifo(SPI_START | SPI_WR | SPI_DATA);			/* Write : RS = 1, RW = 0 */
  spi_tran_fifo((dat >>   8));							/* Write D8..D15 */
  spi_tran_fifo((dat & 0xFF));							/* Write D0..D7 */
  while(LPC_SSP1->SR & (1<<4));	//LPC_SSP0						/* wait until done */
  LCD_CS_HIGH;
}

/*******************************************************************************
* Transfer 1 byte over the serial communication, wait until done and return    *
* received byte                                                                *
*   Parameter:    byte:   byte to be sent                                      *
*   Return:               byte read while sending                              *
*******************************************************************************/
static inline unsigned char spi_tran (unsigned char byte)
{
  uint8_t Dummy; //LPC_SSP0

  while(LPC_SSP1->SR & (1<<4) || LPC_SSP1->SR & (1<<2))	/* while SSP1 busy or Rx FIFO not empty ... */
	  Dummy = LPC_SSP1->DR;								/* ... read Rx FIFO */
  LPC_SSP1->DR = byte;									/* Transmit byte */
  while (!(LPC_SSP1->SR & (1<<2)));						/* Wait until RNE set */
  return (LPC_SSP1->DR);
}




/*******************************************************************************
* Initialize SPI (SSP) peripheral at 8 databit with a bitrate of 12.5Mbps      *
*   Parameter:                                                                 *
*   Return:                                                                    *
*******************************************************************************/



void LCD_X_SPI_Init(void)
{
	 /* Initialize SSP0 pins connect
			 * P1.20: SCK
			 * P1.21: SSEL se usa como gpio ?
			 * P1.23: MISO
			 * P1.24: MOSI
			 */
#ifdef PLACAINFO2
	Chip_IOCON_PinMux(LPC_IOCON, 1, 20, IOCON_MODE_INACT, IOCON_FUNC3);
	Chip_IOCON_PinMux(LPC_IOCON, 1, 21, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 21);
	Chip_IOCON_PinMux(LPC_IOCON, 1, 23, IOCON_MODE_INACT, IOCON_FUNC3);
	Chip_IOCON_PinMux(LPC_IOCON, 1, 24, IOCON_MODE_INACT, IOCON_FUNC3);

	//Reset  P1.29
	Chip_IOCON_PinMux(LPC_IOCON, 1, 29, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1,29);
	// DC- RS P1.27
	Chip_IOCON_PinMux(LPC_IOCON, 1, 27, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 27);

#else
	/*
	Chip_IOCON_PinMux(LPC_IOCON, 0, 15, IOCON_MODE_INACT, IOCON_FUNC2);

	//CHIP SELECT
	Chip_IOCON_PinMux(LPC_IOCON, 0, 8, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 8);

	Chip_IOCON_PinMux(LPC_IOCON, 0, 17, IOCON_MODE_INACT, IOCON_FUNC2);
	Chip_IOCON_PinMux(LPC_IOCON, 0, 18, IOCON_MODE_INACT, IOCON_FUNC2);

	//Reset  P1.29
	Chip_IOCON_PinMux(LPC_IOCON, 0, 7, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0,7);
	// DC- RS P1.27
	Chip_IOCON_PinMux(LPC_IOCON, 0, 6, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 6);
*/
	Chip_IOCON_PinMux(LPC_IOCON, TFT_SCK, IOCON_MODE_INACT, IOCON_FUNC2);

	//CHIP SELECT
	Chip_IOCON_PinMux(LPC_IOCON, TFT_CS, IOCON_MODE_INACT, IOCON_FUNC2);
	//Chip_IOCON_PinMux(LPC_IOCON, 0, 8, IOCON_MODE_INACT, IOCON_FUNC0);
	//Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 8);

	Chip_IOCON_PinMux(LPC_IOCON, TFT_MISO, IOCON_MODE_INACT, IOCON_FUNC2);
	Chip_IOCON_PinMux(LPC_IOCON, TFT_MOSI, IOCON_MODE_INACT, IOCON_FUNC2);

	//Reset  P1.29
	Chip_IOCON_PinMux(LPC_IOCON, TFT_RESET, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, TFT_RESET);
	// DC- RS P1.27
	Chip_IOCON_PinMux(LPC_IOCON, TFT_DC, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, TFT_DC);


#endif


	LCD_CS_HIGH;//Chip select empieza en alto.

	Chip_SSP_DeInit(LPC_SSP);
	Chip_SSP_Init(LPC_SSP);

	ssp_format.frameFormat = SSP_FRAMEFORMAT_SPI;
	ssp_format.bits = SSP_DATA_BITS;
	ssp_format.clockMode = SSP_CLOCK_MODE0;
	Chip_SSP_SetFormat(LPC_SSP, ssp_format.bits, ssp_format.frameFormat, ssp_format.clockMode);
	//Chip_SSP_SetBitRate(LPC_SSP, 12500000);
	Chip_SSP_SetBitRate(LPC_SSP, 8000000);

	Chip_SSP_Enable(LPC_SSP);










	uint8_t Dummy;

  /* Enable clock for SSP1, clock = CCLK / 2 */
  //LPC_SC->PCONP       |= 0x00000400;
  //LPC_SC->PCLKSEL0    |= 0x00200000;	/* PCLK = CCLK / 2 = 50MHz */

  /* Configure the LCD Control pins */
  //LPC_PINCON->PINSEL9 &= 0xF0FFFFFF;
  //LPC_GPIO4->FIODIR   |= 0x30000000;
  //LPC_GPIO4->FIOSET    = 0x20000000;

  /* SSEL1 is GPIO output set to high */
  //LPC_GPIO0->FIODIR   |= 0x00000040;
  //LPC_GPIO0->FIOSET    = 0x00000040;
  //LPC_PINCON->PINSEL0 &= 0xFFF03FFF;
  //LPC_PINCON->PINSEL0 |= 0x000A8000;

  /* Enable SPI in Master Mode, CPOL=1, CPHA=1 */
  /* 12.5 MBit used for Data Transfer @ 100MHz */
  //LPC_SSP1->CR0        = 0x1C7;		/* SCR = 1 */
  //LPC_SSP1->CPSR       = 0x02;		/* CPSDVSR  = 2. Bit frequency = PCLK / (CPSDVSR × [SCR+1]) = 50 / (2 × [1+1]) = 12.5Mbps */
  //LPC_SSP1->CR1        = 0x02;

	/*
  while(LPC_SSP0->SR & (1<<2))
    Dummy = LPC_SSP0->DR;	*/		/* Clear the Rx FIFO */

	  while(LPC_SSP1->SR & (1<<2))
	    Dummy = LPC_SSP1->DR;

  //LPC_GPIO4->FIOSET = 0x10000000;	/* Activate LCD backlight */
}

/*******************************************************************************
* Write command                                                                *
*   Parameter:    c: command to write                                          *
*   Return:                                                                    *
*******************************************************************************/
void LCD_X_SPI_Write00(U16 c)
{
  wr_cmd(c);
}


/*******************************************************************************
* Write data byte                                                              *
*   Parameter:    c: word to write                                             *
*   Return:                                                                    *
*******************************************************************************/
void LCD_X_SPI_Write01(U16 c)
{
  wr_dat(c);
}

/*******************************************************************************
* Write multiple data bytes                                                    *
*   Parameter:    pData:    pointer to words to write                          *
*                 NumWords: Number of words to write                           *
*   Return:                                                                    *
*******************************************************************************/
void LCD_X_SPI_WriteM01(U16 * pData, int NumWords)
{
	LCD_CS_LOW;
  spi_tran_fifo(SPI_START | SPI_WR | SPI_DATA);			/* Write : RS = 1, RW = 0 */

  while(NumWords--)
  {
	  spi_tran_fifo(((*pData) >>   8));					/* Write D8..D15 */
	  spi_tran_fifo(((*(pData++)) & 0xFF));				/* Write D0..D7 */
  }
  while(LPC_SSP1->SR & (1<<4));							/* wait until done */
  LCD_CS_HIGH;
}

/*******************************************************************************
* Read multiple data bytes                                                     *
*   Parameter:    pData:    pointer to words to read                           *
*                 NumWords: Number of words to read                            *
*   Return:                                                                    *
*******************************************************************************/
void LCD_X_SPI_ReadM01(U16 * pData, int NumWords)
{
	LCD_CS_LOW;
  spi_tran_fifo(SPI_START | SPI_RD | SPI_DATA);			/* Read: RS = 1, RW = 1 */
  spi_tran_fifo(0);										/* Dummy byte 1 */
  while(NumWords--)
  {
	*pData = spi_tran(0) << 8;							/* Read D8..D15 */
	*(pData++) |= spi_tran(0);							/* Read D0..D7 */
  }
  while(LPC_SSP1->SR & (1<<4));							/* wait until done */
  LCD_CS_HIGH;
}


/*************************** End of file ****************************/
void LCD_X_SPI_8_Write00 (U8 cmd)
{
	LCD_CD_LOW;
	LCD_CS_LOW;
	spi_tran_fifo(cmd);
	while(LPC_SSP1->SR & (1<<4)); //LPC_SSP0							/* wait until done */
	LCD_CD_HIGH;
	LCD_CS_HIGH;
}

void LCD_X_SPI_8_Write01 (U8 dat)
{
	LCD_CD_HIGH;
	LCD_CS_LOW;
	spi_tran_fifo(dat);
	while(LPC_SSP1->SR & (1<<4));	//LPC_SSP0						/* wait until done */
	LCD_CS_HIGH;
	LCD_CD_LOW;
}

void LCD_X_SPI_8_WriteM00(U8 * pData, int NumWords)
{
	  LCD_CD_LOW;
	  LCD_CS_LOW;

	  while(NumWords--)
	  {
		 spi_tran_fifo((*pData));					/* Write D8..D15 */
		 pData++;				/* Write D0..D7 */
	  }
	  while(LPC_SSP1->SR & (1<<4));		//LPC_SSP0					/* wait until done */
	  LCD_CS_HIGH;
	  LCD_CD_HIGH;
}

void LCD_X_SPI_8_WriteM01 (U8 * pData, int NumWords)
{

	      LCD_CD_HIGH;
		  LCD_CS_LOW;

		  while(NumWords--)
		  {
			 //spi_tran_fifo((*pData));					/* Write D8..D15 */
			 spi_tran_fifo(*(pData++));


			 //pData++;				/* Write D0..D7 */
		  }
 		  while(LPC_SSP1->SR & (1<<4));			//LPC_SSP0				/* wait until done */
		  LCD_CS_HIGH;
		  LCD_CD_LOW;
}

U8 LCD_X_SPI_8_Read00 (void)
{
	U8 A;
	LCD_CD_LOW;
	LCD_CS_LOW;
	A=spi_tran(0);
	LCD_CS_HIGH;
	LCD_CD_HIGH;
	return A;

}

U8 LCD_X_SPI_8_Read01 (void)
{
	U8 A;
	LCD_CD_HIGH;
	LCD_CS_LOW;
	A=spi_tran(0);
	LCD_CS_HIGH;
	LCD_CD_LOW;
	return A;

}
void LCD_X_SPI_8_ReadM00 (U8 * pData, int NumWords)
{
	LCD_CD_LOW;
	LCD_CS_LOW;

	  while(NumWords--)
	  {
		*pData = spi_tran(0);
		pData++;
	  }
	  while(LPC_SSP1->SR & (1<<4));			//LPC_SSP0				/* wait until done */
	  LCD_CS_HIGH;
	  LCD_CD_HIGH;

}

void LCD_X_SPI_8_ReadM01 (U8 * pData, int NumWords)
{

	  LCD_CD_HIGH;
		LCD_CS_LOW;

		  while(NumWords--)
		  {
			*pData = spi_tran(0);
			pData++;
		  }
		  while(LPC_SSP1->SR & (1<<4));		//LPC_SSP0					/* wait until done */
		  LCD_CS_HIGH;
		  LCD_CD_LOW;
}
