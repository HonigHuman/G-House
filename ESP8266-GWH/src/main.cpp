//-- Libraries Included --------------------------------------------------------------
  #include <ESP8266WiFi.h>    // The Basic Function Of The ESP NOD MCU
  #include <DHTesp.h>  
  #include <Arduino.h>
  #include <Esp8266_Wifi_Client.h>


//------------------------------------------------------------------------------------
  // Define I/O Pins
  #define       DHT_PIN               4
  #define       LED0                  2       // WIFI Module LED
  #define       LED_ON                LOW     // LED is ON when LED Pin LOW
  #define       LED_OFF               HIGH    
  #define       BAUD_RATE             9600

//------------------------------------------------------------------------------------
  //PFs
  //void Check_WiFi_and_Connect_or_Reconnect();
  //void Send_Data_To_Server(char []);
  void get_DHT_Data_String(char []);
  void Setup_DHT22 ();

  // DHT Sensor
  DHTesp dht;

  // Device specific ID
  const String  Devicename = "OUT";
  uint64_t SLEEP_PERIOD_S = 120;  //number of seconds to sleep for before sending another data packet

  // Some Variables
  char mess_buff[43];
  unsigned long prevMillis_connect = 0;
  unsigned char buffer[80];
  char result[10];

//====================================================================================

void setup(){
  // setting the serial port ----------------------------------------------
  Serial.begin(BAUD_RATE);  
  
  // setting the mode of pins ---------------------------------------------
  pinMode(LED0, OUTPUT);                          // WIFI OnBoard LED Light
  digitalWrite(LED0, LED_ON);
  
  Setup_DHT22();
  
  // WiFi Connect ----------------------------------------------------
  check_WiFi_and_Connect_or_Reconnect();          // Checking For Connection

  if (WiFi.status() == WL_CONNECTED){

    get_DHT_Data_String(mess_buff);
    send_Data_To_Server(mess_buff);

    digitalWrite(LED0, LED_OFF);
    delay(1000);
    digitalWrite(LED0, LED_ON);                       // Turn WiFi LED Off
    delay(1000);
    digitalWrite(LED0, LED_OFF);

    reset_WiFi_Connection();

    Serial.println("Going to sleep");
    ESP.deepSleep(SLEEP_PERIOD_S * 1000000);
    yield();
  }
}

//====================================================================================
 
void loop(){
  //unsigned long currentMillis = millis();
  if (WiFi.status() != WL_CONNECTED)// and currentMillis - prevMillis_connect > CHECK_CONNECT_TIMEOUT)
  {
    check_WiFi_and_Connect_or_Reconnect();
  }
  
  if (WiFi.status() == WL_CONNECTED)
  {

    get_DHT_Data_String(mess_buff);
    send_Data_To_Server(mess_buff);

    digitalWrite(LED0, LED_OFF);
    delay(1000);
    digitalWrite(LED0, LED_ON);                       // Turn WiFi LED Off
    delay(1000);
    digitalWrite(LED0, LED_OFF);

    ESP.deepSleep(SLEEP_PERIOD_S * 1e6);
    yield();
  }
  
}


//====================================================================================

void get_DHT_Data_String(char message_buffer[]){

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
    
  //char message_buffer[43];
  sprintf(message_buffer, "Device:%s+Temperature:%s+Humidity:%s\n",devStr,tempStr,humStr);
  
  return;
}

//====================================================================================

/*void Send_DHT_Data_To_Server(){
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
}*/

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

