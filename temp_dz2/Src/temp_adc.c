/**
    ******************************************************************************
    * @file    temp_adc.c
    * @author  Myaldzin Timur
    * @version V1.0.0
    * @date    24.11.2025
    * @brief   Функции для работы с АЦП и датчиком температуры
    ******************************************************************************
*/
#include "temp_adc.h"

/**
    *@brief Настройка датчика температуры
    *@param None
    *@retval None
**/
void TempSenseInit(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    RCC->CFGR &= ~RCC_CFGR_ADCPRE;
    RCC->CFGR |=  RCC_CFGR_ADCPRE_DIV6;

    /* Включаем ADC + датчик температуры одновременно */
    ADC1->CR2 |= (ADC_CR2_ADON | ADC_CR2_TSVREFE);

    /* Короткая задержка ~2 µs */
    for (volatile int i = 0; i < 200; i++);

    /* Калибровка */
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while (ADC1->CR2 & ADC_CR2_RSTCAL);

    ADC1->CR2 |= ADC_CR2_CAL;
    while (ADC1->CR2 & ADC_CR2_CAL);

    /* Sample time = 239.5 cycles (требуется RM) */
    ADC1->SMPR1 |= ADC_SMPR1_SMP16;

    /* Канал 16 */
    ADC1->SQR3 = 16;
}

uint32_t Read_Temperature_Sensor(void)
{
    /* Запуск одиночного преобразования */
    ADC1->CR2 |= ADC_CR2_ADON;

    /* Ждём EOC */
    while (!(ADC1->SR & ADC_SR_EOC));

    return ADC1->DR;
}

/**
    *@brief Перевод кода АЦП в градусы Цельсия
    *@param adc_value - значение АЦП
    *@retval Значение температуры в миллиградусах Цельсия
**/
uint32_t MilliCelsius_From_ADC(uint32_t adc_value)
{
    int64_t V_sense = ((int64_t)adc_value * (int64_t)VREF_u) / 4095;
    int64_t diff_uV = (int64_t)V25_u - V_sense;
    int64_t temp_mC = (diff_uV * 1000) / (int64_t)Avg_Slope_u;
    temp_mC += 25000;

    return (int32_t)temp_mC;
}
