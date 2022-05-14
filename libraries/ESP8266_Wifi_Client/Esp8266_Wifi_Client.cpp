//Library for ESP8266 client-server communication for Ghouse project
#include <ESP8266WiFi.h> 
#include "Esp8266_Wifi_Client.h"
#include <Arduino.h>


//------------------------------------------------------------------------------------

  // Authentication Variables
  char*         ssid = "DataTransfer";            // Wifi Name
  //char*         password;        // Wifi Password

//------------------------------------------------------------------------------------
  // WIFI Module Role & Port
  IPAddress     TCP_Server(192, 168, 4, 1);
  IPAddress     TCP_Gateway(192, 168, 4, 1);
  IPAddress     TCP_Subnet(255, 255, 255, 0);
  unsigned int  TCPPort = 2390;
  WiFiClient    TCP_Client;
  

//====================================================================================

void check_WiFi_and_Connect_or_Reconnect(){
  if (WiFi.status() != WL_CONNECTED){
 
    TCP_Client.stop();                                  //Make Sure Everything Is Reset
    WiFi.disconnect();
    Serial.println("Not Connected...trying to connect...");
    delay(50);
    WiFi.mode(WIFI_STA);                                // station (Client) Only - to avoid broadcasting an SSID ??
    WiFi.begin(ssid);                         // the SSID that we want to connect to
   
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

int send_Data_To_Server(char message_buffer[]){
  // first make sure you got disconnected
  int SUCCESS = 0;
  TCP_Client.stop();

  // if sucessfully connected send connection message
  if(TCP_Client.connect(TCP_Server, TCPPort)){

    //char message_buffer[43];
    
    Serial.print     (message_buffer);
    TCP_Client.print (message_buffer);
    SUCCESS = 1;
    }
  
  TCP_Client.setNoDelay(1);   
  return SUCCESS;                                  // allow fast communication?
}

void reset_WiFi_Connection(){
    TCP_Client.stop();                                  //Make Sure Everything Is Reset
    WiFi.disconnect();
}



//====================================================================================

/*void Tell_Server_we_are_there(){
  // first make sure you got disconnected
  TCP_Client.stop();

  // if sucessfully connected send connection message
  if(TCP_Client.connect(TCP_Server, TCPPort)){
    Serial.println    ("<" + Devicename + "-CONNECTED>");
    TCP_Client.println ("<" + Devicename + "-CONNECTED>");
  }
  TCP_Client.setNoDelay(1);                                     // allow fast communication?
}*/
//====================================================================================

