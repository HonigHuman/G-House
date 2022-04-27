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
#include "i2c-lcd.h"
//#include "dht22.h"
#include <stdio.h>
#include <string.h>
#include <cstdlib>


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define gsmUART 								huart2
#define wifi_uart 							huart1
#define DATA_MESSAGE_BUFF_SIZE 	42
#define OUT_DATA_IDX						0
#define GWH_DATA_IDX						1
	
typedef struct 
  {
		char id[4];
		float humidity, temperature;
		int time_index;
		//char *humStr, *tempStr, *id_str;
  } DHT_DataStore, ptrDHT_DataStore;
	

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
char API_KEY[] = "E7BV47IONFYVV2LA";// G-House : "NEX4VYBFJEKPI156";

DHT_DataStore init_data = {"-I-", 3.3, -99.9, 5}; 
DHT_DataStore new_data = {"-N-", 3.3, -99.9, 5};
DHT_DataStore last_OUT_data, last_GWH_data;
DHT_DataStore *last_data_sets[2] = {&init_data, &init_data};
 
char wifi_rx_buffer[DATA_MESSAGE_BUFF_SIZE] = {0};
uint8_t buf[100] = {0};
char msg[50];
char cmd[50];
int flag = 1;
int tim15_cnt = 0;

// FLAGS -------------------------------------------------------------
int DATA_RXD_FLAG = 0;
int GSM_TEST_FLAG = 1;
int POLL_DATA_TRIGGER_FLAG = 0;
int PUSH_DATA_GSM_FLAG = 0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/*void delay (uint16_t us);
void Set_Pin_Output (GPIO_TypeDef *GPIOx, uint16_t GPIO_PIN);
void Set_Pin_Input (GPIO_TypeDef *GPIOx, uint16_t GPIO_PIN);
void DHT22_Start (void);
uint8_t DHT22_Check_Response (void);
uint8_t DHT22_Read_Byte (void);
void DHT22_GetData(DHT_DataTypedef *DHT_Data);*/

int pollData_UART(DHT_DataStore *data);// uint8_t buffer[]);
int pollData_UART_IT(DHT_DataStore *data);
int pollData_UART_Serial(DHT_DataStore *data);
void saveData(char message[], DHT_DataStore *data);
void display_DHTData_LCD(DHT_DataStore *data);
void setup_DataDisplay_LCD();
int is_empty(char *buf, size_t size);

void send_GSM_Data();
int check_A6(void);
void send_String_A6(const char* cmd, uint16_t timeout);

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
  MX_USART1_UART_Init();
  MX_TIM6_Init();
  MX_TIM15_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
	
	HAL_TIM_Base_Start_IT(&htim6);
	
	LCD_init ();
	LCD_set_cursor_to_line(1); //set pointer to first line
  LCD_send_string (" Welcome ");
	LCD_set_cursor_to_line(2); //set pointer to third line
  LCD_send_string ("    at   ");
	LCD_set_cursor_to_line(3); //set pointer to third line
	LCD_send_string("  G-House ");
	
	HAL_TIM_Base_Start_IT(&htim15);
	
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
	POLL_DATA_TRIGGER_FLAG = 0;
	PUSH_DATA_GSM_FLAG = 0;
	tim15_cnt = 0;
	int cnt = 0;
	HAL_I2C_DeInit(&hi2c1);
	
  while (1)
  {
		
		/*if (cnt == 6){
			PUSH_DATA_GSM_FLAG = 1;
		}*/
		/*if (!HAL_GPIO_ReadPin(GPIOA, UART_TRIGGER_Pin))
		{
			HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_SET);
		}*/
		if (POLL_DATA_TRIGGER_FLAG)
		{
			cnt++;
			if (pollData_UART(&new_data) == 1)
			{
					//saveData(wifi_rx_buffer, &new_data);
				//display_DHTData_LCD(&new_data);//&new_data);
					//HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_RESET);
				DATA_RXD_FLAG++;
			}
			POLL_DATA_TRIGGER_FLAG = 0;
		}
	if (PUSH_DATA_GSM_FLAG)
		{
			HAL_TIM_Base_Stop_IT(&htim6);
			HAL_TIM_Base_DeInit(&htim6);
			send_GSM_Data();
			HAL_TIM_Base_Init(&htim6);
			HAL_TIM_Base_Start_IT(&htim6);
			PUSH_DATA_GSM_FLAG = 0;
		}
		
		
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
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
	if (htim == &htim15)
	{
		tim15_cnt++;
		if (tim15_cnt==2){
			tim15_cnt = 0;
			PUSH_DATA_GSM_FLAG = 1;
			HAL_TIM_Base_Start_IT(&htim15);
		}
	}
}

