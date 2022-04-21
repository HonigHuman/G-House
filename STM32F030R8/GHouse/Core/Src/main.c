/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
//#include "dht22.h"
#include "i2c-lcd.h"
#include "dht22.h"
//#include "UartRingbuffer_multi.h"
//#include "ESP8266_HAL.h"
#include <stdio.h>
#include <string.h>
#include <cstdlib>


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

#ifdef __GNUC__
  /* With GCC, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define pc_uart 								&huart2
#define wifi_uart 							&huart1
#define DATA_MESSAGE_BUFF_SIZE 	42
	

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
float T_meas, Rh_meas;
DHT_DataTypedef DHT22_Data;
DHT_DataStore init_data = {"-I-", 0.0, -99.9, 5}; 
DHT_DataStore new_data;
 
char wifi_rx_buffer[DATA_MESSAGE_BUFF_SIZE] = {0};
//char buffer[DATA_MESSAGE_BUFF_SIZE]={0};

int DATA_RXD_FLAG = 0;
int POLL_DATA_TRIGGER_FLAG = 0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void delay (uint16_t us);
void Set_Pin_Output (GPIO_TypeDef *GPIOx, uint16_t GPIO_PIN);
void Set_Pin_Input (GPIO_TypeDef *GPIOx, uint16_t GPIO_PIN);
void DHT22_Start (void);
uint8_t DHT22_Check_Response (void);
uint8_t DHT22_Read_Byte (void);
void DHT22_GetData(DHT_DataTypedef *DHT_Data);

int pollData_UART(DHT_DataStore *data);// uint8_t buffer[]);
int pollData_UART_IT(DHT_DataStore *data);
int pollData_UART_Serial(DHT_DataStore *data);
void saveData(char message[], DHT_DataStore *data);
void display_DHTData_LCD(DHT_DataStore *data);
void setup_DataDisplay_LCD();
int is_empty(char *buf, size_t size);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  MX_I2C2_Init();
  MX_USART1_UART_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
	
	HAL_TIM_Base_Start_IT(&htim6);
	
	LCD_init ();
	LCD_set_cursor_to_line(1); //set pointer to first line
  LCD_send_string (" Welcome ");
	LCD_set_cursor_to_line(2); //set pointer to third line
  LCD_send_string ("    at   ");
	LCD_set_cursor_to_line(3); //set pointer to third line
	LCD_send_string("  G-House ");
	
	HAL_Delay(2000);
	LCD_clear();	
	LCD_init ();
	setup_DataDisplay_LCD();
	display_DHTData_LCD(&init_data);
	HAL_Delay(2000);
	//HAL_UART_Receive_IT(wifi_uart, (uint8_t *) wifi_rx_buffer, sizeof(wifi_rx_buffer));
	HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_RESET);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		/*if (!HAL_GPIO_ReadPin(GPIOA, UART_TRIGGER_Pin))
		{
			HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_SET);
		}*/
		if (POLL_DATA_TRIGGER_FLAG)
		{
			if (pollData_UART(&new_data) == 1)
			{
				//saveData(wifi_rx_buffer, &new_data);
				display_DHTData_LCD(&new_data);
				//HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_RESET);
				DATA_RXD_FLAG = 0;
			}
			POLL_DATA_TRIGGER_FLAG = 0;
		}
		//HAL_Delay(2000);
		
		
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim == &htim6)
	{
		POLL_DATA_TRIGGER_FLAG = 1;
		HAL_TIM_Base_Start_IT(&htim6);
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    HAL_UART_Transmit(&huart2, (uint8_t *) wifi_rx_buffer, sizeof(wifi_rx_buffer), 1000);
    HAL_UART_Receive_IT(&huart1, (uint8_t *) wifi_rx_buffer, sizeof(wifi_rx_buffer));
		DATA_RXD_FLAG = 1;
}

int pollData_UART(DHT_DataStore *data) {
	char buffer[DATA_MESSAGE_BUFF_SIZE]={0};
	HAL_UART_Init(wifi_uart);

	HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_SET);
	
	HAL_UART_Receive(wifi_uart, (uint8_t *) buffer, DATA_MESSAGE_BUFF_SIZE, 3000);
	HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_RESET);


	HAL_UART_Transmit(pc_uart, (uint8_t *) "Received message: \n", sizeof("Received message: \n"), 1000);
	HAL_UART_Transmit(pc_uart, (uint8_t *) buffer, DATA_MESSAGE_BUFF_SIZE, 1000);
	
	HAL_UART_DeInit(wifi_uart);
	
	if (is_empty(buffer, sizeof(buffer)) == 0){	//check if received message is 0 buffer
		saveData(buffer, data);
	}
	else {
		return 0;
	}


	//HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_RESET);
	return 1;
	
}

