#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_I2CDevice.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <ESP8266WiFi.h>
#include <Esp8266_Wifi_Client.h>
 
#define OLED_RESET 0
#define SCREEN_WIDTH 64 // OLED display width, in pixels
#define SCREEN_HEIGHT 48 // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C
#define ONE_WIRE_BUS 12

#define       LED0                  2       // WIFI Module LED
#define       LED_ON                LOW     // LED is ON when LED Pin LOW
#define       LED_OFF               HIGH    
#define       BAUD_RATE             9600

#define TEMP_DISPLAY_TIMEOUT        5000 // ms the temperature gets displayed

//---------PFP--------------------------
void displayTemperatureValue(float tVal);
void get_DS18_Data_String(float temperature, char mess_buff[]);


// Device specific ID
  const String  Devicename = "POL";

  int DISPLAY_ON = 0;
  int DATA_SENT = 0;
  unsigned long startMillis = 0;
  uint64_t SLEEP_PERIOD_S = 20;  //number of seconds to sleep for before sending another data packet
//---------------------------------------

Adafruit_SSD1306 display(OLED_RESET);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


void setup() {
  // put your setup code here, to run once:
  Serial.begin(BAUD_RATE);

  sensors.begin();

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

  display.clearDisplay(); // for Clearing the display
  display.display();


  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");

  float tempC = sensors.getTempCByIndex(0);
  // Check if reading was successful
  while(tempC == DEVICE_DISCONNECTED_C){
    Serial.println("Error: Could not read temperature data. Retrying...");
    Serial.print("Requesting temperatures...");
    sensors.requestTemperatures(); // Send the command to get temperatures
    Serial.println("DONE");

    tempC = sensors.getTempCByIndex(0);
  }

  Serial.print("Temperature for the device 1 (index 0) is: ");
  Serial.println(tempC);

  displayTemperatureValue(tempC); //start displaying Temperature on OLED
  startMillis = millis();
  Serial.print("Timer start is ");
  Serial.println(startMillis);

  while(WiFi.status() != WL_CONNECTED){
    check_WiFi_and_Connect_or_Reconnect();
  }

  char mess_buff[43];
  get_DS18_Data_String(tempC, mess_buff);
  if (send_Data_To_Server(mess_buff)){
    DATA_SENT = 1;
  }
    
  digitalWrite(LED0, LED_OFF);      
    /*
    delay(1000);
    digitalWrite(LED0, LED_ON);                       // Turn WiFi LED Off
    delay(1000);
    digitalWrite(LED0, LED_OFF);*/
  reset_WiFi_Connection();



}

void loop() {
    
  unsigned long currentMillis = millis();
  if ((DISPLAY_ON == 1) and (currentMillis - startMillis > TEMP_DISPLAY_TIMEOUT)) //clear display and go to sleep if display time ellapsed
  {
    Serial.print("Timer end is ");
    Serial.println(currentMillis);
    display.clearDisplay();
    display.display();
    DISPLAY_ON = 0;
    Serial.println("Going to sleep");
    ESP.deepSleep(SLEEP_PERIOD_S * 1000000);
    yield();
  }

}

void get_DS18_Data_String(float temperature, char mess_buff[]){
  
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
  sprintf(mess_buff, "Device:%s+Temperature:%s\n",devStr,tempStr);
  
  return;
}




void displayTemperatureValue(float tVal){
  char buf[10];
  sprintf(buf, "%-.1f",tVal);
  display.clearDisplay(); // for Clearing the display
  display.setTextSize(2.5);
  display.setTextColor(WHITE);
  display.setCursor(0,15);
  display.print(buf);
  display.setTextSize(2);
  display.setCursor(52,15);
  display.print("C");
  display.drawCircle(49, 13, 2, WHITE);
  display.display();
  DISPLAY_ON = 1;

}
