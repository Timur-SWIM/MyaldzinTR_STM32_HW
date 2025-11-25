/**
  ******************************************************************************
  * @file    Src/uart.c
  * @author  Myaldzin T. R.
  * @version V1.0
  * @date    24.11.2025
  * @brief   Файл содержит функции для работы с USART2
  ******************************************************************************
  */
#include "uart.h"
#include <stdbool.h>

char RxBuffer[RX_BUFF_SIZE];			//Буфер приёма USART
char TxBuffer[TX_BUFF_SIZE];			//Буфер передачи USART
volatile bool ComReceived;				//Флаг приёма строки данных (volatile - считывание из памяти, а не кэша, т.к. меняется в прерывании)
volatile uint8_t RxIndex = 0; 			//Индекс для приёма данных

/**
  * @brief  Обработчик прерывания по USART2
  * @param  None
  * @retval None
  */
void USART2_IRQHandler(void)
{
	if ((USART2->SR & USART_SR_RXNE)!=0)								//Status register: read data register not empty
	{
		uint8_t pos = strlen(RxBuffer);									//Количество байт до /0 (конца строки), куда будем писать следующий байт

		RxBuffer[pos] = USART2->DR;										//Считываем содержимое регистра данных 1 байт (при чтении SR автоматически очищается бит RXNE)

		if ((RxBuffer[pos]== 0x0A) && (RxBuffer[pos-1]== 0x0D))			//Если это символ конца строки
		{
			ComReceived = true;											//- выставляем флаг приёма строки
			return;														//- и выходим
		}
	}
}
/**
  * @brief  Инициализация USART2
  * @param  None
  * @retval None
  */
void initUSART2(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;						//включить тактирование порта A
	RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;						//включить тактирование альтернативных ф-ций портов
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;					//включить тактирование UART2

	//PA2 на выход
	GPIOA->CRL &= ~(GPIO_CRL_MODE2 | GPIO_CRL_CNF2);		
	GPIOA->CRL |= (GPIO_CRL_MODE2_1 | GPIO_CRL_CNF2_1);		//MODE=10, Output 2 MHZ; CNF=10 Alternate function push-pull

	//PA3 на вход
	GPIOA->CRL &= ~(GPIO_CRL_MODE3 | GPIO_CRL_CNF3);		
	GPIOA->CRL |= GPIO_CRL_CNF3_0;							//MODE=00, Input mode; CNF=01 Floating input

	/*****************************************
	Скорость передачи данных - 57600 бод
	Частота шины APB1 - 32МГц

	1. USARTDIV = 32'000'000/(16*57600) = 34,7222
	2. 34 = 0x22
	3. 16*0.7 = 11,2 ~ 11 = 0xB
	4. Итого 0x22B
	*****************************************/
	USART2->BRR = 0x22B;		//baud rate register в 34.7222

	USART2->CR1 |= USART_CR1_RE | USART_CR1_TE | USART_CR1_UE;	//разрешить приём, передачу и сам USART
	USART2->CR1 |= USART_CR1_RXNEIE;							//разрешить прерывание по приему байта данных (rx not empty interrupt enable)

	NVIC_EnableIRQ(USART2_IRQn);
}

/**
  * @brief  Передача строки по USART2 без DMA
  * @param  *str - указатель на строку
  * @param  crlf - если true, перед отправкой добавить строке символы конца строки
  * @retval None
  */
void txStr(char *str, bool crlf)
{
	uint16_t i;

	if (crlf)												//если просят,
		strcat(str,"\r\n");									//добавляем символ конца строки

	for (i = 0; i < strlen(str); i++)
	{
		USART2->DR = str[i];								//передаём байт данных
		while ((USART2->SR & USART_SR_TC)==0) {};			//ждём окончания передачи (transmission complete)
	}
}
/**
  * @brief  Обработчик команд
  * @param  None
  * @retval None
  */
void ExecuteCommand(void)
{
	memset(TxBuffer,0,sizeof(TxBuffer));					//Очистка буфера передачи (заполнение нулями)

	/* Обработчик команд */
	if (strncmp(RxBuffer,"T?",2) == 0)					//Это команда "T?" размером 2 символов?
	{
		uint32_t adc_value = Read_Temperature_Sensor();	//Считываем значение АЦП
        uint32_t temperature = MilliCelsius_From_ADC(adc_value); //Преобразуем в градусы Цельсия
        int32_t t_C  = temperature / 1000;   // целая часть
        int32_t t_dC = temperature % 1000;   // дробная часть
        sniprintf(TxBuffer, sizeof(TxBuffer), "Temperature: %d,%03d C", t_C, t_dC); //формируем строку с ответом
	}
	else
		strcpy(TxBuffer,"Invalid Command");					//Если мы не знаем, чего от нас хотят, ругаемся в ответ

	txStr(TxBuffer, true);						//Передача строки ответа с добавлением символов конца строки	
	memset(RxBuffer,0,RX_BUFF_SIZE);						//Очистка буфера приёма (заполнение нулями)
	ComReceived = false;									//Сбрасываем флаг приёма строки
}

bool COM_RECEIVED(void)		
{						
    return ComReceived;	
}
