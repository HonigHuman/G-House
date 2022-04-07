#include <math.h>
#include <stdio.h>
#include "dht22.h"

extern TIM_HandleTypeDef htim3;


#define DHT22_PORT GPIOB
#define DHT22_PIN GPIO_PIN_8


/**
	* @brief Implements a microsecond delay with TIM3.
	* @param us: number of microseconds to delay
	* @retval None
	*/
void delay(uint16_t us) 
{
	__HAL_TIM_SET_COUNTER(&htim3, 0);
	while(__HAL_TIM_GET_COUNTER(&htim3) < us);
}

void Set_Pin_Output(GPIO_TypeDef *GPIOx, uint16_t GPIO_PIN)
{
	//GPIOA->MODER |= (1<<8); //Write a 1 into MODER Register 
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;//GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}
void Set_Pin_Input(GPIO_TypeDef *GPIOx, uint16_t GPIO_PIN)
{
//	GPIOA->MODER |= (0<<8); //Write a 0 into MODER Register 
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull =  GPIO_PULLUP;
	GPIO_InitStruct.Speed =  GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

void DHT22_Start (void)
{
	Set_Pin_Output (DHT22_PORT, DHT22_PIN);  // set the pin as output
	HAL_GPIO_WritePin (DHT22_PORT, DHT22_PIN, 0);   // pull the pin low
	delay (1200);   // wait for >1 ms
	HAL_GPIO_WritePin (DHT22_PORT, DHT22_PIN, 1);
	delay(20);
	Set_Pin_Input(DHT22_PORT, DHT22_PIN);    // set as input
}

uint8_t DHT22_Check_Response (void)
{
	uint8_t Response = 0;
	delay (40);
	if (!(HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN)))
	{
		delay (80);
		if ((HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN))) Response = 1;
		else Response = 2;
	}
	while ((HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN)));   // wait for the pin to go low

	return Response;
}

uint8_t DHT22_Read_Byte (void)
{
	uint8_t i,j;
	for (j=0;j<8;j++)
	{
		while (!(HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN)));   // wait for the pin to go high
		delay (40);   // wait for 40 us
		if (!(HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN)))   // if the pin is low
		{
			i&= ~(1<<(7-j));   // write 0
		}
		else i|= (1<<(7-j));  // if the pin is high, write 1
		while ((HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN)));  // wait for the pin to go low
	}
	return i;
}

void DHT22_GetData (DHT_DataTypedef *DHT_data)
{
	uint8_t present, h_byte1, h_byte2, t_byte1, t_byte2, sum_received, sum_calculated;

	TIM3->CR1 |= 1;

	DHT22_Start();
	present = DHT22_Check_Response();
	h_byte1 = DHT22_Read_Byte();
	h_byte2 = DHT22_Read_Byte();
	t_byte1 = DHT22_Read_Byte();
	t_byte2 = DHT22_Read_Byte();
	sum_received = DHT22_Read_Byte();
	
	sum_calculated = (h_byte1 + h_byte2 + t_byte1 + t_byte2);
	
	if (sum_calculated == sum_received)
	{
		DHT_data->Temperature = ((t_byte1<<8)|t_byte2);
		DHT_data->Humidity = ((h_byte1<<8)|h_byte2);
		DHT_data->T_byte1 = t_byte1;
		DHT_data->T_byte2 = t_byte2;
		char res[4];
		ftoa(DHT_data->Temperature, res, 1);
		DHT_data->Hum_str = res; 
	}
	
	TIM3->CR1 |= 0;
}

// Reverses a string 'str' of length 'len'
void reverse(char* str, int len)
{
    int i = 0, j = len - 1, temp;
    while (i < j) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++;
        j--;
    }
}
  
// Converts a given integer x to string str[]. 
// d is the number of digits required in the output. 
// If d is more than the number of digits in x, 
// then 0s are added at the beginning.
int intToStr(int x, char str[], int d)
{
    int i = 0;
    while (x) {
        str[i++] = (x % 10) + '0';
        x = x / 10;
    }
  
    // If number of digits required is more, then
    // add 0s at the beginning
    while (i < d)
        str[i++] = '0';
  
    reverse(str, i);
    str[i] = '\0';
		return i;
}
  
// Converts a floating-point/double number to a string.
void ftoa(float n, char* res, int afterpoint)
{
    // Extract integer part
    int ipart = (int)n;
  
    // Extract floating part
    float fpart = n - (float)ipart;
  
    // convert integer part to string
    int i = intToStr(ipart, res, 0);
  
    // check for display option after point
    if (afterpoint != 0) {
        res[i] = '.'; // add dot
  
        // Get the value of fraction part upto given no.
        // of points after dot. The third parameter 
        // is needed to handle cases like 233.007
        fpart = fpart * pow(10, afterpoint);
				

        intToStr((int)fpart, res + i + 1, afterpoint);
    }
}



