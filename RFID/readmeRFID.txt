RFID example

Se utilizo con el SSP1 y la conexion descripta abajo, realizado con tareas se configura el módulo y la ssp1 para luego
poder leer las tarjetas ingresadas, hay una función de comparación que lo hace contra una variable ya cargada. 
La lectura se realiza cada 1 seg.

- LPCXpresso LPC1769:

* SSP1 connection:
				- P0.4:  SSP1_SSEL
				- P0.5:  RST
				- P0.7:  SSP1_SCK 		 		
				- P0.8:  SSP1_MISO 		
				- P0.9:  SSP1_MOSI 		

Verificar pines rfid_utils.c 	setuprfids
