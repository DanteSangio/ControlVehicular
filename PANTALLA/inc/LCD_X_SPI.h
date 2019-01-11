/****************************************************************************
 * @file     LCD_X_SPI.h
 * @brief    FlexColor SPI driver
 * @version  1.0
 * @date     09. May. 2012
 *
 * @note
 * Copyright (C) 2012 NXP Semiconductors(NXP), All rights reserved.
 */

#include "genmacro.h"

#ifndef LCD_X_SPI_H_
#define LCD_X_SPI_H_

#define TFT_MOSI	PORT(0),PIN(9)
#define TFT_MISO	PORT(0),PIN(8)
#define TFT_SCK		PORT(0),PIN(7)
#define TFT_CS		PORT(0),PIN(6)
#define TFT_RESET	PORT(0),PIN(21)
#define TFT_DC		PORT(0),PIN(22)


void LCD_X_SPI_Init (void);							/* Initializes the LCD */
void LCD_X_SPI_8_Write00 (U8 c);						/* Write single command */
void LCD_X_SPI_8_Write01 (U8 c);						/* Write single data word */
void LCD_X_SPI_8_WriteM01 (U8 * pData, int NumWords);	/* Write multiple data words */
void LCD_X_SPI_8_WriteM00 (U8 * pData, int NumWords);
void LCD_X_SPI_8_ReadM01 (U8 * pData, int NumWords);	/* Read multiple data words */
void LCD_X_SPI_8_ReadM00 (U8 * pData, int NumWords);	/* Read multiple data words */
U8 LCD_X_SPI_8_Read01 (void);	/* Read multiple data words */
U8 LCD_X_SPI_8_Read00 (void);	/* Read multiple data words */

#ifdef PLACAINFO2

#define LCD_CS_HIGH  Chip_GPIO_SetPinOutHigh(LPC_GPIO, 1, 21)
#define LCD_CS_LOW  Chip_GPIO_SetPinOutLow(LPC_GPIO, 1, 21)
#define LCD_CD_LOW  Chip_GPIO_SetPinOutLow(LPC_GPIO, 1, 27)
#define LCD_CD_HIGH  Chip_GPIO_SetPinOutHigh(LPC_GPIO, 1, 27)
#define LCD_RESET_HIGH  Chip_GPIO_SetPinOutHigh(LPC_GPIO, 1, 29)
#define LCD_RESET_LOW  Chip_GPIO_SetPinOutLow(LPC_GPIO, 1, 29)

#else
/*
#define LCD_CS_HIGH  Chip_GPIO_SetPinOutHigh(LPC_GPIO, 0, 8)
#define LCD_CS_LOW  Chip_GPIO_SetPinOutLow(LPC_GPIO, 0, 8)
#define LCD_CD_LOW  Chip_GPIO_SetPinOutLow(LPC_GPIO, 0, 6)
#define LCD_CD_HIGH  Chip_GPIO_SetPinOutHigh(LPC_GPIO, 0, 6)
#define LCD_RESET_HIGH  Chip_GPIO_SetPinOutHigh(LPC_GPIO, 0, 7)
#define LCD_RESET_LOW  Chip_GPIO_SetPinOutLow(LPC_GPIO, 0, 7)

*/

#define LCD_CS_HIGH  Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_CS)
#define LCD_CS_LOW  Chip_GPIO_SetPinOutLow(LPC_GPIO, TFT_CS)
#define LCD_CD_LOW  Chip_GPIO_SetPinOutLow(LPC_GPIO, TFT_DC)
#define LCD_CD_HIGH  Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_DC)
#define LCD_RESET_HIGH  Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_RESET)
#define LCD_RESET_LOW  Chip_GPIO_SetPinOutLow(LPC_GPIO, TFT_RESET)

#endif

#endif /* LCD_X_SPI_H_ */
