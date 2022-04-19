#include "stm32f0xx_hal.h"

typedef struct
{
	float Temperature;
	float Humidity;
	uint8_t T_byte1;
	uint8_t T_byte2;
	char *Temp_str;
	char *Hum_str;
}DHT_DataTypedef;

typedef struct 
  {
		char id[4];
		float humidity, temperature;
		int time_index;
		char *humStr, *tempStr, *id_str;
  } DHT_DataStore, ptrDHT_DataStore;

extern TIM_HandleTypeDef htim3;

void delay (uint16_t us);
void Set_Pin_Output (GPIO_TypeDef *GPIOx, uint16_t GPIO_PIN);
void Set_Pin_Input (GPIO_TypeDef *GPIOx, uint16_t GPIO_PIN);
void DHT22_Start (void);
void DHT22_Get_Data (DHT_DataTypedef *DHT_data);
uint8_t DHT22_Check_Response (void);
uint8_t DHT22_Read_Byte (void);
void DHT22_GetData(DHT_DataTypedef *DHT_Data);
void ftoa(float n, char* res, int afterpoint);
int intToStr(int x, char str[], int d);
void reverse(char* str, int len);


