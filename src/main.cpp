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
#include <time.h>       /* time clock_gettime strftime gmtime clock_nanosleep*/
#include <stdlib.h>     /* atoi */
#include <arduino.h>  

#include "loragw_conf.h"
#include "loragw_hal.h"
#include "loragw_reg.h"
#include "loragw_aux.h"

#include "cal_fw.var" /* external definition of the variable */

/* -------------------------------------------------------------------------- */
/* --- MACROS PRIVADAS ------------------------------------------------------- */
char dbug_msg[100];

#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
#define MSG(args...)    {\
                          memset(dbug_msg, 0, sizeof(dbug_msg));\
                          sprintf(dbug_msg,"loragw_pkt_logger: " args);\
                          Serial.print(dbug_msg);\
                          }
#define STOP_EXECUTION   while(1) delay(100)          

/* -------------------------------------------------------------------------- */
/* --- CONSTANTES PRIVADAS -------------------------------------------------- */

#define TX_RF_CHAIN                 0 /* TX Solo soportado para radio A */


/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES (GLOBAL) ------------------------------------------- */
uint64_t lgwm = 0; /* LoRa gateway MAC address */
char lgwm_str[17];

/* clock and log file management */
time_t now_time;
time_t log_start_time;

/* application parameters */
static int power = 27; /* 27 dBm by default */
int preamb = 8; /* 8 symbol preamble by default */
int pl_size = 2; /* 2 bytes payload by default */
uint32_t wait_time = 5E5; /*0.5 seconds between packets by default */
bool invert = false;


int sleep_time = 3; /* 3 ms */

/* clock and log rotation management */
int log_rotate_interval = 3600; /* by default, rotation every hour */
int time_check = 0; /* variable used to limit the number of calls to time() function */
unsigned long pkt_in_log = 0; /* count the number of packet written in each log file */


/* allocate memory for packet fetching and processing */
struct lgw_pkt_rx_s rxpkt[16]; /* array containing up to 16 inbound packets metadata */
struct lgw_pkt_rx_s *p; /* pointer on a RX packet */
int nb_pkt;

/* allocate memory for packet sending */
struct lgw_pkt_tx_s txpkt; /* array containing 1 outbound packet + metadata */

/* local timestamp variables until we get accurate GPS time */
struct timespec fetch_time;
char fetch_timestamp[30];
struct tm * x;


void setup() {
    int i;                              //Variables para loops y temporales
    Serial.begin(115200);               //Iniciamos la comunicación serial para debugeo
    
    parse_SX1301_configuration();       //Subimos las configuraciones de canal

    lgwm=parse_gateway_configuration();   //Obtenemos el ID del gateway

    configure_TxGainLUT();       //Configuramos las ganancias de transmisión

    i = lgw_start();

    
    if (i == LGW_HAL_SUCCESS) {
        MSG("INFO: concentrator started, packet can now be received\n");
    } else {
        MSG("ERROR: failed to start the concentrator\n");
        STOP_EXECUTION;
    }

    /* transform the MAC address into a string */
    sprintf(lgwm_str, "%08X%08X", (uint32_t)(lgwm >> 32), (uint32_t)(lgwm & 0xFFFFFFFF));
  
    /* opening log file*/
    time(&now_time);
}

void loop() {
    int i;                              //Variables para loops y temporales
    uint8_t status_var;

    /* fetch packets */
    nb_pkt = lgw_receive(ARRAY_SIZE(rxpkt), rxpkt);
    if (nb_pkt == LGW_HAL_ERROR) {
        MSG("ERROR: failed packet fetch, exiting\n");
        STOP_EXECUTION;
    } else if (nb_pkt == 0) {
        delay(sleep_time);                      /* Esperamos un periodo de tiempo si no hay paquetes */
    } else {
        MSG("%i Mensajes recibidos\n",nb_pkt);
    }

    /* log packets */
    for (i=0; i < nb_pkt; ++i) {
        p = &rxpkt[i];

        /* limpiamos la estructura de transmisión */
        memset(&txpkt, 0, sizeof(txpkt));

        /* Escribimos la frecuencia de transmisión (igual a la del mensaje recibido)*/
        txpkt.freq_hz = p->freq_hz;

        /* Modo de transmisión a un tiempo definido*/
        txpkt.tx_mode = TIMESTAMPED;  
        //txpkt.tx_mode=IMMEDIATE;

        /* Modo de transmisión a un tiempo definido*/
        txpkt.rf_chain = TX_RF_CHAIN;

        /* Potencia de transmisión (por defecto 27 dbm)*/
        txpkt.rf_power = power;    
        txpkt.modulation = MOD_LORA;

        /* Escribimos el BW (igual a la del mensaje recibido)*/
        txpkt.bandwidth = p->bandwidth;

        /* Escribimos el SP (igual a la del mensaje recibido)*/
        txpkt.datarate = p->datarate;

        /* Escribimos el CR (igual a la del mensaje recibido)*/
        txpkt.coderate = p->coderate;

        txpkt.invert_pol = invert;
        txpkt.preamble = preamb;
        txpkt.size = pl_size;

        /* Escribimos el Mensaje de confirmación*/
        strcpy((char *)txpkt.payload, "OK");

        txpkt.count_us = p->count_us + wait_time;

        //Empezamos a escribir en el registro los datos

        /* Si es que se ha recibido un mensaje con CRC correcto */
        if (p->status == STAT_CRC_OK) {
            /* Enviamos el mensaje de confirmacion */
            MSG("Sending OK\n");
            i = lgw_send(txpkt); /* non-blocking scheduling of TX packet */
            if (i == LGW_HAL_ERROR) {
                printf("ERROR\n");
                STOP_EXECUTION;
            } else {
                /* wait for packet to finish sending */
                i=0;
                do {
                    wait_ms(5);
                    i++;
                    lgw_status(TX_STATUS, &status_var); /* get TX status */
                    //printf("enviando\n");
                } while (status_var != TX_FREE && i<1000);
                if (i==5000){
                    printf("Error al enviar mensaje de confirmación\n");
                } else  {
                    printf("Se envió mensaje de confirmación\n");
                }
                
            }
        } else printf("CRC malo, no se enviará mensaje de confirmación\n");
        printf("\n");
    }

}