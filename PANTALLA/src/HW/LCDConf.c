/****************************************************************************
 * @file     LCDConf.c
 * @brief    Display controller configuration
 * @version  1.0
 * @date     09. May. 2012
 *
 * @note
 * Copyright (C) 2012 NXP Semiconductors(NXP), All rights reserved.
 */

#include "GUI.h"
#include "Chip.h"
#include "LCD_X_SPI.h"
#include "LCD_X_SPI.h"
#include "FreeRTOS.h"

#ifndef _WINDOWS
#include "GUIDRV_FlexColor.h"
#endif

/*********************************************************************
*
*       Layer configuration
*
*
*
**********************************************************************
*/





//
// Color depth
//
#define LCD_BITSPERPIXEL 16 /* Currently the values 16 and 18 are supported */
//
// Physical display size
//
#define XSIZE_PHYS 320
#define YSIZE_PHYS 240

//
// Color conversion
//
#define COLOR_CONVERSION GUICC_565

//
// Display driver
//
#define DISPLAY_DRIVER GUIDRV_FLEXCOLOR

//
// Orientation
//
//#define DISPLAY_ORIENTATION (0)
//#define DISPLAY_ORIENTATION (GUI_MIRROR_X)
//#define DISPLAY_ORIENTATION (GUI_MIRROR_Y)
//#define DISPLAY_ORIENTATION (GUI_MIRROR_X | GUI_MIRROR_Y)
#define DISPLAY_ORIENTATION (GUI_SWAP_XY)
//#define DISPLAY_ORIENTATION (GUI_MIRROR_X | GUI_SWAP_XY)
//#define DISPLAY_ORIENTATION (GUI_MIRROR_Y | GUI_SWAP_XY)
//#define DISPLAY_ORIENTATION (GUI_MIRROR_X | GUI_MIRROR_Y | GUI_SWAP_XY)

/*********************************************************************
*
*       Configuration checking
*
**********************************************************************
*/
#ifndef   VXSIZE_PHYS
  #define VXSIZE_PHYS XSIZE_PHYS
#endif
#ifndef   VYSIZE_PHYS
  #define VYSIZE_PHYS YSIZE_PHYS
#endif
#ifndef   XSIZE_PHYS
  #error Physical X size of display is not defined!
#endif
#ifndef   YSIZE_PHYS
  #error Physical Y size of display is not defined!
#endif
#ifndef   COLOR_CONVERSION
  #error Color conversion not defined!
#endif
#ifndef   DISPLAY_DRIVER
  #error No display driver defined!
#endif
#ifndef   DISPLAY_ORIENTATION
  #define DISPLAY_ORIENTATION 0
#endif

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

//#define wr_reg(reg, data) LCD_X_SPI_Write00(reg); LCD_X_SPI_Write01(data);

