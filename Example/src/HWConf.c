/*********************************************************************
*                SEGGER MICROCONTROLLER GmbH & Co. KG                *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (c) 2003-2012     SEGGER Microcontroller GmbH & Co KG       *
*                                                                    *
*        Internet: www.segger.com    Support:  support@segger.com    *
*                                                                    *
**********************************************************************

----------------------------------------------------------------------
File    : HWConf.c for NXP LPC1769 CPU and Keil MCB1700
          eval board.
Purpose : Initializes and handles the hardware for emWin without using
          any RTOS.
          Feel free to modify this file acc. to your target system.
--------  END-OF-HEADER  ---------------------------------------------
*/
#include "chip.h"
#include "HWConf.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
#define NVIC_VTOR           (*(volatile U32*)(0xE000ED08))

/*********************************************************************
*
*       Extern data
*
**********************************************************************
*/
extern volatile int TimeMS;  // Defined in GUI_X.c

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/
/*********************************************************************
*
*       SysTick_Handler()
*
* Function description
*   This is the code that gets called when the processor receives a
*   _SysTick exception.
*/
#ifdef __cplusplus
extern "C" {
#endif
void TIMER0_IRQHandler(void);      // Avoid warning, Systick_Handler is not prototyped in any CMSIS header
#ifdef __cplusplus
}
#endif



void TIMER0_IRQHandler(void)
{
	if (Chip_TIMER_MatchPending(LPC_TIMER0, 1)) {
		Chip_TIMER_ClearMatch(LPC_TIMER0, 1);
		//TimeMS++;
	}
}

/*********************************************************************
*
*       HW_X_Config()
*
* Function description
*   Initializes the hardware (timer).
*   May be modified, if an other timer should be used.
*/

#ifdef __ICCARM__
  typedef void( *intfunc )( void );
  typedef union { intfunc __fun; void * __ptr; } intvec_elem;
  extern const intvec_elem __vector_table[];
  #define __Vectors    __vector_table
#else
  extern unsigned char __Vectors;
#endif

void HW_X_Config(void) {
  uint32_t  TickPrio;
  uint32_t timerFreq;

  //
  // Assuming PLL and core clock were already set by SystemInit() (called
  // from the startup code), initializing any hardware is not necessary.
  // Just ensure that the system clock variable is updated and then
  // set the periodic system timer tick for embOS.
  //
  SystemCoreClockUpdate();                      // Ensure, the SystemCoreClock is set

  Chip_TIMER_Init(LPC_TIMER0);

  	/* Timer rate is system clock rate */
  	timerFreq = Chip_Clock_GetSystemClockRate();

  	/* Timer setup for match and interrupt at TICKRATE_HZ */
  	Chip_TIMER_Reset(LPC_TIMER0);
  	Chip_TIMER_MatchEnableInt(LPC_TIMER0, 1);
  	Chip_TIMER_SetMatch(LPC_TIMER0, 1, (timerFreq / 1000));
  	Chip_TIMER_ResetOnMatchEnable(LPC_TIMER0, 1);
  	Chip_TIMER_Enable(LPC_TIMER0);

  	/* Enable timer interrupt */
  	NVIC_ClearPendingIRQ(TIMER0_IRQn);
  	NVIC_EnableIRQ(TIMER0_IRQn);
  //
  // Initialize NVIC vector base address. Might be necessary for RAM targets
  // or application not running from 0.
  //
  // NVIC_VTOR = (U32)&__Vectors;
  //
  // Set the interrupt priority for the system timer to 2nd lowest level to
  // ensure the timer can preempt PendSV handler
  //
  NVIC_SetPriority(TIMER0_IRQn, (uint32_t) -1);
  TickPrio  = NVIC_GetPriority(TIMER0_IRQn);
  TickPrio -=1;
  NVIC_SetPriority(TIMER0_IRQn, (uint32_t)TickPrio);
}

/*********************************************************************
*
*       __low_level_init()
*
*       Initialize memory controller, clock generation and pll
*
*       Has to be modified, if another CPU clock frequency should be
*       used. This function is called during startup and
*       has to return 1 to perform segment initialization
*/
#ifdef __cplusplus
extern "C" {
#endif
int __low_level_init(void);  // Avoid "no ptototype" warning
#ifdef __cplusplus
  }
#endif
int __low_level_init(void) {
  SystemCoreClockUpdate();   // Ensure, the SystemCoreClock is set
  return 1;
}

/*****  End of file  ************************************************/
