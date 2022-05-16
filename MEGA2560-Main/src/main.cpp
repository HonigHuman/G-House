#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

#define BAUD_RATE               9600
#define pcUART                  Serial
#define wifiUART                Serial1       
#define gsmUART                 Serial2

#define SLAVE_ADDRESS_LCD       0x27
#define LCD_COLS				20
#define LCD_ROWS				4
#define LCD_DATA_0_COL			6

#define POLL_TRIGGER_PIN        31
#define GSM_RESET_PIN			33

#define GSM_RESET_TIME_MS 		300   //reset_A6 pin of gsm A6 pulled low for (ms)
#define GSM_MAX_SYNC_SENDS		10 		//maximum count of AT sends before resetting GSM module
#define GSM_INTERVAL_MIN        5 

#define TIM3_OVF_HZ             2
#define OUT_DATA_IDX			0
#define GWH_DATA_IDX			1
#define POL_DATA_IDX            2


typedef struct 
  {
		char id[4];
		double humidity, temperature;
  } DHT_DataStore, ptrDHT_DataStore;

//--------------- PFP --------------
void init_Timers();
void init_LCD();
void init_GPIOs();
void start_Timer_OVF_IT(int timer_index);

int pollData_UART(DHT_DataStore *data);// uint8_t buffer[]);
void saveData(char message[], DHT_DataStore *data);
void display_DHTData_LCD(DHT_DataStore *data);
void setup_DataDisplay_LCD();
int is_empty(char *buf, size_t size);

void send_GSM_Data();
int check_A6(void);
void send_String_A6(const char* cmd, uint16_t timeout);
void reset_A6();

//--------------------------------------
uint16_t tim3_ovf_cnt = 0;
uint32_t n_rxd_data = 0;
int PUSH_DATA_GSM_FLAG = 0;
int POLL_DATA_TRIGGER_FLAG = 0;

String API_KEY = "NEX4VYBFJEKPI156";

DHT_DataStore test_data = {"GWH", 33.3, 59.9}; 
DHT_DataStore init_data = {"-I-", 3.3, -99.9}; 
DHT_DataStore new_data = {"-N-", 3.3, -99.9};
DHT_DataStore last_OUT_data, last_GWH_data;
DHT_DataStore *last_data_sets[2] = {&init_data, &init_data};

LiquidCrystal_PCF8574 lcd(0x27);


void setup(){
	
	pcUART.begin(BAUD_RATE);
	wifiUART.begin(BAUD_RATE);
	gsmUART.begin(BAUD_RATE);

	init_GPIOs();
	init_LCD();
	
	init_Timers();
	start_Timer_OVF_IT(3);
	start_Timer_OVF_IT(4);
}

void loop() {
  if (n_rxd_data == 4)
  	{
		PUSH_DATA_GSM_FLAG = 1;
	}

	if (POLL_DATA_TRIGGER_FLAG)
	{
		pcUART.println("Data Polling...");
		if (pollData_UART(&new_data) == 1)
		{
			// saveData(wifi_rx_buffer, &new_data);
			display_DHTData_LCD(&new_data); //&new_data);
			n_rxd_data++;
		}
		POLL_DATA_TRIGGER_FLAG = 0;
	}

	if (PUSH_DATA_GSM_FLAG)
	{
		send_GSM_Data();
		PUSH_DATA_GSM_FLAG = 0;
	}
}

void init_GPIOs(){
	pinMode(GSM_RESET_PIN, OUTPUT);
	pinMode(POLL_TRIGGER_PIN, OUTPUT);
}

void init_Timers(){
  // initialize timer1 
  noInterrupts();           // disable all interrupts
  TCCR3A = 0;
  TCCR3B = 0;
  TCNT3 = 34286;            // preload timer 65536-16MHz/1024/0.5Hz
  TCCR3B |= (1 << CS32)|(1 << CS30);    // 256 prescaler 
  //TIMSK3 |= (1 << TOIE3);   // enable timer overflow interrupt

  TCCR4A = 0;
  TCCR4B = 0;
  TCNT4 = 18661;            // preload timer 65536-16MHz/1024/0.333Hz
  TCCR4B |= (1 << CS42)|(1 << CS40);    // 1024 prescaler 


  TCCR5A = 0;
  TCCR5B = 0;
  TCNT5 = 34286;            // preload timer 65536-16MHz/256/2Hz
  TCCR5B |= (1 << CS52)|(1 << CS50);    // 1024 ?(256 prescaler )

  interrupts();             // enable all interrupts
}

