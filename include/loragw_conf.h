#include <arduino.h>   /* delay */

/* -------------------------------------------------------------------------- */
/* --- DECLARACIONES DE FUNCIONES PRIVADAS ---------------------------------- */

/**
@brief Configura los canales a usar y el BW y SF para el ccaso del canal 9
*/
int parse_SX1301_configuration(void);

/**
@brief Adquiere el ID del gateway
@return El valor del ID en entero
*/
unsigned long long parse_gateway_configuration(void);

/**
@brief Configura las ganancias disponibles para la transmisión de mensaje
*/
void configure_TxGainLUT(void);

/* -------------------------------------------------------------------------- */
/* --- DECLARACIONES DE FUNCIONES PRIVADAS ---------------------------------- */



