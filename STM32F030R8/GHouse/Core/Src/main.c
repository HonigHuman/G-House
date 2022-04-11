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
#include "UartRingbuffer_multi.h"
#include "ESP8266_HAL.h"
#include <stdio.h>
#include <string.h>


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
#define pc_uart &huart2
#define wifi_uart &huart1
	

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
float T_meas, Rh_meas;
DHT_DataTypedef DHT22_Data;
DHT_DataStore *new_d_ptr, new_data;


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

void pollData_UART(DHT_DataStore *data);// uint8_t buffer[]);
void saveData(char message[], DHT_DataStore *data);//char message[], DHT_DataStore *data);
void display_DHTData_LCD(DHT_DataStore *data);

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
  /* USER CODE BEGIN 2 */
	
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
	LCD_set_cursor_to_line(2); //set pointer to first line
  LCD_send_string ("Temp.:             C");
	LCD_set_cursor_to_line(4); //set pointer to third line
  LCD_send_string ("Rel.Hum.:    ");
	LCD_send_cmd(0x80|0x52);
	LCD_send_data(0xdf);
	LCD_send_cmd(0x80|0x67);
	LCD_send_data(0x25);
	//delay(100);
	//LCD_send_cmd(0x80|0x00);
	
	//char str[] = "AT+RST\r\n";	

	
	//HAL_UART_Transmit(&huart2, (uint8_t *) str, strlen(str), 0xFFFF);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		
		pollData_UART(&new_data);
		HAL_UART_Init(pc_uart);		
		
		display_DHTData_LCD(&new_data);
		
		HAL_Delay(2000);
		//DHT22_GetData(&DHT22_Data);
		//T_meas = DHT22_Data.Temperature/10;
		//Rh_meas = DHT22_Data.Humidity/10;

		/*printf("DHT: T = %.1f \n", T_meas);
		printf("DHT: R_h = %.1f \n", Rh_meas);
		char t_str[20];
		sprintf(t_str, "%.1f",T_meas);
		LCD_send_cmd(0x80|0x4e);
		LCD_send_string(t_str);

		char hum_str[20];
		sprintf(hum_str, "%.1f",Rh_meas);
		LCD_send_cmd(0x80|0x62);
		LCD_send_string(hum_str);
				
		HAL_Delay(2000);
		*/
		
		
		/*
		if (IsDataAvailable(pc_uart))
	  {
		  int data = Uart_read(pc_uart);
		  Uart_write(data, wifi_uart);
	  }

	  if (IsDataAvailable(wifi_uart))
	  {
		  if (Get_after("AT version:", 8, buffer, wifi_uart))
		  {
			  Uart_sendstring("AT VERSION=", pc_uart);
			  Uart_sendstring(buffer, pc_uart);
		  }
	  }
		*/
		
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
void pollData_UART(DHT_DataStore *data) {
	char buffer[41];
	HAL_UART_Init(wifi_uart);
	HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_SET);
	HAL_UART_Receive(wifi_uart, (uint8_t *) buffer, 41, 1000);

	//HAL_UART_Transmit(pc_uart, (uint8_t *) "Received message: \n", sizeof("Received message: \n"), 1000);
	//HAL_UART_Transmit(pc_uart, (uint8_t *) buffer, 50, 1000);
	
	saveData(buffer, data);
	HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_RESET);
	
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
			printf(attr);

        char *val;
        char nl[] = "\n";
        val = strtok(NULL, ":");

        if (strcmp("Device",attr)==0){
          data->device = val;
					printf(data->device);
        }
        else if (strcmp("Temperature",attr)==0){
          data->temperature = val;
					printf(data->temperature);
        }
        else if (strcmp("Humidity",attr)==0){
          data->humidity = val;
					printf(data->humidity);
					
        }      
    } 
	//printf(data->humidity);
}

void display_DHTData_LCD(DHT_DataStore *data){
		
	
		//char t_str = data->temperature;
		//sprintf(t_str, "%.1f",T_meas);
		LCD_send_cmd(0x80|0x4e);
		LCD_send_string(data->temperature);

		//char hum_str[20];
		//sprintf(hum_str, "%.1f",Rh_meas);
		LCD_send_cmd(0x80|0x62);
		LCD_send_string(data->humidity);
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
