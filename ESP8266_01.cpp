/*
	ESP8266_01.cpp
	Tested with ESP8266-01 AT firmware >= v2.2.0.0
	ESP8266 AT user guide can be found here: https://docs.espressif.com/projects/esp-at/en/release-v2.2.0.0_esp8266/
*/

#include "Arduino.h"
#include "Stream.h"
#include "String.h"
#include "ESP8266_01.h"


ESP8266_01::ESP8266_01(){
	serial = NULL;
	debug = NULL;
	debugOn = false;
}

ESP8266_01::ESP8266_01(Stream &serial, Stream &debug, bool debugOn): 
	serial(&serial), debug(&debug)
{
	this->debugOn = debugOn;
	if(init()){
		printToDebug("ESP8266 ready");
	}
	else{
		printToDebug("ESP8266 init ERROR");
	}
	enableStoringResponseServerData();
}

ESP8266_01::~ESP8266_01(){
}

ESP8266_01& ESP8266_01::operator=(const ESP8266_01 &esp)
{
  serial = esp.serial;
  debug = esp.debug;
  debugOn = esp.debugOn;
  return *this;
}

bool ESP8266_01::checkAck(String ack, int timeout){
	// PRIVATE
	// Used after sending a command to ESP8266 to check if it is fine
	// ack define the acknowledgement string with ESP8266 respond 
	// timeout (optional) define the max time waiting for acknowledgement
	_message = "";
	unsigned long currentMillis = millis();
	unsigned long startMillis = millis();
	// !_message.substring(_message.length()-(sizeof(ACK_OUT)+1),_message.length()-2).equals(ACK_OUT)
	while(currentMillis - startMillis < timeout && _message.lastIndexOf(ack) < 0 && _message.lastIndexOf(NACK_OUT) < 0){
		// Put in _message all ESP8266 send to the board
		// If find ack or nack stop
		if(serial->available()){
			_message += serial->readString();
			//printToDebug(_message);
		}
		currentMillis = millis();
	}
	if(currentMillis - startMillis >= timeout){
		printToDebug("Command response TIMEOUT");
		return false;
	}
	if(_message.lastIndexOf(ack) >= 0){
		// ack found -> command fine!
		// Depending on command type _message could contain information
		return true;
	}
	// command problem
	return false;
}
bool ESP8266_01::sendCommand(String command, String ack, bool waitAck, int timeout){
	// PUBLIC
	// Used for transmit command from board to ESP8266
	// ack (optional)
	// waitAck (optional) if true check command run correctly // NOTE: if false function ALWAYS return as command run correctly
	// timeout (optional)
	serial->flush();
	serial->print(command + "\r\n");
	if(waitAck){
		return(checkAck(ack, timeout));
	}
	else{
		return true;
	}
}
bool ESP8266_01::init(){
	// PUBLIC
	// Used for check communication board - ESP8266 is ok
	// If problem then:
	// 		- check if serial istance passed to ESP8266_01() is referring to the serial port which ESP8266 is connected to
	//		- check rx tx cable swap
	//		- check correct serial baudrate (ESP8266_01 normally use 115200 bit/s) (ESP8266 baudrate can be change with "AT+UART_DEF=115200,8,1,0,0" command) 
	return(sendCommand("AT", ACK_OUT));
}
void ESP8266_01::reset(){
	// PUBLIC
	// Restart module
	if(sendCommand("AT+RST", ACK_OUT)){
		printToDebug("Restart OK");
	}
	else{
		printToDebug("Restart ERROR");
	}
}
void ESP8266_01::setAsStation(){
	// PUBLIC
	// Set ESP8266 as client only // STA mode
	if(sendCommand("AT+CWMODE=1", ACK_OUT)){
		printToDebug("Mode station OK");
	}
	else{
		printToDebug("Mode station ERROR");
	}
}
void ESP8266_01::setAsAp(){
	// PUBLIC
	// Set ESP8266 as access point only // AP mode
	if(sendCommand("AT+CWMODE=2", ACK_OUT)){
		printToDebug("Mode ap OK");
	}
	else{
		printToDebug("Mode ap ERROR");
	}
}
void ESP8266_01::setAsStationAp(){
	// PUBLIC
	// Set ESP8266 as client and access point // STA + AP mode
	if(sendCommand("AT+CWMODE=3", ACK_OUT)){
		printToDebug("Mode station + ap OK");
	}
	else{
		printToDebug("Mode station + ap ERROR");
	}
}
void ESP8266_01::scanWifi(){
	// PUBLIC
	// STA or STA + AP mode ONLY
	// Print available wifis
	sendCommand("AT+CWLAP", ACK_OUT);
	cleanMessage(11, _message.length()); // 8 size of command
	printToDebug(_message);
}
String ESP8266_01::isConnected(){
	// PUBLIC
	// STA or STA + AP mode ONLY
	// Return wifi name if connected
	sendCommand("AT+CWJAP?", ACK_OUT);
	int i = _message.indexOf("+CWJAP:");
	
	if(i < 0){
		// No wifi connected
		return "";
	}
	else{
		cleanMessage(i, _message.length() - 4); // 4 = length of "/r/n" 
		return _message;
	}
}
bool ESP8266_01::connectWifi(String ssid, String password){
	// PUBLIC
	// STA or STA + AP mode ONLY
	// Connect an ap with name and password
	printToDebug("Connecting " + ssid + "...");
	if(sendCommand("AT+CWJAP=\"" + ssid + "\"" + "," + "\"" + password + "\"", ACK_OUT, true, 30000)){
		printToDebug(ssid + " connect success");
		return true;
	}
	else{
		printToDebug(ssid + " connect ERROR");
	}
	return false;
}
void ESP8266_01::setSingleConnection(){
	// PUBLIC
	// STA or STA + AP mode ONLY
	// Set module accept only 1 connection to a server at a time 
	if(sendCommand("AT+CIPMUX=0", ACK_OUT)){
		printToDebug("Single connection set OK");
	}
	else{
		printToDebug("Single connection set FAIL");
	}
}
void ESP8266_01::setMultipleConnection(){
	// PUBLIC
	// STA or STA + AP mode ONLY
	// Set module accept multiple simultaneous connection to servers 
	// Every connection is saved with an id [0,4] -> max 5 contemporary connections
	if(sendCommand("AT+CIPMUX=1", ACK_OUT)){
		printToDebug("Multiple connection set OK");
	}
	else{
		printToDebug("Multiple connection set FAIL");
	}
}
int ESP8266_01::getStatus(){
	// PUBLIC
	// STA or STA + AP mode ONLY
	// Get module state
	// 0: The ESP station is not initialized.
	// 1: The ESP station is initialized, but not started a Wi-Fi connection yet.
	// 2: The ESP station is connected to an AP and its IP address is obtained.
	// 3: The ESP station has created a TCP/SSL transmission.
	// 4: All of the TCP/UDP/SSL connections of the ESP device station are disconnected.
	// 5: The ESP station started a Wi-Fi connection, but was not connected to an AP or disconnected from an AP
	sendCommand("AT+CIPSTATUS", ACK_OUT);
	cleanMessage(_message.indexOf("\nSTATUS:") + 8, _message.lastIndexOf("\r"));
	return(_message.toInt());
}
void ESP8266_01::enableStoringResponseServerData(){
	// PUBLIC
	// STA or STA + AP mode ONLY
	// Store servers response otherwise response must be kept actively after sending the request   
	sendCommand("AT+CIPRECVMODE=1", ACK_OUT);
}
int ESP8266_01::getResponseServerDataLength(int id){
	// PRIVATE
	// STA or STA + AP mode ONLY
	// Get server connected saved with id response length
	sendCommand("AT+CIPRECVLEN?", ACK_OUT);
	// if single connection -> +CIPRECVLEN:length
	// if multiple connection -> +CIPRECVLEN:length,length,length,length,length
	cleanMessage(_message.indexOf("\n+CIPRECVLEN:") + 13, _message.lastIndexOf("\r"));
	String lengths[5];
	int StringCount = 0;
    while(_message.length() > 0){
      int index = _message.indexOf(',');
      if (index == -1) // No , found
      {
        lengths[StringCount++] = _message;
        break;
      }
      else{
        lengths[StringCount++] = _message.substring(0, index);
        _message = _message.substring(index+1);
      }
    }
	return(lengths[id].toInt());
}
void ESP8266_01::enableAutoRedirectUrl(){
	// PUBLIC
	// If response from server contain a HTTP 302 redirect url follow it -> new connection create
	redirectUrlOn = true;
}
void ESP8266_01::disableAutoRedirectUrl(){
	// PUBLIC
	// DEFAULT Ignore HTTP 302 redirect url in server response
	redirectUrlOn = false;
}
String ESP8266_01::getResponseServer(int id, bool keepAlive){
	// PRIVATE
	// Get response stored after request to server
	// Connected server saved with id
	String site;
	sendCommand("AT+CIPRECVDATA=" + String(id) + ",5000", ACK_OUT);
	if(redirectUrlOn){
		// HTTP 302 redirect url
		// If keepAlive true don't close connection
		int redirectUrlIndex = _message.indexOf("Location:");
		if(redirectUrlIndex > -1){
			cleanMessage(redirectUrlIndex, _message.length());
			cleanMessage(10, _message.indexOf("\r")); // 10 = "Location: " length
			site = _message;
			printToDebug("HTTP 302 redirect to link: " + site);
			if(!keepAlive){
				sendCommand("AT+CIPCLOSE=" + String(id), ACK_OUT);
			}
			getSecureConnection(site.substring(8,36), site.substring(36,site.length()), 443, true, keepAlive);
			// Get redirect response
			String response = _message;
			if(!keepAlive){
				sendCommand("AT+CIPCLOSE=" + String(id), ACK_OUT);
			}
			_message = response;
		}
	}
	cleanMessage(_message.indexOf("HTTP/"), _message.indexOf("\r\n\r\nOK"));
	return(_message);
	//printToDebug(_message);
}
bool ESP8266_01::sendRequestServer(int id, String host, String request){
	// PRIVATE
	// Send generic request to server named host connected with id
	// Generic HTTP request: 
	// 		GET url HTTP/1.1
	//		Host: host
	//		User-Agent: device
	String s = "GET " + request + " HTTP/1.1\r\nHost: " + host + "\r\nUser-Agent: ESP8266\r\n\r\n";
	sendCommand("AT+CIPSEND=" + String(id) + "," + String(s.length()), ACK_OUT_CIPSEND);
	for(int i=0;i<s.length();i+=100){
		// Divide message for better UART communication ( default ESP8266 max RX buffer = 128 bytes)
		serial->print(s.substring(i, i+100));
	}
	if(checkAck(ACK_OUT_SENDOK)){
		printToDebug("Request to host send OK");
		return(true);
	}
	printToDebug("Request to host send FAIL");
	return(false);
}
String ESP8266_01::getServerConnectedIp(int id){
	// PRIVATE
	// Get ip address of connected server with id
	sendCommand("AT+CIPSTATUS", ACK_OUT);
	int status = _message.substring(_message.indexOf("\nSTATUS:") + 8, _message.lastIndexOf("\r")).toInt();
	if(status == 3){ // 3 = connected to server
		cleanMessage(_message.indexOf("+CIPSTATUS:" + String(id)) + 20, _message.length());
		cleanMessage(0, _message.indexOf("\r"));
		cleanMessage(0, _message.lastIndexOf("\""));
		return(_message);
	}
	return("NULL");
}
String ESP8266_01::getDomainNameIp(String host){
	// PUBLIC
	// Return ip address from domain name
	if(sendCommand("AT+CIPDOMAIN=\"" + host + "\"", ACK_OUT)){
		cleanMessage(_message.indexOf("+CIPDOMAIN:") + 12, _message.lastIndexOf("\""));
		return(_message);
	}
	return("NULL");
}
int ESP8266_01::getHostId(String host){
	// PRIVATE
	// Return id connection if module connected to server host
	String ip = getDomainNameIp(host);
	sendCommand("AT+CIPSTATUS", ACK_OUT);
	int status = _message.substring(_message.indexOf("\nSTATUS:") + 8, _message.lastIndexOf("\r")).toInt();
	if(status == 3){ // 3 = connected to server
		int j = _message.indexOf(ip);
		if(j >= 0){
			cleanMessage(0, j);
			int i = _message.lastIndexOf(":");
			cleanMessage(i+1, i+2);
			return(_message.toInt());	
		}
		else{
			return(-1);
		}
	}
	return(-1);
}
String ESP8266_01::getSecureConnection(String host, String request, int port, bool waitResponse, bool keepAlive){
	// PUBLIC
	// Start a SSL request to host on port and return response if waitResponse
	// If keepAlive don't close connection to host (HTTP1.1 doesn't need header: "Connection: keep-alive")
	int id = getHostId(host);
	if(id < 0){
		if(sendCommand("AT+CIPSTARTEX=\"SSL\",\"" + host + "\"," + port, ACK_OUT, true, 20000)){
			id = getHostId(host);
			printToDebug("Host: " + host + " connect OK");
		}
		else{
			printToDebug("Host: " + host + " connect FAIL");
			return(NACK_OUT);
		}
	}
	if(sendRequestServer(id, host, request)){
		if(waitResponse){
			unsigned long currentMillis = millis();
			unsigned long startMillis = millis();
			while(getResponseServerDataLength(id) <= 0 && currentMillis - startMillis < RESPONSE_TIMEOUT){
				currentMillis = millis();
				delay(10);
			}
			if(currentMillis - startMillis >= RESPONSE_TIMEOUT){
				printToDebug("Server response TIMEOUT");
				return(NACK_OUT);
			}
			else{
				return(getResponseServer(id, keepAlive));
			}
		}
	}
	else{
		return(NACK_OUT);
	}
	if(!keepAlive){
		sendCommand("AT+CIPCLOSE=" + String(id), ACK_OUT);
	}
	return(ACK_OUT);
	//cleanMessage(11, _message.length()); // 8 size of command
	//printToDebug(_message);
}
void ESP8266_01::cleanMessage(int startIndex, int stopIndex){
	// PUBLIC
	_message = _message.substring(startIndex, stopIndex);
}
void ESP8266_01::printToDebug(String text){
	// PUBLIC
	if(debugOn){
		debug->println(text);
	}
}
