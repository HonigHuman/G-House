//Library for ESP8266 client-server communication for Ghouse project
#include <ESP8266WiFi.h> 
#include "Esp8266_Wifi_Client.h"

// Define I/O Pins
  #define       DHT_PIN               4
  #define       LED0                  2       // WIFI Module LED
  #define       LED_ON                LOW     // LED is ON when LED Pin LOW
  #define       LED_OFF               HIGH    
  #define       MAX_CONNECT_ATTEMPTS  3
  #define       CHECK_CONNECT_TIMEOUT 20000
  #define       BAUD_RATE             9600

//------------------------------------------------------------------------------------

  // Authentication Variables
  char*         ssid;            // Wifi Name
  char*         password;        // Wifi Password

//------------------------------------------------------------------------------------
  // WIFI Module Role & Port
  IPAddress     TCP_Server(192, 168, 4, 1);
  IPAddress     TCP_Gateway(192, 168, 4, 1);
  IPAddress     TCP_Subnet(255, 255, 255, 0);
  unsigned int  TCPPort = 2390;
  WiFiClient    TCP_Client;
  
//------------------------------------------------------------------------------------
  // Some Variables
  unsigned long prevMillis_connect = 0;
  unsigned char buffer[80];
  char result[10];
  uint64_t SLEEP_PERIOD_S = 120;  //number of seconds to sleep for before sending another data packet

//====================================================================================


//====================================================================================

void Send_Request_To_Server() {
unsigned long tNow;
    
  tNow=millis();
  dtostrf(tNow, 8, 0, result);                            // create a char[] out of the tNow

  TCP_Client.println(result);                             // Send Data

  while(1){
    int len = TCP_Client.available();                     // Check For Reply   
    if (len > 0) {
      if (len > 80){ 
        len = 80;
      }
      String line = TCP_Client.readStringUntil('\r');     // if '\r' is found
      Serial.print("received: ");                         // print the content
      Serial.println(line);
      break;                                              // exit
    }
    if((millis()-tNow)>1000){                             // if more then 1 Second No Reply -> exit
      Serial.println("timeout");
      break;                                              // exit
    }
  }                        

  TCP_Client.flush();                                     // Empty Bufffer 
  Check_WiFi_and_Connect_or_Reconnect();
}

//====================================================================================

void Check_WiFi_and_Connect_or_Reconnect(){
  if (WiFi.status() != WL_CONNECTED){
 
    TCP_Client.stop();                                  //Make Sure Everything Is Reset
    WiFi.disconnect();
    Serial.println("Not Connected...trying to connect...");
    delay(50);
    WiFi.mode(WIFI_STA);                                // station (Client) Only - to avoid broadcasting an SSID ??
    WiFi.begin("DataTransfer");                         // the SSID that we want to connect to
   
    int connect_cnt = 0;
    while(WiFi.status() != WL_CONNECTED){
      for(int i=0; i < 10; i++){
        digitalWrite(LED0, LED_OFF);
        delay(250);
        digitalWrite(LED0, LED_ON);
        delay(250);
        Serial.print(".");
      }
      Serial.println("");
      connect_cnt++;
      /*if (connect_cnt == MAX_CONNECT_ATTEMPTS)
      {
        Serial.println("Maximum Connection attempts, entering sleep for 10s!");
        prevMillis_connect = millis();
        digitalWrite(LED0, LED_OFF);
        return;
      }*/
      
    }
  // stop blinking to indicate if connected -------------------------------
    digitalWrite(LED0, LED_OFF);
    Serial.println("!-- Client Device Connected --!");

  // Printing IP Address --------------------------------------------------
    Serial.println("Connected To      : " + String(WiFi.SSID()));
    Serial.println("Signal Strenght   : " + String(WiFi.RSSI()) + " dBm");
    Serial.print  ("Server IP Address : ");
    Serial.println(TCP_Server);
    Serial.print  ("Device IP Address : ");
    Serial.println(WiFi.localIP());
 
  // conecting as a client -------------------------------------
    //Tell_Server_we_are_there();
  }
}

//====================================================================================

void Send_DHT_Data_To_Server(){
  // first make sure you got disconnected
  TCP_Client.stop();

  // if sucessfully connected send connection message
  if(TCP_Client.connect(TCP_Server, TCPPort)){
    float humidity = dht.getHumidity();
    float temperature = dht.getTemperature();
    if (humidity == NAN || temperature == NAN)
    {
      //return;
    }
    //char param_del_char = '+';
    char humStr[6];
    if (humidity < 10.0)
    {
      sprintf(humStr, "0%.1f", humidity);
    }
    else 
    {
      sprintf(humStr, "%.1f", humidity);
    }

    char tempStr[6];
    if (temperature < 10.0 and temperature >= 0.0)
    {
      sprintf(tempStr, "00%.1f", temperature);
    }
    else if ((temperature < 0.0 and temperature > -10.0)||(temperature > 10.0))
    {
      sprintf(tempStr, "0%.1f", temperature);
    }
    else
    {
      sprintf(tempStr, "%.1f", temperature);
    }
    char devStr[4];
    sprintf(devStr,"%s",Devicename.c_str());
    
    char message_buffer[43];
    sprintf(message_buffer, "Device:%s+Temperature:%s+Humidity:%s\n",devStr,tempStr,humStr);
    Serial.print     (message_buffer);
    TCP_Client.print (message_buffer);
   

    }
  
  TCP_Client.setNoDelay(1);                                     // allow fast communication?
}

//====================================================================================

void Tell_Server_we_are_there(){
  // first make sure you got disconnected
  TCP_Client.stop();

  // if sucessfully connected send connection message
  if(TCP_Client.connect(TCP_Server, TCPPort)){
    Serial.println    ("<" + Devicename + "-CONNECTED>");
    TCP_Client.println ("<" + Devicename + "-CONNECTED>");
  }
  TCP_Client.setNoDelay(1);                                     // allow fast communication?
}
//====================================================================================

