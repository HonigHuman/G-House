#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define BAUD_RATE               9600
#define pcUART                  Serial
#define wifiUART                Serial1       
#define gsmUART                 Serial2

#define POLL_TRIGGER_PIN        8

#define GSM_INTERVAL_MIN        5 
#define TIM3_OVF_HZ             2
#define OUT_DATA_IDX						0
#define GWH_DATA_IDX						1
#define POL_DATA_IDX            2
#define GSM_RESET_TIME_MS 			300   //reset_A6 pin of gsm A6 pulled low for (ms)
#define GSM_MAX_SYNC_SENDS			10 		//maximum count of AT sends before resetting GSM module

typedef struct 
  {
		char id[4];
		float humidity, temperature;
  } DHT_DataStore, ptrDHT_DataStore;

//--------------- PFP --------------
void init_Timers();
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
int PUSH_GSM_DATA_FLAG = 0;
int POLL_DATA_TRIGGER_FLAG = 0;


DHT_DataStore init_data = {"-I-", 3.3, -99.9}; 
DHT_DataStore new_data = {"-N-", 3.3, -99.9};
DHT_DataStore last_OUT_data, last_GWH_data;
DHT_DataStore *last_data_sets[2] = {&init_data, &init_data};


void setup() {

  init_Timers();

  pcUART.begin(BAUD_RATE);
  wifiUART.begin(BAUD_RATE);
  gsmUART.begin(BAUD_RATE);

  start_Timer_OVF_IT(3);
  start_Timer_OVF_IT(4);
}

void loop() {
  if (cnt == 4){
				PUSH_DATA_GSM_FLAG = 1;
				cnt++;
	}

	if (POLL_DATA_TRIGGER_FLAG){
		cnt++;
			
		if (pollData_UART(&new_data) == 1){
				//saveData(wifi_rx_buffer, &new_data);
				display_DHTData_LCD(&new_data);//&new_data);
				//HAL_GPIO_WritePin(GPIOA, UART_TRIGGER_Pin, GPIO_PIN_RESET);
				n_rxd_data++;
		}
		POLL_DATA_TRIGGER_FLAG = 0;
	}

	if (PUSH_DATA_GSM_FLAG){
		send_GSM_Data();			
		PUSH_DATA_GSM_FLAG = 0;
	}		
  
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

ISR (TIMER3_OVF_vect)
{
/* Toggle a pin on timer overflow */
  TCNT3 = 34286;
  tim3_ovf_cnt++;
  Serial.print("Timer 3 overflow, cnt: ");
  Serial.println(tim3_ovf_cnt);
  if (tim3_ovf_cnt == GSM_INTERVAL_MIN*60/TIM3_OVF_HZ)  {
    tim3_ovf_cnt = 0;
    PUSH_GSM_DATA_FLAG = 1;
  }
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

ISR (TIMER4_OVF_vect)
{
  TCNT4 = 18661;
  if (!PUSH_GSM_DATA_FLAG){
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