/*void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    HAL_UART_Transmit(&huart2, (uint8_t *) wifi_rx_buffer, sizeof(wifi_rx_buffer), 1000);
    HAL_UART_Receive_IT(&huart1, (uint8_t *) wifi_rx_buffer, sizeof(wifi_rx_buffer));
		DATA_RXD_FLAG = 1;
}*/

int pollData_UART(DHT_DataStore *data) {
	char buffer[DATA_MESSAGE_BUFF_SIZE]={0};
	HAL_UART_Init(&wifi_uart);

	HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_SET);
	
	HAL_UART_Receive(&wifi_uart, (uint8_t *) buffer, DATA_MESSAGE_BUFF_SIZE, 3000);
	HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_RESET);


	//HAL_UART_Transmit(pc_uart, (uint8_t *) "Received message: \n", sizeof("Received message: \n"), 1000);
	//HAL_UART_Transmit(pc_uart, (uint8_t *) buffer, DATA_MESSAGE_BUFF_SIZE, 1000);
	
	HAL_UART_DeInit(&wifi_uart);
	
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
	HAL_UART_Init(&wifi_uart);
	//HAL_Delay(100);

	HAL_UART_Transmit(&wifi_uart, (uint8_t *) "GETDATA\n", sizeof("GETDATA\n"), 1000);

	//HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_SET);
	
	HAL_UART_Receive(&wifi_uart, (uint8_t *) buffer, DATA_MESSAGE_BUFF_SIZE, 3000);
	//HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_RESET);


	//HAL_UART_Transmit(pc_uart, (uint8_t *) "Received message: \n", sizeof("Received message: \n"), 1000);
	//HAL_UART_Transmit(pc_uart, (uint8_t *) buffer, DATA_MESSAGE_BUFF_SIZE, 1000);
	
	HAL_UART_DeInit(&wifi_uart);
	
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
	char buffer[DATA_MESSAGE_BUFF_SIZE]={0};
	HAL_UART_Init(&wifi_uart);

	HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_SET);
	
	
	HAL_UART_Receive_IT(&wifi_uart, (uint8_t *) buffer, DATA_MESSAGE_BUFF_SIZE);
	//HAL_UART_Receive(wifi_uart, (uint8_t *) buffer, DATA_MESSAGE_BUFF_SIZE, 3000);
	HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_RESET);


	//HAL_UART_Transmit(pc_uart, (uint8_t *) "Received message: \n", sizeof("Received message: \n"), 1000);
	//HAL_UART_Transmit(pc_uart, (uint8_t *) buffer, DATA_MESSAGE_BUFF_SIZE, 1000);
	
	HAL_UART_DeInit(&wifi_uart);
	
	if (is_empty(buffer, sizeof(buffer)) == 0){	//check if received message is 0 buffer
		saveData(buffer, data);
	}
	else {
		return 0;
	}


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
			//data->id_str = val;
		}
    else if (strcmp("Temperature",attr)==0){
      data->temperature = atof(val);
			//data->tempStr = val;
					//printf(data->temperature);
    }
    else if (strcmp("Humidity",attr)==0){
			data->humidity = atof(val);
			//data->humStr = val;
					//printf(data->humidity);
    }      
  } 
	
	
	if (strcmp(data->id, "OUT")==0){
		memcpy(&last_OUT_data,data, sizeof(DHT_DataStore));
		last_data_sets[OUT_DATA_IDX] = &last_OUT_data;
	}
	else if (strcmp(data->id, "GWH")==0){
		memcpy(&last_GWH_data, data, sizeof(DHT_DataStore));
		last_data_sets[GWH_DATA_IDX] = &last_GWH_data; 
	}
}

