#include <ESP8266_01.h>

#define SSID "ssid"
#define PASSWORD "password"

ESP8266_01 esp;

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);

  // ESP8266 init (serial esp, serial debug, debug on) 
  esp = ESP8266_01(Serial1, Serial, true);
  // Set as client only
  esp.setAsStation();
  esp.setSingleConnection();
  // esp.setMultipleConnection(); // Usefull for fast interaction with multiple site
  // Connect to wifi loop
  while(esp.isConnected().equals("")){
    esp.connectWifi(SSID, PASSWORD);
  }
  // Enable HTTP 302 redirect link
  esp.enableAutoRedirectUrl();
  // Get website request response
  // getSecureConnection( url, request address, port, wait response on, keep-alive connection on (for fast interaction))
  String response = esp.getSecureConnection("www.google.com", "/", 443, true, true);
  Serial.println(response);
}

void loop() {
  // put your main code here, to run repeatedly:

}
