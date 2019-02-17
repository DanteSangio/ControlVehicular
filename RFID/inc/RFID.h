
#define MIN_BALANCE 300

#include "MFRC522.h"

void userTapIn(MFRC522Ptr_t mfrc);
void ConvIntaChar(uint32_t informacionRFID,char* auxRfid);
void Comparar(unsigned int tarjeta);/* Compara dos numeros binarios*/

