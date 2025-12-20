#ifndef CORTO_PLAZO_H
#define CORTO_PLAZO_H

/**
 *@brief planificador corto plazo (5 estados) 
 *@param void* _
 *@return void* función para thread
*/
void* planificador_corto_plazo(void* _);

/// @brief Espera el tiempo del quantum en un hilo para no bloquear al hilo principal del kernel
void* timer_quantum(void* arg);

#endif /*CORTO_PLAZO_H*/