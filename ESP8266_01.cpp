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
	init();
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
	_message = "";
	unsigned long currentMillis = millis();
	unsigned long startMillis = millis();
	// !_message.substring(_message.length()-(sizeof(ACK_OUT)+1),_message.length()-2).equals(ACK_OUT)
	while(currentMillis - startMillis < timeout && _message.lastIndexOf(ack) < 0 && _message.lastIndexOf(NACK_OUT) < 0){
		if(serial->available()){
			_message += serial->readString();
		}
		currentMillis = millis();
	}
	if(currentMillis - startMillis >= timeout){
		printToDebug("Command response TIMEOUT");
		return false;
	}
	if(_message.lastIndexOf(ack) >= 0){
		//_message = _message.substring(0, _message.length()-(sizeof(ACK_OUT)+2));
		return true;
	}
	return false;
}
bool ESP8266_01::sendCommand(String command, String ack, bool waitAck, int timeout){
	serial->flush();
	serial->print(command + "\r\n");
	if(waitAck){
		return(checkAck(ack, timeout));
	}
	else{
		return true;
	}
}
void ESP8266_01::init(){
  if(sendCommand("AT", ACK_OUT)){
  	printToDebug("ESP8266 ready");
  }
  else{
  	printToDebug("ESP8266 init ERROR");
  }
}
void ESP8266_01::reset(){
	if(sendCommand("AT+RST", ACK_OUT)){
		printToDebug("Reset success");
	}
	else{
		printToDebug("Reset ERROR");
	}
}
void ESP8266_01::setAsStation(){
	// Set ESP8266 as client only
	if(sendCommand("AT+CWMODE=1", ACK_OUT)){
		printToDebug("Mode station success");
	}
	else{
		printToDebug("Mode station ERROR");
	}
}
void ESP8266_01::setAsAp(){
	// Set ESP8266 as access point only
	if(sendCommand("AT+CWMODE=2", ACK_OUT)){
		printToDebug("Mode ap success");
	}
	else{
		printToDebug("Mode ap ERROR");
	}
}
void ESP8266_01::setAsStationAp(){
	// Set ESP8266 as client and access point
	if(sendCommand("AT+CWMODE=3", ACK_OUT)){
		printToDebug("Mode station + ap success");
	}
	else{
		printToDebug("Mode station + ap ERROR");
	}
}
void ESP8266_01::scanWifi(){
	sendCommand("AT+CWLAP", ACK_OUT);
	cleanMessage(11, _message.length()); // 8 size of command
	printToDebug(_message);
}
String ESP8266_01::isConnected(){
	// return wifi name if connected
	sendCommand("AT+CWJAP?", ACK_OUT);
	int i = _message.indexOf("+CWJAP:");
	
	if(i < 0){
		return "";
	}
	else{
		cleanMessage(i, _message.length() - 4);
		return _message;
	}
}
bool ESP8266_01::connectWifi(String ssid, String password){
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
	sendCommand("AT+CIPMUX=0", ACK_OUT);
}
int ESP8266_01::getStatus(){
	sendCommand("AT+CIPSTATUS", ACK_OUT);
	cleanMessage(_message.indexOf("\nSTATUS:") + 8, _message.lastIndexOf("\r"));
	return(_message.toInt());
}
void ESP8266_01::enableStoringResponseServerData(){
	sendCommand("AT+CIPRECVMODE=1", ACK_OUT);
}
int ESP8266_01::getResponseServerDataLength(){
	sendCommand("AT+CIPRECVLEN?", ACK_OUT);
	cleanMessage(_message.indexOf("\n+CIPRECVLEN:") + 13, _message.lastIndexOf("\r"));
	return(_message.toInt());
}
void ESP8266_01::enableAutoRedirectUrl(){
	redirectUrlOn = true;
}
void ESP8266_01::disableAutoRedirectUrl(){
	redirectUrlOn = false;
}
String ESP8266_01::getResponseServer(){
	String site;
	sendCommand("AT+CIPRECVDATA=5000", ACK_OUT);
	if(redirectUrlOn){
		// HTTP 302 redirect url
		int redirectUrlIndex = _message.indexOf("Location:");
		if(redirectUrlIndex > -1){
			cleanMessage(redirectUrlIndex, _message.length());
			cleanMessage(10, _message.indexOf("\r"));
			site = _message;
			printToDebug("HTTP 302 redirect to link: " + site);
			sendCommand("AT+CIPCLOSE", ACK_OUT);
			getSecureConnection(site.substring(8,36), site.substring(36,site.length()), 443);
			// get redirect response
			String response = _message;
			sendCommand("AT+CIPCLOSE", ACK_OUT);
			_message = response;
		}
	}
	cleanMessage(_message.indexOf("HTTP/"), _message.indexOf("\r\n\r\nOK"));
	return(_message);
	//printToDebug(_message);
}
bool ESP8266_01::sendRequestServer(String host, String request){
	String s = "GET " + request + " HTTP/1.1\r\nHost: " + host + "\r\nUser-Agent: ESP8266\r\n\r\n";
	sendCommand("AT+CIPSEND=" + String(s.length()), ACK_OUT_CIPSEND);
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
String ESP8266_01::getServerConnectedIp(){
	// Get IP of connected host
	sendCommand("AT+CIPSTATUS", ACK_OUT);
	int status = _message.substring(_message.indexOf("\nSTATUS:") + 8, _message.lastIndexOf("\r")).toInt();
	if(status == 3){ // 3 = connected to server
		cleanMessage(_message.indexOf("+CIPSTATUS:0") + 20, _message.lastIndexOf("\r"));
		cleanMessage(0, _message.lastIndexOf("\""));
		return(_message);
	}
	return("NULL");
}
String ESP8266_01::getDomainNameIp(String host){
	// Return IP from domain name
	if(sendCommand("AT+CIPDOMAIN=\"" + host + "\"", ACK_OUT)){
		cleanMessage(_message.indexOf("+CIPDOMAIN:") + 12, _message.lastIndexOf("\""));
		return(_message);
	}
	return("NULL");
}
String ESP8266_01::getSecureConnection(String host, String request, int port, bool waitResponse, bool keepAlive){
	// Start a SSL request to host and return response if waitResponse
	// If keepAlive don't close connection to host (HTTP1.1 doesn't need header: "Connection: keep-alive")
	String actualIpConnected = getServerConnectedIp();
	if(!actualIpConnected.equals(getDomainNameIp(host))){
		if(sendCommand("AT+CIPSTART=\"SSL\",\"" + host + "\"," + port, ACK_OUT, true, 20000)){
			printToDebug("Host: " + host + " connect OK");
		}
		else{
			printToDebug("Host: " + host + " connect FAIL");
			return(NACK_OUT);
		}
	}
	if(sendRequestServer(host, request)){
		if(waitResponse){
			unsigned long currentMillis = millis();
			unsigned long startMillis = millis();
			while(getResponseServerDataLength() <= 0 && currentMillis - startMillis < RESPONSE_TIMEOUT){
				currentMillis = millis();
				delay(10);
			}
			if(currentMillis - startMillis >= RESPONSE_TIMEOUT){
				printToDebug("Server response TIMEOUT");
				return(NACK_OUT);
			}
			else{
				return(getResponseServer());
			}
		}
	}
	else{
		return(NACK_OUT);
	}
	if(!keepAlive){
		sendCommand("AT+CIPCLOSE", ACK_OUT);
	}
	return(ACK_OUT);
	//cleanMessage(11, _message.length()); // 8 size of command
	//printToDebug(_message);
}
void ESP8266_01::cleanMessage(int startIndex, int stopIndex){
	_message = _message.substring(startIndex, stopIndex);
}
void ESP8266_01::printToDebug(String text){
	if(debugOn){
		debug->println(text);
	}
}
