/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2013 Semtech-Cycleo

Description:
    Host specific functions to address the LoRa concentrator registers through a
    SPI interface.
    Single-byte read/write and burst read/write.
    Does not handle pagination.
    Could be used with multiple SPI ports in parallel (explicit file descriptor)

License: Revised BSD License, see LICENSE.TXT file include in the project
Maintainer: Sylvain Miermont
*/


#ifndef _LORAGW_SPI_H
#define _LORAGW_SPI_H

/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

#include <stdint.h>        /* C99 types*/
#include <arduino.h>   
#include <SPI.h>

#include "config.h"    /* library configuration options (dynamically generated) */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC CONSTANTS ----------------------------------------------------- */

#define LGW_SPI_SUCCESS     0
#define LGW_SPI_ERROR       -1
#define LGW_BURST_CHUNK     1024

#define LGW_SPI_MUX_MODE0   0x0     /* No FPGA */
#define LGW_SPI_MUX_TARGET_SX1301   0x0

#define LORA_DEFAULT_SPI           SPI
#define LORA_DEFAULT_SPI_FREQUENCY 8E6 
#define LORA_DEFAULT_SS_PIN        10
#define LORA_DEFAULT_RESET_PIN     17
#define LORA_DEFAULT_SCK_PIN       18
#define LORA_DEFAULT_MISO_PIN      19
#define LORA_DEFAULT_MOSI_PIN      23


/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS PROTOTYPES ------------------------------------------ */

/**
@brief LoRa concentrator SPI setup (configure I/O and peripherals)
@param spi_target_ptr pointer on a generic pointer to SPI target (implementation dependant)
@return status of register operation (LGW_SPI_SUCCESS/LGW_SPI_ERROR)
*/

int lgw_spi_open(SPIClass **spi_target);

/**
@brief LoRa concentrator SPI close
@param spi_target generic pointer to SPI target (implementation dependant)
@return status of register operation (LGW_SPI_SUCCESS/LGW_SPI_ERROR)
*/

int lgw_spi_close(SPIClass *spi_target);

/**
@brief LoRa concentrator SPI single-byte write
@param spi_target generic pointer to SPI target (implementation dependant)
@param address 7-bit register address
@param data data byte to write
@return status of register operation (LGW_SPI_SUCCESS/LGW_SPI_ERROR)
*/
int lgw_spi_w(SPIClass *spi_target, uint8_t spi_mux_mode, uint8_t spi_mux_target, uint8_t address, uint8_t data);

/**
@brief LoRa concentrator SPI single-byte read
@param spi_target generic pointer to SPI target (implementation dependant)
@param address 7-bit register address
@param data data byte to write
@return status of register operation (LGW_SPI_SUCCESS/LGW_SPI_ERROR)
*/
int lgw_spi_r(SPIClass *spi_target, uint8_t spi_mux_mode, uint8_t spi_mux_target, uint8_t address, uint8_t *data);

/**
@brief LoRa concentrator SPI burst (multiple-byte) write
@param spi_target generic pointer to SPI target (implementation dependant)
@param address 7-bit register address
@param data pointer to byte array that will be sent to the LoRa concentrator
@param size size of the transfer, in byte(s)
@return status of register operation (LGW_SPI_SUCCESS/LGW_SPI_ERROR)
*/
int lgw_spi_wb(SPIClass *spi_target, uint8_t spi_mux_mode, uint8_t spi_mux_target, uint8_t address, uint8_t *data, uint16_t size);

/**
@brief LoRa concentrator SPI burst (multiple-byte) read
@param spi_target generic pointer to SPI target (implementation dependant)
@param address 7-bit register address
@param data pointer to byte array that will be written from the LoRa concentrator
@param size size of the transfer, in byte(s)
@return status of register operation (LGW_SPI_SUCCESS/LGW_SPI_ERROR)
*/
int lgw_spi_rb(SPIClass *spi_target, uint8_t spi_mux_mode, uint8_t spi_mux_target, uint8_t address, uint8_t *data, uint16_t size);

#endif

/* --- EOF ------------------------------------------------------------------ */
