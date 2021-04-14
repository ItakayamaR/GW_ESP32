/*
    Activar aquí para definir si se quiere mostrar mensajes de debug de las distintas librerias
    Si se pone 1, se mostrará mensajes a traves de la comunicación serial de la computadora
*/

#include <arduino.h>   /* delay */

#define DEBUG_AUX 1
#define DEBUG_SPI 1
#define DEBUG_REG 1
#define DEBUG_HAL 1

static char debug_msg[100];