void display_DHTData_LCD(DHT_DataStore *data){
		HAL_I2C_Init(&hi2c1);
	
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
		sprintf(t_buffer, "%.1f", t);
		LCD_send_cmd(0x80|(0x1a+offset+slot_offset));	//display Temperature
		LCD_send_string(t_buffer);
		
		if (h < 10.0){
			offset = 1;
		}
		else{
			offset = 0;
		}
		char h_buffer[6];
		sprintf(h_buffer, "%.1f", h);
		LCD_send_cmd(0x80|(0x5b+offset+slot_offset));	//doisplay Humidity
		LCD_send_string(h_buffer);
		HAL_I2C_DeInit(&hi2c1);
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

void send_GSM_Data(){
	
	float temp_OUT = last_data_sets[OUT_DATA_IDX]->temperature;
	float hum_OUT = last_data_sets[OUT_DATA_IDX]->humidity;
	float temp_GWH = last_data_sets[GWH_DATA_IDX]->temperature;
	float hum_GWH = last_data_sets[GWH_DATA_IDX]->humidity;
	HAL_UART_Init(&gsmUART);
	
	//synchronize gsm module
	if(check_A6()==0){
		sprintf(msg,"Couldn't synchronize A6.\r\n");
		//HAL_UART_Transmit(&huart2,(uint8_t *)msg,strlen(msg),1000);
	}
		
	send_String_A6("AT+CSQ\r\n", 1000);
	send_String_A6("AT+CPIN?\r\n", 1000);
	send_String_A6("AT+CREG?\r\n", 1000);
	send_String_A6("AT+CGATT=1\r\n", 1000);
	send_String_A6("AT+CIPMUX=0\r\n", 1000);
	send_String_A6("AT+CSTT=\"internet\",\"\",\"\"\r\n", 2000);
	send_String_A6("AT+CSTT?\r\n", 1000);
	send_String_A6("AT+CIICR\r\n", 1000);
	send_String_A6("AT+CIFSR\r\n", 1000);
	send_String_A6("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80\r\n", 4000);
	send_String_A6("AT+CIPSEND\r\n", 2000);
	
	sprintf(msg, "GET https://api.thingspeak.com/update?api_key=%s&field1=%.1f&field2=%.1f&field3=%.1f&field4=%.1f\r\n",API_KEY, temp_OUT, hum_OUT, temp_GWH, hum_GWH);
	send_String_A6(msg, 4000);
	
	char ctrlZ[2] = {(char) 0x1A, (char) 0x0D};
	send_String_A6((uint8_t *)ctrlZ, 2000);
	send_String_A6("AT+CIPCLOSE\r\n", 1000);
	HAL_UART_DeInit(&gsmUART);
}

int check_A6(void){
	int flag = 1;
	
	while(flag){
		sprintf(cmd,"AT\r\n");
		//HAL_UART_Transmit(&pcUART,(uint8_t *)cmd,strlen(cmd),1000);
		HAL_UART_Transmit(&gsmUART,(uint8_t *)cmd,strlen(cmd),1000);
		HAL_UART_Receive_IT (&gsmUART, buf, sizeof(buf));
		HAL_Delay(1000);
		
		if(strstr((char *)buf,"OK")){
			sprintf(msg,"Module Connected\r\n");
			//HAL_UART_Transmit(&pcUART,(uint8_t *)msg,strlen(msg),1000);
			flag=0;
		}
		HAL_Delay(250);
		//HAL_UART_Transmit(&pcUART,(uint8_t *)buf,sizeof(buf),1000);
		memset(buf,0,sizeof(buf));
	}
	return 1;
}

void send_String_A6(const char* cmd_str, uint16_t delay){
	
	//memset(buf2,0,sizeof(buf2));
	sprintf(msg, cmd_str);
	//HAL_UART_Transmit(&huart2,(uint8_t *)msg,strlen(msg),1000);
	//HAL_UART_Transmit(&huart2,(uint8_t *)"\r\n",sizeof("\r\n"),1000);
	HAL_UART_Transmit(&gsmUART,(uint8_t *)msg,strlen(msg),1000);
	HAL_UART_Receive_IT (&gsmUART, buf, sizeof(buf));
	
	/*HAL_TIM_Base_Start(&htim3);
	uint16_t time_start = __HAL_TIM_GET_COUNTER(&htim3);
	while (__HAL_TIM_GET_COUNTER(&htim3) - time_start < delay);
	HAL_TIM_Base_Stop(&htim3);*/
	HAL_Delay(delay);
	
	//HAL_UART_Transmit(&huart2,(uint8_t *)buf,sizeof(buf),1000);
	memset(buf,0,sizeof(buf));
}  

int is_empty(char *buf, size_t size)
{
    return buf[0] == 0 && !memcmp(buf, buf + 1, size - 1);
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
