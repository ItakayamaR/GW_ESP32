/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2013 Semtech-Cycleo

Description:
    Host specific functions to address the LoRa concentrator registers through
    a SPI interface.
    Single-byte read/write and burst read/write.
    Does not handle pagination.
    Could be used with multiple SPI ports in parallel (explicit file descriptor)

License: Revised BSD License, see LICENSE.TXT file include in the project
Maintainer: Sylvain Miermont
*/


/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

#include <stdint.h>        /* C99 types */
#include <stdio.h>        /* printf fprintf */
#include <stdlib.h>        /* malloc free */
#include <unistd.h>        /* lseek, close */
#include <fcntl.h>        /* open */
#include <string.h>        /* memset */

#include <sys/ioctl.h>
#include <SPI.h>

#include "loragw_spi.h"
#include "loragw_hal.h"
#include "loragw_debug.h"   /* Activar mensajes seriales */


/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#if DEBUG_SPI == 1
   #define DEBUG_MSG(str)                Serial.print(str)
    #define DEBUG_PRINTF(fmt, args...)    {\
                                            memset(debug_msg, 0, sizeof(debug_msg));\
                                            sprintf(debug_msg,"%s:%d: " fmt, __func__, __LINE__, args);\
                                            Serial.print(debug_msg);\
                                            }
    #define CHECK_NULL(a)                 {\
                                            if(a==NULL){\
                                                memset(debug_msg, 0, sizeof(debug_msg));\
                                                sprintf(debug_msg,"%s:%d: ERROR: NULL POINTER AS ARGUMENT\n", __FUNCTION__, __LINE__);\
                                                Serial.print(debug_msg);\
                                                return LGW_SPI_ERROR;}\
                                          }
#else
    #define DEBUG_MSG(str)
    #define DEBUG_PRINTF(fmt, args...)
    #define CHECK_NULL(a)                if(a==NULL){return LGW_SPI_ERROR;}
#endif

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

#define READ_ACCESS     0x00
#define WRITE_ACCESS    0x80
#define SPI_SPEED       8000000
#define SPI_DEV_PATH    "/dev/spidev0.0"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */

SPISettings _spiSettings(LORA_DEFAULT_SPI_FREQUENCY, MSBFIRST, SPI_MODE0);
int _ss=LORA_DEFAULT_SS_PIN;
int _reset=LORA_DEFAULT_RESET_PIN;
int _sck=LORA_DEFAULT_SCK_PIN;
int _miso=LORA_DEFAULT_MISO_PIN;
int _mosi=LORA_DEFAULT_MOSI_PIN;


/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS DEFINITION ------------------------------------------ */

