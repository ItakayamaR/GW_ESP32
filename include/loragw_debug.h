/*
    Activar aquí para definir si se quiere mostrar mensajes de debug de las distintas librerias
    Si se pone 1, se mostrará mensajes a traves de la comunicación serial de la computadora
*/

#include <arduino.h>   /* delay */

#define DEBUG_AUX 0
#define DEBUG_SPI 0
#define DEBUG_REG 1
#define DEBUG_HAL 1

/* -------------------------------------------------------------------------- */
/* --- VARIABLES COMPARTIDAS ------------------------------------------------ */
static char debug_msg[100];


/* -------------------------------------------------------------------------- */
/* --- DECLARACIÓN DE FUNCIONES PUBLICAS ------------------------------------ */

/* Poner esta función en los puntos donde quieras pausar el código */
void Debugeo(void);

/* Realiza un reseteo al gateway y reinicia el ESP */
void Reseteo(void);


