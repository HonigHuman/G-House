
//-- Libraries Included --------------------------------------------------------------
  #include <ESP8266WiFi.h>    // The Basic Function Of The ESP NOD MCU
  #include <DHTesp.h>  

//------------------------------------------------------------------------------------
  // Define I/O Pins
  #define       LED0      4       // WIFI Module LED

//------------------------------------------------------------------------------------
  // Authentication Variables
  char*         ssid;            // Wifi Name
  char*         password;        // Wifi Password
  const String  Devicename = "GWH";

//------------------------------------------------------------------------------------
  // WIFI Module Role & Port
  IPAddress     TCP_Server(192, 168, 4, 1);
  IPAddress     TCP_Gateway(192, 168, 4, 1);
  IPAddress     TCP_Subnet(255, 255, 255, 0);

  unsigned int  TCPPort = 2390;

  WiFiClient    TCP_Client;
  
//------------------------------------------------------------------------------------
  // DHT Sensor
  DHTesp dht;

//------------------------------------------------------------------------------------
  // Some Variables
  unsigned char buffer[80];
  char result[10];

//====================================================================================

void setup(){
  // setting the serial port ----------------------------------------------
  Serial.begin(115200);  
  
  // setting the mode of pins ---------------------------------------------
  pinMode(LED0, OUTPUT);                          // WIFI OnBoard LED Light
  digitalWrite(LED0, !LOW);                       // Turn WiFi LED Off

  Setup_DHT22();
  
  // WiFi Connect ----------------------------------------------------
  Check_WiFi_and_Connect_or_Reconnect();          // Checking For Connection
}

//====================================================================================
 
void loop(){
  
  delay(dht.getMinimumSamplingPeriod());

  Send_DHT_Data_To_Server();
  Send_Request_To_Server();

}

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
   
    while(WiFi.status() != WL_CONNECTED){
      for(int i=0; i < 10; i++){
        digitalWrite(LED0, !HIGH);
        delay(250);
        digitalWrite(LED0, !LOW);
        delay(250);
        Serial.print(".");
      }
      Serial.println("");
    }
  // stop blinking to indicate if connected -------------------------------
    digitalWrite(LED0, !HIGH);
    Serial.println("!-- Client Device Connected --!");

  // Printing IP Address --------------------------------------------------
    Serial.println("Connected To      : " + String(WiFi.SSID()));
    Serial.println("Signal Strenght   : " + String(WiFi.RSSI()) + " dBm");
    Serial.print  ("Server IP Address : ");
    Serial.println(TCP_Server);
    Serial.print  ("Device IP Address : ");
    Serial.println(WiFi.localIP());
 
  // conecting as a client -------------------------------------
    Tell_Server_we_are_there();
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
    Serial.println    (Devicename + " -> DHT Humidity: "+humidity);
    Serial.println    (Devicename + " -> DHT Temperature: "+temperature);

    TCP_Client.println (Devicename + " -> DHT Humidity: "+humidity);
    TCP_Client.println (Devicename + " -> DHT Temperature: "+temperature);
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

void Setup_DHT22()
{
  Serial.println();
  Serial.println("Status\tHumidity (%)\tTemperature (C)\t(F)\tHeatIndex (C)\t(F)");
  String thisBoard= ARDUINO_BOARD;
  Serial.println(thisBoard);

  // Autodetect is not working reliable, don't use the following line
  // dht.setup(17);
  // use this instead: 
  dht.setup(4, DHTesp::DHT22); // Connect DHT sensor to GPIO 17
}
