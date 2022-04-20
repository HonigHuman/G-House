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
  void ISR();
  void sendData_UART(String message);
  void serialEventHandler();




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

  char init_message[43];
  String current_message;
  String saved_messages[2];


void setup(){

  // Setting the serial port
  Serial.begin(BAUD_RATE);           // Computer Communication
    
  // Setting the mode of the pins
  pinMode(LED0, OUTPUT);          // WIFI OnBoard LED Light
    
  // setting up a Wifi AccessPoint
  SetWifi("DataTransfer","");
  attachInterrupt(digitalPinToInterrupt(DATA_POLLING_FLAG_PIN), ISR, RISING);
 
  sprintf(init_message, "Device:SSS+Humidity:09.9+Temperature:099.9");  //setting init message for startup and when no client is connected
  current_message = init_message;
}

//====================================================================================
//String current_message = "Device:-D-_Humidity:09.9_Temperature:099.9";
void loop(){
  if (DATA_POLLING_FLAG == 1) {
    
    //Serial.println("TX Interrupt!");
    sendData_UART(current_message);
    DATA_POLLING_FLAG = 0;
  }
  else if(Serial.available()){  //listening to STM32 UART CMD (only used when STM32 uses poll_DATA_UART_Serial)
    serialEventHandler();
  }
  
  HandleClients(); 
  
  /* if (Serial.available() > 0) {
    byte c = Serial.read();
    if (indx < sizeof buff) {
      buff [indx++] = c; // save data in the next index in the array buff
    }
  } */
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
    sendData_UART(current_message);
  }
}


void HandleClients(){
unsigned long tNow;
       
  if(TCP_SERVER.hasClient()){
    //Serial.print("Have client!");
    WiFiClient TCP_Client = TCP_SERVER.available();
    TCP_Client.setNoDelay(1);                                          // enable fast communication
    while(1){
      //---------------------------------------------------------------
      // If clients are connected
      //---------------------------------------------------------------
      if(TCP_Client.available()){
        // read the message
        String messageStr = TCP_Client.readStringUntil('\n');
        //char message[43]; 
        //sprintf(message, "%s", messageStr);
       
/*         // print the message on the screen
        Serial.print("Received packet of size ");
        Serial.println(sizeof(Message));
        
        // print who sent it
        Serial.print("From ");
        Serial.print(TCP_Client.remoteIP());
        Serial.print(", port ");
        Serial.println(TCP_Client.remotePort());
 */
        // content
        if (REPRINT_MESSAGE_FLAG==1) {
          Serial.print("ESP Received Message: \n");
          Serial.println(messageStr);
        }
        
        current_message = messageStr;                          // important to use println instead of print, as we are looking for a '\r' at the client
        TCP_Client.flush();
        
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
    current_message = init_message;
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


IRAM_ATTR void ISR()
{
  DATA_POLLING_FLAG = 1;
  sendData_UART(current_message);
  DATA_POLLING_FLAG = 0;
}
