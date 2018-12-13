/* tarea8.c
 *
 *  Created on: 6 de oct. de 2017
 *      Author: Kenji
 */


#include "Pantalla.h"
//#include "tarea1.h"



WM_HWIN CreateWindow(void);



void vTaskPantalla(void *pvParameters)
{


	uint8_t op,menu=EST_MEDICION,gananterior;
	static uint8_t PC_config=0; // empieza en el menu de medicion
	char cadena[16];

	GUI_COLOR colorfondoboton;

	BUTTON_Handle botona,botonb,botonc,botond,botone;

	GRAPH_Handle grafico;
	GRAPH_SCALE_Handle escalagrafh,escalagrafv;
	GRAPH_DATA_Handle datagraf;

	short int array[250];
	int i=250;

	while(i!=0)
	{
		i--;

			array[i]=0;

	}

	GUI_Init();
	GUI_SetBkColor(0x00D3D3D3); //gris claro
	GUI_Clear();

	GUI_SetFont(GUI_FONT_COMIC18B_ASCII);
	GUI_DispStringAt("T.P. Tecnicas Digitales 2 UTN FRBA",20,0);
	GUI_DispStringAt("Control Vehicular",105,25);
	GUI_DispStringAt("Sanata Romeo",143,60);
	//GUI_DispStringAt("Ponderacion en tiempo:",10,95);
	//GUI_DispStringAt("Ponderacion en frecuencia:",10,115);
	//GUI_DispStringAt("Slow",175,95);
	//GUI_DispStringAt("A",203,115);






/*

	//dibujo linea que separa la medicion de las opciones
	GUI_DrawHLine(140,0,320);

	BUTTON_SetDefaultFont(GUI_FONT_COMIC18B_ASCII);
	BUTTON_SetDefaultTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
	GUI_SetFont(GUI_FONT_COMIC18B_ASCII);
	botona=BUTTON_CreateEx(5,145,150,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON0);
	//BUTTON_SetTextAling(botona,GUI_TA_HCENTER | GUI_TA_VCENTER);
	BUTTON_SetText(botona, "FAST/SLOW");
	botonb=BUTTON_CreateEx(165,145,150,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON1);
	BUTTON_SetText(botonb, "A/C");
	//GUI_SetTextAling(botona,GUI_TA_HCENTER | GUI_TA_VCENTER);
	//GUI_SetFont(botona,GUI_FONT_COMIC18B_ASCII);
	botonc=BUTTON_CreateEx(5,195,150,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON2);
	BUTTON_SetText(botonc, "GRAFICO");
	botond=BUTTON_CreateEx(165,195,150,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON3);
	BUTTON_SetText(botond, "OPCIONES");
	BUTTON_SetBkColor(botona,BUTTON_CI_UNPRESSED,0x00FFFF80);*/
	GUI_Exec();

	colorfondoboton=BUTTON_GetDefaultBkColor(BUTTON_CI_UNPRESSED);

	//borro pantalla
	//GUI_SetBkColor(0x0080FF80);//Verde
	//GUI_SetBkColor(0x00D3D3D3); //gris claro
	//GUI_Clear();

	grafico=GRAPH_CreateEx(0,5,310,180,0,WM_CF_HIDE,0,GUI_ID_GRAPH0);


		while(1)
		{
			xQueueReceive( xcolalcd, &op, portMAX_DELAY );

			if(op==11){PC_config=1; // para reutilizar codigo
			}else if(op==12||op==13){
				PC_config=0;
			}

			if(op==SELEC_A)
			{

				BUTTON_SetBkColor(botonb,BUTTON_CI_UNPRESSED,colorfondoboton);
				BUTTON_SetBkColor(botonc,BUTTON_CI_UNPRESSED,colorfondoboton);
				BUTTON_SetBkColor(botond,BUTTON_CI_UNPRESSED,colorfondoboton);

			if(PC_config)
				BUTTON_SetBkColor(botone,BUTTON_CI_UNPRESSED,colorfondoboton);

				BUTTON_SetBkColor(botona,BUTTON_CI_UNPRESSED,0x00FFFF80);
				GUI_Exec();
			}
			if(op==SELEC_B)
			{

				BUTTON_SetBkColor(botona,BUTTON_CI_UNPRESSED,colorfondoboton);
				BUTTON_SetBkColor(botonc,BUTTON_CI_UNPRESSED,colorfondoboton);
				BUTTON_SetBkColor(botond,BUTTON_CI_UNPRESSED,colorfondoboton);

				if(PC_config)
				BUTTON_SetBkColor(botone,BUTTON_CI_UNPRESSED,colorfondoboton);

				BUTTON_SetBkColor(botonb,BUTTON_CI_UNPRESSED,0x00FFFF80);
				GUI_Exec();
			}
			if(op==SELEC_C)
			{
				BUTTON_SetBkColor(botonb,BUTTON_CI_UNPRESSED,colorfondoboton);
				BUTTON_SetBkColor(botona,BUTTON_CI_UNPRESSED,colorfondoboton);
				BUTTON_SetBkColor(botond,BUTTON_CI_UNPRESSED,colorfondoboton);

				if(PC_config)
				BUTTON_SetBkColor(botone,BUTTON_CI_UNPRESSED,colorfondoboton);

				BUTTON_SetBkColor(botonc,BUTTON_CI_UNPRESSED,0x00FFFF80);
				GUI_Exec();
			}
			if(op==SELEC_D)
			{
				BUTTON_SetBkColor(botonb,BUTTON_CI_UNPRESSED,colorfondoboton);
				BUTTON_SetBkColor(botonc,BUTTON_CI_UNPRESSED,colorfondoboton);
				BUTTON_SetBkColor(botona,BUTTON_CI_UNPRESSED,colorfondoboton);


				if(PC_config)BUTTON_SetBkColor(botone,BUTTON_CI_UNPRESSED,colorfondoboton);

				BUTTON_SetBkColor(botond,BUTTON_CI_UNPRESSED,0x00FFFF80);
				GUI_Exec();
			}
			if(op==SELEC_E)
			{
				BUTTON_SetBkColor(botonb,BUTTON_CI_UNPRESSED,colorfondoboton);
				BUTTON_SetBkColor(botonc,BUTTON_CI_UNPRESSED,colorfondoboton);
				BUTTON_SetBkColor(botona,BUTTON_CI_UNPRESSED,colorfondoboton);
				BUTTON_SetBkColor(botond,BUTTON_CI_UNPRESSED,colorfondoboton);

				BUTTON_SetBkColor(botone,BUTTON_CI_UNPRESSED,0x00FFFF80);
				GUI_Exec();
			}


			if(op==4)
			{
				GUI_DispStringAtCEOL("Slow",175,95);
			}
			if(op==5)
			{
				GUI_DispStringAtCEOL("Fast",175,95);
			}
			if(op==6)
			{
				GUI_DispStringAtCEOL("A",203,115);
			}
			if(op==7)
			{
				GUI_DispStringAtCEOL("C",203,115);
			}


			if(op==10)
			{
				 LCD_X_SPI_8_Write00(0x28);

				 LCD_X_SPI_8_Write00(0x10);
			}
			if(op==11)
			{

				WM_DeleteWindow(botona);
				WM_DeleteWindow(botonb);
				WM_DeleteWindow(botonc);
				WM_DeleteWindow(botond);
				WM_DeleteWindow(grafico);

				GUI_SetBkColor(0x0080FF80);//Verde
				GUI_Clear();
				GUI_Exec();
				//MUESTRO EN PANTALLA EL MENU DE PC

				grafico=GRAPH_CreateEx(10,5,300,180,0,WM_CF_HIDE,0,GUI_ID_GRAPH0);

				botona=BUTTON_CreateEx(20,0,280,38,0,WM_CF_SHOW,0,GUI_ID_BUTTON1);
				BUTTON_SetText(botona, "Guardar en memoria");

				botonb=BUTTON_CreateEx(20,43,280,38,0,WM_CF_SHOW,0,GUI_ID_BUTTON1);
				BUTTON_SetText(botonb, "Guardar en PC");

				botonc=BUTTON_CreateEx(20,86,280,38,0,WM_CF_SHOW,0,GUI_ID_BUTTON1);
				BUTTON_SetText(botonc, "Borrar memoria");

				botone=BUTTON_CreateEx(20,129,280,38,0,WM_CF_SHOW,0,GUI_ID_BUTTON1);
				BUTTON_SetText(botone, "Configurar hora");

				botond=BUTTON_CreateEx(20,172,280,38,0,WM_CF_SHOW,0,GUI_ID_BUTTON1);
				BUTTON_SetText(botond, "Volver");

				//GUI_DrawHLine(225,0,320); // antes 220
				//GUI_DispStringAt("Estado:",20,221);
				GUI_DispStringAtCEOL("Esperando seleccion",5,217); //antes 221
				BUTTON_SetBkColor(botona,BUTTON_CI_UNPRESSED,0x00FFFF80);
				GUI_Exec();
			}
			if(op==8)
			{
				GUI_DispStringAtCEOL("Guar. en memoria correctamente",5,217);
			}
			if(op==9)
			{
				GUI_DispStringAtCEOL("Guard. en PC correctamente",5,217);
			}
			if(op==14)
			{
				GUI_DispStringAtCEOL("Memoria borrada",5,217);
			}
			if(op==35)
			{
				GUI_DispStringAtCEOL("No hay sesiones para guardar",5,217);
			}
			if(op==36)
			{
				GUI_DispStringAtCEOL("Memoria llena",5,217);
			}

			if(op==12)
			{
				//grafico menu de medicion
				menu=EST_MEDICION;

				WM_DeleteWindow(botona);
				WM_DeleteWindow(botonb);
				WM_DeleteWindow(botonc);
				WM_DeleteWindow(botond);

				WM_DeleteWindow(botone);

				WM_DeleteWindow(grafico);


				GUI_SetBkColor(0x00D3D3D3); //gris claro
				GUI_Clear();
				GUI_Exec();

				grafico=GRAPH_CreateEx(10,5,300,180,0,WM_CF_HIDE,0,GUI_ID_GRAPH0);

				GUI_DispStringAt("T.P. Tecnicas Digitales 2 UTN FRBA",20,0);
				GUI_DispStringAt("Sonometro",105,25);
				GUI_DispStringAt("dB SPL",190,60);
				GUI_DispStringAt("Ponderacion en tiempo:",10,95);
				GUI_DispStringAt("Ponderacion en frecuencia:",10,115);

				if(ultimo_estado==3)
				{
					GUI_DispStringAt("Slow",175,95);
					GUI_DispStringAt("A",203,115);
				}
				else if(ultimo_estado==4)
				{
					GUI_DispStringAt("Slow",175,95);
					GUI_DispStringAt("C",203,115);
				}
				else if(ultimo_estado==5)
				{
					GUI_DispStringAt("Fast",175,95);
					GUI_DispStringAt("A",203,115);
				}
				else
				{
					GUI_DispStringAt("Fast",175,95);
					GUI_DispStringAt("C",203,115);
				}


				GUI_SetFont(GUI_FONT_COMIC18B_ASCII);
				botona=BUTTON_CreateEx(5,145,150,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON0);
				BUTTON_SetText(botona, "FAST/SLOW");
				botonb=BUTTON_CreateEx(165,145,150,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON1);
				BUTTON_SetText(botonb, "A/C");
				botonc=BUTTON_CreateEx(5,195,150,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON2);
				BUTTON_SetText(botonc, "GRAFICO");
				botond=BUTTON_CreateEx(165,195,150,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON3);
				BUTTON_SetText(botond, "OPCIONES");
				BUTTON_SetBkColor(botona,BUTTON_CI_UNPRESSED,0x00FFFF80);
				GUI_Exec();

			}
			if(op==13)
			{
				menu=EST_GRAFICO;
				WM_DeleteWindow(botona);
				WM_DeleteWindow(botonb);
				WM_DeleteWindow(botonc);
				WM_DeleteWindow(botond);

				WM_DeleteWindow(grafico);

				GUI_SetBkColor(0x00D3D3D3); //gris claro
				GUI_Clear();


				GUI_DrawHLine(190,0,320);
				botona=BUTTON_CreateEx(20,195,280,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON1);
				BUTTON_SetText(botona, "Volver");
				botonb=BUTTON_CreateEx(165,145,150,40,0,WM_CF_HIDE,0,GUI_ID_BUTTON1);
				BUTTON_SetText(botonb, "A/C");
				botonc=BUTTON_CreateEx(5,195,150,40,0,WM_CF_HIDE,0,GUI_ID_BUTTON2);
				BUTTON_SetText(botonc, "GRAFICO");
				botond=BUTTON_CreateEx(165,195,150,40,0,WM_CF_HIDE,0,GUI_ID_BUTTON3);
				BUTTON_SetText(botond, "OPCIONES");

				BUTTON_SetBkColor(botona,BUTTON_CI_UNPRESSED,0x00FFFF80);
				GUI_Exec();

				grafico=GRAPH_CreateEx(0,5,310,180,0,WM_CF_SHOW,0,GUI_ID_GRAPH0);
				//escalagrafh= GRAPH_SCALE_Create(170,GUI_TA_HCENTER,GRAPH_SCALE_CF_HORIZONTAL,50);
				//GRAPH_SCALE_SetFactor(escalagrafh,0.214228);
				//GRAPH_AttachScale(grafico,  escalagrafh);
				//==jhgjescalagrafv= GRAPH_SCALE_Create(5,GUI_TA_LEFT | GUI_TA_TOP,GRAPH_SCALE_CF_VERTICAL,30);
				//escalagrafv= GRAPH_SCALE_Create(0,GUI_TA_BOTTOM | GUI_TA_LEFT ,GRAPH_SCALE_CF_VERTICAL,30);
				//GRAPH_SCALE_SetFactor(escalagrafv,0.6667);
				//GRAPH_AttachScale(grafico,  escalagrafv);



				datagraf=GRAPH_DATA_YT_Create(0x0080FF80,310,array,10);

				GRAPH_SetBorder(grafico,30,0,0,0);


				if(Chip_GPIO_GetPinState(LPC_GPIO , PORT(0), PIN(2)))
				{
					//ESCALA DE 55 a 85: saltos de a 5 (30 pixeles)

					escalagrafv= GRAPH_SCALE_Create(20,GUI_TA_RIGHT,GRAPH_SCALE_CF_VERTICAL,30);
					GRAPH_SCALE_SetFactor(escalagrafv,0.16666667);
					GRAPH_SCALE_SetNumDecs(escalagrafv,0);
					GRAPH_SCALE_SetOff(escalagrafv,-340);
					GRAPH_AttachScale(grafico,  escalagrafv);
					gananterior=1;

				}
				else
				{
					//ESCALA DE 75 a 115: saltos de a 10 (45 pixeles)
					escalagrafv= GRAPH_SCALE_Create(20,GUI_TA_RIGHT || GUI_TA_BOTTOM,GRAPH_SCALE_CF_VERTICAL,45);
					GRAPH_SCALE_SetOff(escalagrafv,-337);
					GRAPH_SCALE_SetFactor(escalagrafv,0.22222222);
					GRAPH_SCALE_SetNumDecs(escalagrafv,0);

					GRAPH_AttachScale(grafico,  escalagrafv);
					gananterior=0;
				}

				GRAPH_AttachData(grafico,datagraf);



				//	GRAPH_SetGridVis(grafico,1);
				//GRAPH_SetGridDistY(grafico,30);

	/*
				GUI_DispDecAt(20, 5 , 155, 2);
				GUI_DispDecAt(40, 5 , 125, 2);
				GUI_DispDecAt(60, 5 , 95, 2);
				GUI_DispDecAt(80, 5 , 65, 2);
				GUI_DispDecAt(100, 0 , 35, 3);
				GUI_DispDecAt(120, 0 , 5, 3);
//este de arriba es que iba antes

				GUI_DispDecAt(120, 0 , 65, 3);
				GUI_DispDecAt(20, 5 , 165, 2);
				GUI_DispDecAt(40, 5 , 145, 2);
				GUI_DispDecAt(60, 5 , 125, 2);
				GUI_DispDecAt(80, 5 , 105, 2);
				GUI_DispDecAt(100, 0 , 85, 3);
*/




				GUI_Exec();

			}

			if(op==15){//grafico menu de Configuracion de Hora

				WM_DeleteWindow(botona);
				WM_DeleteWindow(botonb);
				WM_DeleteWindow(botonc);
				WM_DeleteWindow(botond);

				WM_DeleteWindow(botone);

				WM_DeleteWindow(grafico);


				GUI_SetBkColor(0x00D3D3D3); //gris claro
				GUI_Clear();
				GUI_Exec();

				grafico=GRAPH_CreateEx(10,5,300,180,0,WM_CF_HIDE,0,GUI_ID_GRAPH0);

				GUI_DispStringAt("Seleccione Hora:min",20,0);

				Chip_RTC_GetFullTime(LPC_RTC, pTime); // leo el valor actual de la hora

				GUI_SetFont(GUI_FONT_COMIC18B_ASCII );
				botona=BUTTON_CreateEx(5,145,150,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON0);
				sprintf(cadena,"%.2d",pTime->time[RTC_TIMETYPE_HOUR]);
				BUTTON_SetText(botona, cadena);

				botonb=BUTTON_CreateEx(165,145,150,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON1);
				sprintf(cadena,"%.2d",pTime->time[RTC_TIMETYPE_MINUTE]);
				BUTTON_SetText(botonb, cadena);

				BUTTON_SetBkColor(botona,BUTTON_CI_UNPRESSED,0x00FFFF80);
				GUI_Exec();


			}

			if(op==16) // para pantalla de configuracion de Hora ( boton izquierda)
			{
				BUTTON_SetBkColor(botonb,BUTTON_CI_UNPRESSED,colorfondoboton);
				BUTTON_SetBkColor(botona,BUTTON_CI_UNPRESSED,0x00FFFF80);

				GUI_Exec();
			}

			if(op==17) // para pantalla de configuracion de Hora ( boton derecha)
			{
				BUTTON_SetBkColor(botona,BUTTON_CI_UNPRESSED,colorfondoboton);
				BUTTON_SetBkColor(botonb,BUTTON_CI_UNPRESSED,0x00FFFF80);

				GUI_Exec();
			}

			if(op==18){//grafico menu de Configuracion de Fecha

				WM_DeleteWindow(botona);
				WM_DeleteWindow(botonb);

				WM_DeleteWindow(grafico);


				GUI_SetBkColor(0x00D3D3D3); //gris claro
				GUI_Clear();
				GUI_Exec();

				grafico=GRAPH_CreateEx(10,5,300,180,0,WM_CF_HIDE,0,GUI_ID_GRAPH0);

				GUI_DispStringAt("Seleccione dia/mes/Ano",20,0);

				Chip_RTC_GetFullTime(LPC_RTC, pTime); // leo el valor actual de la hora

				GUI_SetFont(GUI_FONT_COMIC18B_ASCII);
				botona=BUTTON_CreateEx(5,145,100,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON0);
				sprintf(cadena,"%.2d",pTime->time[RTC_TIMETYPE_DAYOFMONTH]);
				BUTTON_SetText(botona, cadena);

				botonb=BUTTON_CreateEx(105,145,100,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON1);
				sprintf(cadena,"%.2d",pTime->time[RTC_TIMETYPE_MONTH]);
				BUTTON_SetText(botonb, cadena);

				botonc=BUTTON_CreateEx(205,145,100,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON2);
				sprintf(cadena,"%.4d",pTime->time[RTC_TIMETYPE_YEAR]);
				BUTTON_SetText(botonc, cadena);


				BUTTON_SetBkColor(botona,BUTTON_CI_UNPRESSED,0x00FFFF80);
				GUI_Exec();


			}

			if(op==19) // para pantalla de configuracion de fecha ( boton derecha)
			{
				BUTTON_SetBkColor(botonb,BUTTON_CI_UNPRESSED,colorfondoboton);
				BUTTON_SetBkColor(botonc,BUTTON_CI_UNPRESSED,colorfondoboton);
				BUTTON_SetBkColor(botona,BUTTON_CI_UNPRESSED,0x00FFFF80);
				//op=
				GUI_Exec();
			}

			if(op==20) // para pantalla de configuracion de fecha ( boton medio)
			{
				BUTTON_SetBkColor(botona,BUTTON_CI_UNPRESSED,colorfondoboton);
				BUTTON_SetBkColor(botonc,BUTTON_CI_UNPRESSED,colorfondoboton);
				BUTTON_SetBkColor(botonb,BUTTON_CI_UNPRESSED,0x00FFFF80);

				GUI_Exec();
			}

			if(op==21) // para pantalla de configuracion de fecha ( boton derecha)
			{
				BUTTON_SetBkColor(botona,BUTTON_CI_UNPRESSED,colorfondoboton);
				BUTTON_SetBkColor(botonb,BUTTON_CI_UNPRESSED,colorfondoboton);
				BUTTON_SetBkColor(botonc,BUTTON_CI_UNPRESSED,0x00FFFF80);

				GUI_Exec();
			}

			if(op==23){//grafico menu de Configuracion de Hora

				Chip_RTC_GetFullTime(LPC_RTC, pTime); // leo el valor actual de la hora

				GUI_SetFont(GUI_FONT_COMIC18B_ASCII);

				sprintf(cadena,"%.2d",pTime->time[RTC_TIMETYPE_HOUR]);
				BUTTON_SetText(botona, cadena);

				sprintf(cadena,"%.2d",pTime->time[RTC_TIMETYPE_MINUTE]);
				BUTTON_SetText(botonb, cadena);

				GUI_Exec();
			}


			if(op==24){//grafico menu de Configuracion de Hora

				Chip_RTC_GetFullTime(LPC_RTC, pTime); // leo el valor actual de la hora

				GUI_SetFont(GUI_FONT_COMIC18B_ASCII);

				sprintf(cadena,"%.2d",pTime->time[RTC_TIMETYPE_DAYOFMONTH]);
				BUTTON_SetText(botona, cadena);

				sprintf(cadena,"%.2d",pTime->time[RTC_TIMETYPE_MONTH]);
				BUTTON_SetText(botonb, cadena);

				sprintf(cadena,"%.4d",pTime->time[RTC_TIMETYPE_YEAR]);
				BUTTON_SetText(botonc, cadena);

				GUI_Exec();
			}
			if(op==25)
			{
				if(menu==EST_MEDICION)
				{
					//PONER EL NUEVO VALOR;
					GUI_GotoXY(93,60);
					GUI_DispFloat(S1[S1i-1],5);


				}
				else if(menu==EST_GRAFICO)
				{
					//AGREGAR VALOR AL GRAFICO
					if(Chip_GPIO_GetPinState(LPC_GPIO , PORT(0), PIN(2)) && gananterior==0 )
					{
						GRAPH_DATA_YT_Clear(datagraf);
						gananterior=1;
						GRAPH_SCALE_Delete(escalagrafv);
						//ESCALA DE 55 a 85: saltos de a 5 (30 pixeles)

						escalagrafv= GRAPH_SCALE_Create(20,GUI_TA_RIGHT,GRAPH_SCALE_CF_VERTICAL,30);
						GRAPH_SCALE_SetFactor(escalagrafv,0.16666667);
						GRAPH_SCALE_SetNumDecs(escalagrafv,0);
						GRAPH_SCALE_SetOff(escalagrafv,-340);
						GRAPH_AttachScale(grafico,  escalagrafv);

					}
					if(!Chip_GPIO_GetPinState(LPC_GPIO , PORT(0), PIN(2)) && gananterior==1 )
					{
						GRAPH_DATA_YT_Clear(datagraf);

						gananterior=0;
						GRAPH_SCALE_Delete(escalagrafv);

						//ESCALA DE 75 a 115: saltos de a 10 (45 pixeles)
						escalagrafv= GRAPH_SCALE_Create(20,GUI_TA_RIGHT || GUI_TA_BOTTOM,GRAPH_SCALE_CF_VERTICAL,45);
						GRAPH_SCALE_SetOff(escalagrafv,-337);
						GRAPH_SCALE_SetFactor(escalagrafv,0.22222222);
						GRAPH_SCALE_SetNumDecs(escalagrafv,0);
						GRAPH_AttachScale(grafico,  escalagrafv);


					}
					if(gananterior==1)
					{
						 GRAPH_DATA_YT_AddValue(datagraf, (uint16_t)((S1[S1i-1]-56.7)*6));
						 GUI_Exec();
					}
					else
					{
						 GRAPH_DATA_YT_AddValue(datagraf, (uint16_t)((S1[S1i-1]-75)*4.6));
						 GUI_Exec();
					}


				}
			}

		}

}