int pollData_UART_Serial(DHT_DataStore *data) {
	char buffer[DATA_MESSAGE_BUFF_SIZE]={0};
	HAL_UART_Init(wifi_uart);
	//HAL_Delay(100);

	HAL_UART_Transmit(wifi_uart, (uint8_t *) "GETDATA\n", sizeof("GETDATA\n"), 1000);

	//HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_SET);
	
	HAL_UART_Receive(wifi_uart, (uint8_t *) buffer, DATA_MESSAGE_BUFF_SIZE, 3000);
	//HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_RESET);


	HAL_UART_Transmit(pc_uart, (uint8_t *) "Received message: \n", sizeof("Received message: \n"), 1000);
	HAL_UART_Transmit(pc_uart, (uint8_t *) buffer, DATA_MESSAGE_BUFF_SIZE, 1000);
	
	HAL_UART_DeInit(wifi_uart);
	
	if (is_empty(buffer, sizeof(buffer)) == 0){	//check if received message is 0 buffer
		saveData(buffer, data);
	}
	else {
		return 0;
	}


	//HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_RESET);
	return 1;
	
}

int pollData_UART_IT(DHT_DataStore *data) {
	//char buffer[DATA_MESSAGE_BUFF_SIZE]={0};
	//HAL_UART_Init(wifi_uart);

	HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_SET);
	//HAL_UART_Receive(wifi_uart, (uint8_t *) buffer, 42, 1000);

	//HAL_UART_Transmit(pc_uart, (uint8_t *) "Received message: \n", sizeof("Received message: \n"), 1000);
	//HAL_UART_Transmit(pc_uart, (uint8_t *) buffer, sizeof(buffer), 1000);
	/*if (buffer[0] == 0)
	{
		return 0;
	} */
	//saveData(buffer, data);
	
	//HAL_UART_DeInit(wifi_uart);

	//HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_RESET);
	return 1;
	
}


void saveData(char message[], DHT_DataStore *data){//(char message[], DHT_DataStore *data){
	char *token;
  char *token_list[5];
  char *cstr = &message[0];

   /* get the first token */
  token = strtok(cstr, "+");
   
   /* walk through other tokens */
	int i = 0;
	while( token != NULL ) {
		token_list[i] = token;
    i++;
    token = strtok(NULL, "+");
  }
  int n_tokens = i;

  for(int x = 0; x < n_tokens; x++){
		char *attr;
    attr = strtok(token_list[x], ":");

    char *val;
		char nl[] = "\n";
		val = strtok(NULL, ":");

		if (strcmp("Device",attr)==0){
			strcpy(data->id, val);
			data->id_str = val;
		}
    else if (strcmp("Temperature",attr)==0){
      data->temperature = atof(val);
			data->tempStr = val;
					//printf(data->temperature);
    }
    else if (strcmp("Humidity",attr)==0){
			data->humidity = atof(val);
			data->humStr = val;
					//printf(data->humidity);
    }      
  } 
}

void display_DHTData_LCD(DHT_DataStore *data){
		int slot_offset;
		if ((strcmp(data->id,"GWH")==0)||(strcmp(data->id,"SGS")==0)){
			slot_offset = 6;
		}
		else if ((strcmp(data->id,"OUT")==0)||(strcmp(data->id,"SOS")==0)){
			slot_offset = 0;
		}
		else return;
		
		LCD_send_cmd(0x80|(0x07+slot_offset));		//clear Devicename
		LCD_send_string("   ");
		LCD_send_cmd(0x80|(0x1a+slot_offset));	//clear Temperature
		LCD_send_string("     ");
		LCD_send_cmd(0x80|(0x5b+slot_offset));	//clear Humidity
		LCD_send_string("     ");
		
		int offset = 0;
		float t = data->temperature;
		float h = data->humidity;
		
		LCD_send_cmd(0x80|(0x07+slot_offset));	//display Devicename
		LCD_send_string(data->id);
	
		if ((t < 0.0 && t > -10.0)||(t >= 10.0)){
			offset = 1;
		}
		else if (t >= 0.0 && t < 10.0){
			offset = 2;
		}
		char t_buffer[6];
		sprintf(t_buffer, "%.1f", data->temperature);
		LCD_send_cmd(0x80|(0x1a+offset+slot_offset));	//display Temperature
		LCD_send_string(t_buffer);
		
		if (h < 10.0){
			offset = 1;
		}
		else{
			offset = 0;
		}
		char h_buffer[6];
		sprintf(h_buffer, "%.1f", data->humidity);
		LCD_send_cmd(0x80|(0x5b+offset+slot_offset));	//doisplay Humidity
		LCD_send_string(h_buffer);
}

void setup_DataDisplay_LCD(){
	LCD_set_cursor_to_line(1); //set pointer to first line
  LCD_send_string ("ID   :");
	LCD_set_cursor_to_line(3); //set pointer to first line
  LCD_send_string ("Temp :");
	LCD_set_cursor_to_line(4); //set pointer to third line
  LCD_send_string ("r.Hum:");
	LCD_send_cmd(0x80|0x26);	
	LCD_send_data(0xdf);	//degree sign
	LCD_send_cmd(0x80|0x27);	
	LCD_send_string("C");	
	LCD_send_cmd(0x80|0x67);
	LCD_send_data(0x25);	// percent sign
}

int is_empty(char *buf, size_t size)
{
    return buf[0] == 0 && !memcmp(buf, buf + 1, size - 1);
}

PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the EVAL_COM1 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}



/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