void init_LCD(){
  lcd.begin(20,4);
  delay(1);
  lcd.leftToRight();
  lcd.setBacklight(HIGH);
  
  lcd.clear();
  
  lcd.setCursor(0,0);
  lcd.print("Welcome");
  lcd.setCursor(2,1);
  lcd.print("at");
  lcd.setCursor(0,2);
  lcd.print("G-House");
  delay(1000);
  lcd.clear();
  setup_DataDisplay_LCD();

}

void start_Timer_OVF_IT(int timer_index){
  switch (timer_index)
  {
  case 3:
    TIFR3 = 1<<TOV3;
    TIMSK3 |= (1 << TOIE3); // enable timer overflow interrupt
    break;
  case 4:
    TIFR4 = 1<<TOV4;
    TIMSK4 |= (1 << TOIE4);
    break;
  case 5:
    TIFR5 = 1<<TOV5;
    TIMSK5 |= (1 << TOIE5);
    break;
  
  default:
    break;
  }
}

int pollData_UART(DHT_DataStore *data) {
	
	digitalWrite(POLL_TRIGGER_PIN, HIGH);//HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_SET);
	
	String buffer = wifiUART.readStringUntil('\n');
	digitalWrite(POLL_TRIGGER_PIN, LOW);

	if (is_empty(&buffer[0], sizeof(buffer)) == 0){	//check if received message is 0 buffer
		saveData(&buffer[0], data);
	}
	else {
		return 0;
	}
	return 1;
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
		
		lcd.home();
		lcd.setCursor(LCD_DATA_0_COL+slot_offset+1,0);		//clear Devicename
		lcd.print("   ");
		lcd.setCursor(LCD_DATA_0_COL+slot_offset,2);	//clear Temperature
		lcd.print("     ");
		lcd.setCursor(LCD_DATA_0_COL+slot_offset,3);	//clear Humidity
		lcd.print("     ");
		
		int offset = 0;
		float t = data->temperature;
		float h = data->humidity;
		
		lcd.setCursor(LCD_DATA_0_COL+slot_offset+1,0);	//display Devicename
		lcd.print(data->id);

		char t_buffer[6];
		dtostrf(t,5,1,t_buffer);
		
		lcd.setCursor(LCD_DATA_0_COL+offset+slot_offset,2);	//display Temperature
		lcd.print(t_buffer);
		
		char h_buffer[6];
		dtostrf(h,5,1,h_buffer);
		lcd.setCursor(LCD_DATA_0_COL+offset+slot_offset,3);	//doisplay Humidity
		lcd.print(h_buffer);

}

void setup_DataDisplay_LCD(){
	lcd.setCursor(0,0); // set pointer to first line
	lcd.print("ID:");
	lcd.setCursor(0,2); // set pointer to first line
	lcd.print("Tp:");
	lcd.setCursor(0,3); // set pointer to third line
	lcd.print("rH:");
	lcd.setCursor(LCD_COLS-2, 2);
	lcd.write(0xdf); // degree sign
	lcd.print("C");
	lcd.setCursor(LCD_COLS-1,3);
	lcd.write(0x25); // percent sign */
}

