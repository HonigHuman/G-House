//-- Libraries Included --------------------------------------------------------------
  #include <ESP8266WiFi.h>
  #include <Arduino.h>
  #include <uart_register.h>
  #include <string.h>
  #include <string>
  #include <cstring>

//------------------------------------------------------------------------------------
  // Define I/O Pins
  #define     LED0                  2         // WIFI Module LED
  #define     TX                    1
  #define     RX                    3     
  #define     DATA_POLLING_FLAG_PIN 5        //Interrupt Trigger Pin (D1)
  #define     LED_ON                LOW     // LED is ON when LED Pin LOW
  #define     LED_OFF               HIGH    

  #define     BAUD_RATE             9600
  

//------------------------------------------------------------------------------------
  //Private function declaration
  void SetWifi(char* Name, char* Password);
  void HandleClients();
  void poll_Data_ISR();
  void sendData_UART(String message);
  void serialEventHandler();
  char* get_Device_ID(char message[]);
  char* copyString(char s[]);





//------------------------------------------------------------------------------------
  // Authentication Variables
  char*       ssid;              // SERVER WIFI NAME
  char*       password;          // SERVER PASSWORD
//------------------------------------------------------------------------------------
  // WiFi settings
  #define     MAXSC     6           // MAXIMUM NUMBER OF CLIENTS

  IPAddress APlocal_IP(192, 168, 4, 1);
  IPAddress APgateway(192, 168, 4, 1);
  IPAddress APsubnet(255, 255, 255, 0);

  unsigned int TCPPort = 2390;
 
  WiFiServer  TCP_SERVER(TCPPort);      // THE SERVER AND THE PORT NUMBER
  WiFiClient  TCP_Client[MAXSC];        // THE SERVER CLIENTS Maximum number
//------------------------------------------------------------------------------------

  // Variables
  char result[10];
  volatile byte indx;

  int DATA_POLLING_FLAG = 0;
  int REPRINT_MESSAGE_FLAG = 0;
  int CURRENT_DATA_SLOT = 0;


  String current_msg_GWH;
  String current_msg_OUT;
  String saved_messages[2];


void setup(){

  // Setting the serial port
  Serial.begin(BAUD_RATE);           // Computer Communication
    
  // Setting the mode of the pins
  pinMode(LED0, OUTPUT);          // WIFI OnBoard LED Light
    
  // setting up a Wifi AccessPoint
  SetWifi("DataTransfer","");
  attachInterrupt(digitalPinToInterrupt(DATA_POLLING_FLAG_PIN), poll_Data_ISR, RISING);
 
  current_msg_GWH = "Device:SGS+Humidity:00.0+Temperature:000.0";
  current_msg_OUT = "Device:SOS+Humidity:00.0+Temperature:000.0";

}

//====================================================================================
//String current_message = "Device:-D-_Humidity:09.9_Temperature:099.9";
void loop(){
  
  /*else if(Serial.available()){  //listening to STM32 UART CMD (only used when STM32 uses poll_DATA_UART_Serial)
    serialEventHandler();
  } */
  
  HandleClients(); 

}

//====================================================================================
 
void SetWifi(char* Name, char* Password){
  // Stop any previous WIFI
  WiFi.disconnect();

  // Setting The Wifi Mode
  WiFi.mode(WIFI_AP_STA);
  Serial.println("WIFI Mode : AccessPoint");
  
  // Setting the AccessPoint name & password
  ssid      = Name;
  password  = Password;
   
  // Starting the access point
  WiFi.softAPConfig(APlocal_IP, APgateway, APsubnet);                 // softAPConfig (local_ip, gateway, subnet)
  WiFi.softAP(ssid, password, 1, 0, MAXSC);                           // WiFi.softAP(ssid, password, channel, hidden, max_connection)     
  Serial.println("WIFI < " + String(ssid) + " > ... Started");
   
  // wait a bit
  delay(50);
   
  // getting server IP
  IPAddress IP = WiFi.softAPIP();
  
  // printing the server IP address
  Serial.print("AccessPoint IP : ");
  Serial.println(IP);

  // starting server
  TCP_SERVER.begin();                                                 // which means basically WiFiServer(TCPPort);
  
  Serial.println("Server Started");
}

//====================================================================================
void serialEventHandler(){
  String input_str = Serial.readStringUntil('\n');
  if (input_str == "GETDATA" || input_str == "\nGETDATA"){
    sendData_UART(current_msg_GWH);
  }
}


void HandleClients(){
unsigned long tNow;
       
  if(TCP_SERVER.hasClient()){
    //Serial.print("Have client!");
    WiFiClient TCP_Client = TCP_SERVER.available();
    TCP_Client.setNoDelay(1);           
                                // enable fast communication
    while(1){
      //---------------------------------------------------------------
      // If clients are connected
      //---------------------------------------------------------------
      if(TCP_Client.available()){
        // read the message
        String messageStr = TCP_Client.readStringUntil('\n');
       
        if (REPRINT_MESSAGE_FLAG==1) {
          Serial.print("ESP Received Message: \n");
          Serial.println(messageStr);
        }
        char msg_cpy[43];         
        strcpy(msg_cpy, &messageStr[0]);
        
        char *sender_id = get_Device_ID(&msg_cpy[0]);
        
        if (strcmp("GWH", sender_id)==0){
          current_msg_GWH = messageStr;
        }
        else if (strcmp("OUT", sender_id)==0){
          current_msg_OUT = messageStr;
        }
                               // important to use println instead of print, as we are looking for a '\r' at the client
        TCP_Client.flush();
        break;
      }
       
      //---------------------------------------------------------------
      // If clients are disconnected                                            // does not realy work....
      //---------------------------------------------------------------     
      if(!TCP_Client || !TCP_Client.connected()){
        // Here We Turn Off The LED To Indicated The Its Disconnectted
        digitalWrite(LED0, LED_OFF);
        break;
      }
      
    }   
  }
  else{
    //Serial.println("Have no Client");
    //current_message = init_message;
    // the LED blinks if no clients are available
    /*digitalWrite(LED0, HIGH);
    delay(250);
    digitalWrite(LED0, LOW);
    delay(250); */
  }
}

//Sends a dht data struct via UART to STM32
void sendData_UART(String message){
  Serial.print(message);
  return;
}


IRAM_ATTR void poll_Data_ISR()
{
  //DATA_POLLING_FLAG = 1;
  if (CURRENT_DATA_SLOT == 0) {
      sendData_UART(current_msg_GWH);
      CURRENT_DATA_SLOT = 1;
      return;
    }
  else if (CURRENT_DATA_SLOT == 1) {
      sendData_UART(current_msg_OUT);
      CURRENT_DATA_SLOT = 0;
      return;
    }
  //DATA_POLLING_FLAG = 0;
}

char *get_Device_ID(char message[]){ 
  
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
		val = strtok(NULL, ":");

		if (strcmp("Device",attr)==0){

			//strcpy(ret_id, val);
		  return val;
		}      
  } 
  char *ret = "...";
  return ret;
}