/*********************************************************************
*
*       _InitController
*
* Purpose:
*   Initializes the display controller
*/
static void _InitController(void) {
#ifndef WIN32

vTaskDelay( 10/portTICK_PERIOD_MS );

  LCD_X_SPI_Init();

  vTaskDelay( 10/portTICK_PERIOD_MS );

//Forma 3 de inic
  LCD_RESET_HIGH;
  vTaskDelay( 5/portTICK_PERIOD_MS );

  LCD_RESET_LOW;

  vTaskDelay( 5/portTICK_PERIOD_MS );
  LCD_RESET_HIGH;
  //LCD_X_SPI_8_Write00(0x01);//soft reset
 vTaskDelay( 2000/portTICK_PERIOD_MS );


  LCD_X_SPI_8_Write00(0xCB);
  LCD_X_SPI_8_Write01(0x39);
  LCD_X_SPI_8_Write01(0x2C);
  LCD_X_SPI_8_Write01(0x00);
  LCD_X_SPI_8_Write01(0x34);
  LCD_X_SPI_8_Write01(0x02);

  LCD_X_SPI_8_Write00(0xCF);
  LCD_X_SPI_8_Write01(0x00);
  LCD_X_SPI_8_Write01(0XC1);
  LCD_X_SPI_8_Write01(0X30);

  LCD_X_SPI_8_Write00(0xE8);
  LCD_X_SPI_8_Write01(0x85);
  LCD_X_SPI_8_Write01(0x00);
  LCD_X_SPI_8_Write01(0x78);

  LCD_X_SPI_8_Write00(0xEA);
  LCD_X_SPI_8_Write01(0x00);
  LCD_X_SPI_8_Write01(0x00);

  LCD_X_SPI_8_Write00(0xED);
  LCD_X_SPI_8_Write01(0x64);
  LCD_X_SPI_8_Write01(0x03);
  LCD_X_SPI_8_Write01(0X12);
  LCD_X_SPI_8_Write01(0X81);

  LCD_X_SPI_8_Write00(0xF7);
  LCD_X_SPI_8_Write01(0x20);

  LCD_X_SPI_8_Write00(0xC0);    //Power control
  LCD_X_SPI_8_Write01(0x23);   //VRH[5:0]

  LCD_X_SPI_8_Write00(0xC1);    //Power control
  LCD_X_SPI_8_Write01(0x10);   //SAP[2:0];BT[3:0]

  LCD_X_SPI_8_Write00(0xC5);    //VCM control
  LCD_X_SPI_8_Write01(0x3e); //�Աȶȵ���
  LCD_X_SPI_8_Write01(0x28);

  LCD_X_SPI_8_Write00(0xC7);    //VCM control2
  LCD_X_SPI_8_Write01(0x86);  //--

  LCD_X_SPI_8_Write00(0x36);    // Memory Access Control
  LCD_X_SPI_8_Write01(0x48); //C8	   //48 68����//28 E8 ����

  LCD_X_SPI_8_Write00(0x3A);
  LCD_X_SPI_8_Write01(0x55);

  LCD_X_SPI_8_Write00(0xB1);
  LCD_X_SPI_8_Write01(0x00);
  LCD_X_SPI_8_Write01(0x18);

  LCD_X_SPI_8_Write00(0xB6);    // Display Function Control
  LCD_X_SPI_8_Write01(0x08);
  LCD_X_SPI_8_Write01(0x82);
  LCD_X_SPI_8_Write01(0x27);

  LCD_X_SPI_8_Write00(0xF2);    // 3Gamma Function Disable
  LCD_X_SPI_8_Write01(0x00);

  LCD_X_SPI_8_Write00(0x26);    //Gamma curve selected
  LCD_X_SPI_8_Write01(0x01);

  LCD_X_SPI_8_Write00(0x11);    //Exit Sleep

  vTaskDelay( 1000/portTICK_PERIOD_MS );

  LCD_X_SPI_8_Write00(0x29);    //Display on

  vTaskDelay( 1000/portTICK_PERIOD_MS );
  //LCD_X_SPI_8_Write00(0x2c);



#endif
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       LCD_X_Config
*
* Purpose:
*   Called during the initialization process in order to set up the
*   display driver configuration.
*
*/
void LCD_X_Config(void) {
  GUI_DEVICE * pDevice;
  GUI_PORT_API PortAPI;
  CONFIG_FLEXCOLOR Config = {0};

  //
  // Set display driver and color conversion for 1st layer
  //
#ifndef _WINDOWS
  pDevice = GUI_DEVICE_CreateAndLink(DISPLAY_DRIVER, COLOR_CONVERSION, 0, 0);
#else
  pDriver = GUIDRV_WIN32;
#endif
  //
  // Display driver configuration, required for Lin-driver
  //
  if (DISPLAY_ORIENTATION & GUI_SWAP_XY) {
    LCD_SetSizeEx (0, YSIZE_PHYS,   XSIZE_PHYS);
    LCD_SetVSizeEx(0, VYSIZE_PHYS,  VXSIZE_PHYS);
  } else {
    LCD_SetSizeEx (0, XSIZE_PHYS,   YSIZE_PHYS);
    LCD_SetVSizeEx(0, VXSIZE_PHYS,  VYSIZE_PHYS);
  }

  //
  // Function pointers for 8 bit interface
  //
#ifndef _WINDOWS
  /*
  PortAPI.pfWrite16_A0  = LCD_X_SPI_Write00;
  PortAPI.pfWrite16_A1  = LCD_X_SPI_Write01;
  PortAPI.pfWriteM16_A1 = LCD_X_SPI_WriteM01;
  PortAPI.pfReadM16_A1  = LCD_X_SPI_ReadM01;
 */
  PortAPI.pfWrite8_A0  = LCD_X_SPI_8_Write00;
  PortAPI.pfWrite8_A1  = LCD_X_SPI_8_Write01;
  PortAPI.pfWriteM8_A0 = LCD_X_SPI_8_WriteM00;
  PortAPI.pfWriteM8_A1 = LCD_X_SPI_8_WriteM01;
  PortAPI.pfRead8_A0   = LCD_X_SPI_8_Read00;
  PortAPI.pfRead8_A1   = LCD_X_SPI_8_Read01;
  PortAPI.pfReadM8_A0  = LCD_X_SPI_8_ReadM00;
  PortAPI.pfReadM8_A1  = LCD_X_SPI_8_ReadM01;


  //GUIDRV_FlexColor_SetFunc(pDevice, &PortAPI, GUIDRV_FLEXCOLOR_F66708, GUIDRV_FLEXCOLOR_M16C0B16);
    GUIDRV_FlexColor_SetFunc(pDevice, &PortAPI, GUIDRV_FLEXCOLOR_F66709, GUIDRV_FLEXCOLOR_M16C0B8);

  //
  // Orientation
  //
  Config.Orientation = DISPLAY_ORIENTATION;
  Config.NumDummyReads = -1;		/* 5 dummy bytes are required when reading GRAM by SPI. 1 byte is read in LCD_X_SPI_WriteM01, so 4 bytes are left */
  GUIDRV_FlexColor_Config(pDevice, &Config);
#endif
}

/*********************************************************************
*
*       LCD_X_DisplayDriver
*
* Purpose:
*   This function is called by the display driver for several purposes.
*   To support the according task the routine needs to be adapted to
*   the display controller. Please note that the commands marked with
*   'optional' are not cogently required and should only be adapted if
*   the display controller supports these features.
*
* Parameter:
*   LayerIndex - Index of layer to be configured
*   Cmd        - Please refer to the details in the switch statement below
*   pData      - Pointer to a LCD_X_DATA structure
*/
int LCD_X_DisplayDriver(unsigned LayerIndex, unsigned Cmd, void * pData) {
  int r;

  GUI_USE_PARA(LayerIndex);
  GUI_USE_PARA(pData);
  switch (Cmd) {
  //
  // Required
  //
  case LCD_X_INITCONTROLLER: {
    //
    // Called during the initialization process in order to set up the
    // display controller and put it into operation. If the display
    // controller is not initialized by any external routine this needs
    // to be adapted by the customer...
    //
    _InitController();
    return 0;
  }
  default:
    r = -1;
  }
  return r;
}

/*************************** End of file ****************************/