void send_GSM_Data(){
	char *msg;
	reset_A6();
	float temp_OUT = last_data_sets[OUT_DATA_IDX]->temperature;
	float hum_OUT = last_data_sets[OUT_DATA_IDX]->humidity;
	float temp_GWH = last_data_sets[GWH_DATA_IDX]->temperature;
	float hum_GWH = last_data_sets[GWH_DATA_IDX]->humidity;

	char *temp_OUT_str, *hum_OUT_str, *temp_GWH_str, *hum_GWH_str; 
	dtostrf(temp_OUT,3,1,temp_OUT_str);
	dtostrf(hum_OUT,3,1,hum_OUT_str);
	dtostrf(temp_GWH,3,1,temp_GWH_str);
	dtostrf(hum_GWH,3,1,hum_GWH_str);

	//synchronize gsm module
	if(check_A6()==0){
		pcUART.print("Couldn't synchronize A6.\r\n");
	}		
	send_String_A6("AT+CSQ\r\n", 1000);
	send_String_A6("AT+CPIN?\r\n", 1000);
	send_String_A6("AT+CREG=0\r\n", 1000);
	send_String_A6("AT+CREG?\r\n", 1000);
	send_String_A6("AT+CGATT=1\r\n", 1000);
	send_String_A6("AT+CIPMUX=0\r\n", 1000);
	send_String_A6("AT+CSTT=\"internet\",\"\",\"\"\r\n", 2000);
	send_String_A6("AT+CSTT?\r\n", 1000);
	send_String_A6("AT+CIICR\r\n", 1000);
	send_String_A6("AT+CIFSR\r\n", 1000);
	send_String_A6("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80\r\n", 4000);
	send_String_A6("AT+CIPSEND\r\n", 2000);
	
	//sprintf(msg, "GET https://api.thingspeak.com/update?api_key=%s&field1=%.1f&field2=%.1f&field3=%.1f&field4=%.1f\r\n",API_KEY, temp_OUT, hum_OUT, temp_GWH, hum_GWH);
	sprintf(msg, "GET https://api.thingspeak.com/update?api_key=%s&field1=%s&field2=%s&field3=%s&field4=%s\r\n",API_KEY, temp_OUT_str, hum_OUT_str, temp_GWH_str, hum_GWH_str);

	send_String_A6(msg, 4000);
	
	char ctrlZ[2] = {(char) 0x1A, (char) 0x0D};
	send_String_A6((uint8_t *)ctrlZ, 2000);
	send_String_A6("AT+CIPCLOSE\r\n", 1000);
	send_String_A6("AT+CIPSHUT\r\n", 1000);
	//HAL_UART_DeInit(&gsmUART);
	//HAL_UART_AbortReceive_IT(&gsmUART); */
}

void reset_A6 (){
	digitalWrite(GSM_RESET_PIN, LOW);
	delay(GSM_RESET_TIME_MS);
	digitalWrite(GSM_RESET_PIN, HIGH);
}

int check_A6(void){
	char cmd[50];
	int flag = 1;
	int send_cnt = 0;
	char buf[50] = {0};
	//memset(buf,0,sizeof(buf));
	while(flag){
		memset(buf,0,sizeof(buf));
		pcUART.print("Sending cmd: ");
		sprintf(cmd,"AT\r\n");
		pcUART.print(cmd);
		gsmUART.print(cmd);
		
		int i = 0;
		while (gsmUART.available())
		{
			buf[i++] = gsmUART.read();
		}
		pcUART.println(buf);

		if(strstr(&buf[0],"OK")){
			pcUART.println("Module connected!");
			flag = 0;
		}
		delay(500);
		send_cnt++;

		if (send_cnt == 30){	//reset_A6 GSM board if not responding after GSM_MAX_SYNC_SENDS
			reset_A6();
			send_cnt = 0;
		}
	}
	return 1; 
}

void send_String_A6(const char* cmd, uint16_t timeout){
	//
}


void saveData(char message[], DHT_DataStore *data){
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
		}
    else if (strcmp("Temperature",attr)==0){
      data->temperature = atof(val);
    }
    else if (strcmp("Humidity",attr)==0){
			data->humidity = atof(val);
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

ISR (TIMER3_OVF_vect)
{
/* Toggle a pin on timer overflow */
  TCNT3 = 34286;
  tim3_ovf_cnt++;
  /* Serial.print("Timer 3 overflow, cnt: ");
  Serial.println(tim3_ovf_cnt); */
  if (tim3_ovf_cnt == GSM_INTERVAL_MIN*60/TIM3_OVF_HZ)  {
    tim3_ovf_cnt = 0;
    PUSH_DATA_GSM_FLAG = 1;
  }
}

ISR (TIMER4_OVF_vect)
{
  TCNT4 = 18661;
  if (!PUSH_DATA_GSM_FLAG){
    POLL_DATA_TRIGGER_FLAG = 1;
  }  
}

ISR (TIMER5_OVF_vect)
{
/* Toggle a pin on timer overflow */
  Serial.println("Timer 5 overflow");
}


int is_empty(char *buf, size_t size)
{
    return buf[0] == 0 && !memcmp(buf, buf + 1, size - 1);
}