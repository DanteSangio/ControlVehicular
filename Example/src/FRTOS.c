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




//#include "RegsLPC1769.h"



#define DEBUGOUT1(...) printf(__VA_ARGS__)
#define DEBUGOUT(...) printf(__VA_ARGS__)
#define DEBUGSTR(...) printf(__VA_ARGS__)


/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

static volatile bool fIntervalReached;
static volatile bool fAlarmTimeMatched;
static volatile bool On0, On1;

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Gets and shows the current time and date */
static void showTime(RTC_TIME_T *pTime)
{
	DEBUGOUT1("Time: %.2d:%.2d:%.2d %.2d/%.2d/%.4d\r\n",
			 pTime->time[RTC_TIMETYPE_HOUR],
			 pTime->time[RTC_TIMETYPE_MINUTE],
			 pTime->time[RTC_TIMETYPE_SECOND],
			 pTime->time[RTC_TIMETYPE_MONTH],
			 pTime->time[RTC_TIMETYPE_DAYOFMONTH],
			 pTime->time[RTC_TIMETYPE_YEAR]);
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	RTC interrupt handler
 * @return	Nothing
 */
void RTC_IRQHandler(void)
{
	uint32_t sec;

	/* Toggle heart beat LED for each second field change interrupt */
	if (Chip_RTC_GetIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE)) {
		/* Clear pending interrupt */
		Chip_RTC_ClearIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE);
		On0 = (bool) !On0;
		//Board_LED_Set(0, On0);
	}

	/* display timestamp every 5 seconds in the background */
	sec = Chip_RTC_GetTime(LPC_RTC, RTC_TIMETYPE_SECOND);
	if (!(sec % 5)) {
		fIntervalReached = true;	/* set flag for background */
	}

	/* Check for alarm match */
	if (Chip_RTC_GetIntPending(LPC_RTC, RTC_INT_ALARM)) {
		/* Clear pending interrupt */
		Chip_RTC_ClearIntPending(LPC_RTC, RTC_INT_ALARM);
		fAlarmTimeMatched = true;	/* set alarm handler flag */
	}
}

/**
 * @brief	Main entry point
 * @return	Nothing
 */
int main(void)
{
	RTC_TIME_T FullTime;

	//DEBUGOUT1("PRUEBA RTC..\n");	//Imprimo en la consola

	SystemCoreClockUpdate();
	//Board_Init();

	fIntervalReached  = 0;
	fAlarmTimeMatched = 0;
	On0 = On1 = false;
	//Board_LED_Set(2, false);

	DEBUGOUT1("PRUEBA RTC..\n");	//Imprimo en la consola

	DEBUGSTR("The RTC operates on a 1 Hz clock.\r\n" \
			 "Register writes can take up to 2 cycles.\r\n"	\
			 "It will take a few seconds to fully\r\n" \
			 "initialize it and start it running.\r\n\r\n");

	DEBUGSTR("We'll print a timestamp every 5 seconds.\r\n"	\
			 "...and another when the alarm occurs.\r\n");

	Chip_RTC_Init(LPC_RTC);

	/* Set current time for RTC 2:00:00PM, 2012-10-05 */
	FullTime.time[RTC_TIMETYPE_SECOND]  = 0;
	FullTime.time[RTC_TIMETYPE_MINUTE]  = 0;
	FullTime.time[RTC_TIMETYPE_HOUR]    = 14;
	FullTime.time[RTC_TIMETYPE_DAYOFMONTH]  = 5;
	FullTime.time[RTC_TIMETYPE_DAYOFWEEK]   = 5;
	FullTime.time[RTC_TIMETYPE_DAYOFYEAR]   = 279;
	FullTime.time[RTC_TIMETYPE_MONTH]   = 10;
	FullTime.time[RTC_TIMETYPE_YEAR]    = 2012;

	Chip_RTC_SetFullTime(LPC_RTC, &FullTime);

	/* Set ALARM time for 17 seconds from time */
	FullTime.time[RTC_TIMETYPE_SECOND]  = 17;
	Chip_RTC_SetFullAlarmTime(LPC_RTC, &FullTime);

	/* Set the RTC to generate an interrupt on each second */
	Chip_RTC_CntIncrIntConfig(LPC_RTC, RTC_AMR_CIIR_IMSEC, ENABLE);
//
	/* Enable matching for alarm for second, minute, hour fields only */
	Chip_RTC_AlarmIntConfig(LPC_RTC, RTC_AMR_CIIR_IMSEC | RTC_AMR_CIIR_IMMIN | RTC_AMR_CIIR_IMHOUR, ENABLE);

	/* Clear interrupt pending */
	Chip_RTC_ClearIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE | RTC_INT_ALARM);

	/* Enable RTC interrupt in NVIC */
	NVIC_EnableIRQ((IRQn_Type) RTC_IRQn);

	/* Enable RTC (starts increase the tick counter and second counter register) */
	Chip_RTC_Enable(LPC_RTC, ENABLE);

	/* Loop forever */
	while (1) {
		if (fIntervalReached) {	/* Every 5s */
			fIntervalReached = 0;

			On1 = (bool) !On1;
#if !defined(CHIP_LPC175X_6X)
			Board_LED_Set(1, On1);
#endif
			/* read and display time */
			Chip_RTC_GetFullTime(LPC_RTC, &FullTime);
			showTime(&FullTime);
		}

		if (fAlarmTimeMatched) {
			fAlarmTimeMatched = false;

			/* announce event */
			DEBUGSTR("ALARM triggered!\r\n");

			/* read and display time */
			Chip_RTC_GetFullTime(LPC_RTC, &FullTime);
			showTime(&FullTime);
		}
	}
}

/**
 * @}
 */
