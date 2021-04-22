/*
  Función para debuguear el código de los distintos archivos

*/
#include "loragw_debug.h"   


#define RESET_PIN     17
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */



void Debugeo(void){
    Serial.println("Se pausó aqui");
    Serial.println("");
    Serial.println("");
    delay(3000);
}

void Reseteo(void){
    Serial.println("Se procederá a realizar un reset");
    pinMode(RESET_PIN, OUTPUT);

    // perform reset
    digitalWrite(RESET_PIN, HIGH);
    delay(1000);
    digitalWrite(RESET_PIN, LOW);
    delay(500);
    
    ESP.restart();
}