/* SPI initialization and configuration */
int lgw_spi_open(SPIClass** spi_target_ptr) {
    // setup pins
    pinMode(_ss, OUTPUT);
    digitalWrite(_ss, HIGH);
    
    if (_reset!= -1) {
        pinMode(_reset, OUTPUT);
        // perform reset
        digitalWrite(_reset, HIGH);
        delay(10);
        digitalWrite(_reset, LOW);
    }
    
    //puntero hacia el spi del sistema
    *spi_target_ptr=&LORA_DEFAULT_SPI;

    // start SPI
    (*spi_target_ptr)->begin(_sck, _miso, _mosi);


    if (*spi_target_ptr == NULL) {
        DEBUG_PRINTF("ERROR: failed to open SPI device %s\n", SPI_DEV_PATH);
        return LGW_SPI_ERROR;
    }

    DEBUG_MSG("Note: SPI port opened and configured ok\n");
    return LGW_SPI_SUCCESS;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* SPI release */
int lgw_spi_close(SPIClass *spi_target) {

    /* check input variables */
    CHECK_NULL(spi_target);

    // stop SPI
    spi_target->end();
    
    DEBUG_MSG("Note: SPI port closed\n");
    return LGW_SPI_SUCCESS;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* Simple write */
int lgw_spi_w(SPIClass *spi_target, uint8_t spi_mux_mode, uint8_t spi_mux_target, uint8_t address, uint8_t data) {
    int a=1;

    CHECK_NULL(spi_target);
    if ((address & 0x80) != 0) {
        DEBUG_MSG("WARNING: SPI address > 127\n");
    }

    /* I/O transaction */
    digitalWrite(_ss, LOW);

    spi_target->beginTransaction(_spiSettings);
    spi_target->transfer(WRITE_ACCESS | (address & 0x7F));
    a=spi_target->transfer(data);
    spi_target->endTransaction();

    digitalWrite(_ss, HIGH);

    /* determine return code */
    if (a != 0) {
        DEBUG_MSG("ERROR: SPI WRITE FAILURE\n");
        return LGW_SPI_ERROR;
    } else {
        DEBUG_MSG("Note: SPI write success\n");
        return LGW_SPI_SUCCESS;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* Simple read */
int lgw_spi_r(SPIClass *spi_target, uint8_t spi_mux_mode, uint8_t spi_mux_target, uint8_t address, uint8_t *data) {
    int a=270;

    /* check input variables */
    CHECK_NULL(spi_target);
    if ((address & 0x80) != 0) {
        DEBUG_MSG("WARNING: SPI address > 127\n");
    }
    CHECK_NULL(data);

    /* I/O transaction */
    digitalWrite(_ss, LOW);

    spi_target->beginTransaction(_spiSettings);
    spi_target->transfer(READ_ACCESS | (address & 0x7F));
    a=spi_target->transfer(0xFF);
    spi_target->endTransaction();

    digitalWrite(_ss, HIGH);

    //Serial.println(a);
    /* determine return code */
    if (a > 256) {
        DEBUG_MSG("ERROR: SPI WRITE FAILURE\n");
        return LGW_SPI_ERROR;
    } else {
        DEBUG_MSG("Note: SPI write success\n");
        (*data)= a;
        return LGW_SPI_SUCCESS;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* Burst (multiple-byte) write */
int lgw_spi_wb(SPIClass *spi_target, uint8_t spi_mux_mode, uint8_t spi_mux_target, uint8_t address, uint8_t *data, uint16_t size) {
    int i;
    int a=1;

    /* check input parameters */
    CHECK_NULL(spi_target);
    if ((address & 0x80) != 0) {
        DEBUG_MSG("WARNING: SPI address > 127\n");
    }
    CHECK_NULL(data);
    if (size == 0) {
        DEBUG_MSG("ERROR: BURST OF NULL LENGTH\n");
        return LGW_SPI_ERROR;
    }

    /* I/O transaction */
    digitalWrite(_ss, LOW);
    
    spi_target->beginTransaction(_spiSettings);
    a=spi_target->transfer(WRITE_ACCESS | (address & 0x7F));
    for (i=0; i < size; ++i) {
        a+=spi_target->transfer(*(data+i));
    }
    spi_target->endTransaction();
    digitalWrite(_ss, HIGH);

    DEBUG_PRINTF("BURST WRITE: Bytes transferidos: %d # bytes totales: %d \n", i, size);

    /* determine return code */
    if (a != 0) {
        DEBUG_MSG("ERROR: SPI BURST WRITE FAILURE\n");
        return LGW_SPI_ERROR;
    } else {
        DEBUG_MSG("Note: SPI burst write success\n");
        return LGW_SPI_SUCCESS;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* Burst (multiple-byte) read */
int lgw_spi_rb(SPIClass *spi_target, uint8_t spi_mux_mode, uint8_t spi_mux_target, uint8_t address, uint8_t *data, uint16_t size) {
    int i;

    /* check input parameters */
    CHECK_NULL(spi_target);
    if ((address & 0x80) != 0) {
        DEBUG_MSG("WARNING: SPI address > 127\n");
    }
    CHECK_NULL(data);
    if (size == 0) {
        DEBUG_MSG("ERROR: BURST OF NULL LENGTH\n");
        return LGW_SPI_ERROR;
    }

    /* I/O transaction */
    digitalWrite(_ss, LOW);
    
    spi_target->beginTransaction(_spiSettings);
    spi_target->transfer(READ_ACCESS | (address & 0x7F));
    for (i=0; i < size; ++i) {
        *(data+i)=spi_target->transfer(0);
    }
    spi_target->endTransaction();
    digitalWrite(_ss, HIGH);

    DEBUG_PRINTF("BURST READ: Bytes transferidos: %d # bytes totales: %d \n", i, size);


    /* determine return code */
    if (i != size) {
        DEBUG_MSG("ERROR: SPI BURST READ FAILURE\n");
        return LGW_SPI_ERROR;
    } else {
        DEBUG_MSG("Note: SPI burst read success\n");
        return LGW_SPI_SUCCESS;
    }
}

/* --- EOF ------------------------------------------------------------------ */
