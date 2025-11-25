/**
  ******************************************************************************
  * @file    main.c
  * @author  Myaldzin Timur
  * @version V1.0.0
  * @date    24.11.2025
  * @brief   Задача №2 - "Датчик температуры"
  *			 Отладочная плата: STM32F103 Nucleo-64
  ******************************************************************************
  */
#include <stdbool.h>
#include <sys/_intsup.h>
#include "main.h"
#include "temp_adc.h"
#include "uart.h"

/**
  * @brief  Настройка тактирования на 64МГц (HSI или HSE)
  * @param  None
  * @retval None
  */
void initClk(void)
{
    /* --- ШАГ 1: Включаем выбранный источник тактирования --- */

#if defined(USE_HSE)

    /* Включаем HSE */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) {};

#elif defined(USE_HSI)

    /* Включаем HSI */
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY)) {};

#else
#error "Не выбран источник тактирования! Определите USE_HSE или USE_HSI."
#endif


    /* --- ШАГ 2: Настройка Flash --- */
    FLASH->ACR |= FLASH_ACR_PRFTBE;       // Prefetch buffer ON
    FLASH->ACR &= ~FLASH_ACR_LATENCY;     // Очистить
    FLASH->ACR |= FLASH_ACR_LATENCY_2;    // 2 wait states (до 72MHz)


    /* --- ШАГ 3: Настройка делителей шин --- */
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;      // AHB  = SYSCLK
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;     // APB2 = AHB
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;     // APB1 = AHB/2 (максимум 36 MHz)


    /* --- ШАГ 4: Настройка PLL --- */

#if defined(USE_HSE)

    /* PLLSRC = HSE, умножение на 8 → 8 МГц * 8 = 64 МГц */
    RCC->CFGR |= RCC_CFGR_PLLSRC;         // Источник PLL = HSE
    RCC->CFGR &= ~RCC_CFGR_PLLXTPRE;      // HSE не делить
    RCC->CFGR |= RCC_CFGR_PLLMULL8;       // Множитель ×8

#elif defined(USE_HSI)

    /* PLLSRC = HSI/2, умножение на 16 → 4 МГц * 16 = 64 МГц */
    RCC->CFGR &= ~RCC_CFGR_PLLSRC;        // Источник PLL = HSI/2
    RCC->CFGR |= RCC_CFGR_PLLMULL16;      // Множитель ×16

#endif


    /* --- ШАГ 5: Запуск PLL --- */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {};


    /* --- ШАГ 6: Переключение SYSCLK на PLL --- */
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {};
}

/**
  * @brief  Основная программа
  * @param  None
  * @retval None
  */
int main(void)
{
	initClk();
	TempSenseInit();
  initUSART2();
	/*Основной цикл*/
	while(true){
      if (COM_RECEIVED())			//Если получена команда по UART
      {
          ExecuteCommand();		//Выполнить команду
      }
	};
}
