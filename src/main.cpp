/*
  Programa para pruebas con el gateway lora "IOT Lora Gateway Hat", basado en el RAK22477
  Modificado para trabajar con un ESP32
  Empresa: Diacsa
  Autor: Ismael Takayama

    Se uso la libreria Lora_gateway de Semtech
    Datasheets:
    Sx1301: https://semtech.my.salesforce.com/sfc/p/#E0000000JelG/a/44000000MDnR/Et1KWLCuNDI6MDagfSPAvqqp.Y869Flgs1LleWyfjDY

  El programa recepciona los datos de los canales establecidos en el archivo loragw_conf.c y devuelve un mensaje
  de confirmacion en el mismo canal y con el mismo SP y CR.

  Los mensajes pueden ser enviados con cualquier SP y CR, pero debe estar dentro de los canales establecidos. Ademas, deberan
  tener el sync word de 0x34 y el crc activado.

  Los canales establecidos son (MHz):
  - 916.8
  - 917
  - 917.2
  - 917.4
  - 917.6
  - 917.8
  - 918
  - 918.2
  - 917.5 (BW 500KHz, SF 8)

  Los canales se pueden modificar en el documento loragw_conf.c

*/

#include <stdint.h>     /* C99 types */
#include <stdbool.h>    /* bool type */
#include <stdio.h>      /* printf fprintf sprintf fopen fputs */

#include <string.h>     /* memset */
#include <signal.h>     /* sigaction */
#include <time.h>       /* time clock_gettime strftime gmtime clock_nanosleep*/
#include <unistd.h>     /* getopt access */
#include <stdlib.h>     /* atoi */
#include <arduino.h>  

#include "loragw_conf.h"
#include "loragw_hal.h"
#include "loragw_reg.h"
#include "loragw_aux.h"

/* -------------------------------------------------------------------------- */
/* --- MACROS PRIVADAS ------------------------------------------------------- */

char dbug_msg[100];

#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
#define MSG(args...)    {\
                          memset(dbug_msg, 0, sizeof(dbug_msg));\
                          sprintf(dbug_msg,"loragw_pkt_logger: ",args);\
                          Serial.print(dbug_msg);\
                          }




void setup() {
  Serial.begin(115200);
}

void loop() {

}