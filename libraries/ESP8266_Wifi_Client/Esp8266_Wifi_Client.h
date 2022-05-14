//Header file 
extern const String  Devicename;

#define       LED0                  2       // WIFI Module LED
#define       LED_ON                LOW     // LED is ON when LED Pin LOW
#define       LED_OFF               HIGH    

#define       MAX_CONNECT_ATTEMPTS  3
#define       CHECK_CONNECT_TIMEOUT 20000

void check_WiFi_and_Connect_or_Reconnect();
int send_Data_To_Server(char []);
void reset_WiFi_Connection();
//void tell_Server_we_are_there ();