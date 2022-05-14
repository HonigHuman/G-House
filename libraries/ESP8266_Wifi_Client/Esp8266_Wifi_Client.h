//Header file 
  #define       MAX_CONNECT_ATTEMPTS  3
  #define       CHECK_CONNECT_TIMEOUT 20000
  #define       BAUD_RATE             9600

  void Send_Request_To_Server();
  void Check_WiFi_and_Connect_or_Reconnect();
  void Send_DHT_Data_To_Server();
  void Tell_Server_we_are_there ();