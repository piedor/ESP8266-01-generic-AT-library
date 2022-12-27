/*
	ESP8266_01.h	HEADER file
	Tested with ESP8266-01 AT firmware >= v2.2.0.0
	ESP8266 AT user guide can be found here: https://docs.espressif.com/projects/esp-at/en/release-v2.2.0.0_esp8266/ or in docs folder
*/

#ifndef __ESP8266_01_H__
#define __ESP8266_01_H__

#include "Stream.h"
#include "String.h"

#define ACK_OUT "OK"
#define ACK_OUT_NOCHANGE "no change"
#define ACK_OUT_LINKED "Linked"
#define ACK_OUT_SENDOK "SEND OK"
#define ACK_OUT_CIPSEND ">"
#define NACK_OUT "ERROR"
#define NACK_OUT_WIFI "FAIL"
#define RESPONSE_TIMEOUT 10000 // 10 sec
#define MAX_LEN_RESPONSE_BUFFER 2147483647 //0x7fffffff

class ESP8266_01 {
  private:
  	String _message;
  	bool _multipleConnection;
  	
  	bool checkAck(String ack, int timeout = RESPONSE_TIMEOUT);
  	int getResponseServerDataLength(int id);
  	String getResponseServer(int id, bool keepAlive, int responseLength);
  	bool sendRequestServer(int id, String host, String request);
  	String getServerConnectedIp(int id);
  	int getHostId(String host);
    void cleanMessage(int startIndex, int stopIndex);
    
  public:
  	Stream* serial;
  	Stream* debug;
  	bool debugOn;
  	bool redirectUrlOn;
  	
	ESP8266_01();
  	ESP8266_01(Stream &serial, Stream &debug, bool debugOn = false);
	~ESP8266_01();
	ESP8266_01& operator=(const ESP8266_01 &esp);
	bool sendCommand(String command, String ack = ACK_OUT, bool waitAck = true, int timeout = RESPONSE_TIMEOUT);
    bool init();
    void reset();
    void setAsStation();
    void setAsAp();
    void setAsStationAp();
    void scanWifi();
    String isConnected();
    bool connectWifi(String ssid, String password);
    void setSingleConnection();
    void setMultipleConnection();
    int getStatus();
    void enableStoringResponseServerData();
    void enableAutoRedirectUrl();
    void disableAutoRedirectUrl();
    String getDomainNameIp(String host);
    String getSecureConnection(String host, String request, int port = 80, bool waitResponse = true, bool keepAlive = false);
    void printToDebug(String text);
};

#endif
