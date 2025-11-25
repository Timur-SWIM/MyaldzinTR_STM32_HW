#ifndef TEMP_ADC_H_
#define TEMP_ADC_H_ 

#endif

#include "stm32f103xb.h"
#include <stdint.h>
#include "global.h"

void TempSenseInit(void);
uint32_t Read_Temperature_Sensor(void);
uint32_t MilliCelsius_From_ADC(uint32_t adc_value);


