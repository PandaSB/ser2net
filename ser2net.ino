/*
  ESP8266 mDNS serial wifi bridge - March 2016 - BARTHELEMY Stéphane
  base on  Daniel Parnell code 
 */

#define BONJOUR_SUPPORT
#define USE_WDT

#include <ESP8266WiFi.h>
#ifdef BONJOUR_SUPPORT
#include <ESP8266mDNS.h>
#endif
#include <WiFiClient.h>
#include <config.h>

// serial end ethernet buffer size
#define BUFFER_SIZE 128

#ifdef BONJOUR_SUPPORT
// multicast DNS responder
MDNSResponder mdns;
#endif

WiFiServer server(TCP_LISTEN_PORT);

void connect_to_wifi() {
  int count = 0;
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // Wait for WIFI connection
  while (WiFi.status() != WL_CONNECTED) {
#ifdef USE_WDT
    wdt_reset();
#endif
    delay(250);
  }
}

void error() {
  int count = 0;
  
  while(1) {
    count++;
    delay(100);
  }
}

void setup(void)
{  
#ifdef USE_WDT
  wdt_enable(1000);
#endif
  
  Serial.begin(BAUD_RATE);
  
  // Connect to WiFi network
  connect_to_wifi();

#ifdef BONJOUR_SUPPORT
  // Set up mDNS responder:
  if (!mdns.begin(DEVICE_NAME, WiFi.localIP())) {
    error();
  }
#endif

  // Start TCP server
  server.begin();
}

WiFiClient client;

void loop(void)
{
  size_t bytes_read;
  uint8_t net_buf[BUFFER_SIZE];
  uint8_t serial_buf[BUFFER_SIZE];
  
#ifdef USE_WDT
  wdt_reset();
#endif

  if(WiFi.status() != WL_CONNECTED) {
    // we've lost the connection, so we need to reconnect
    if(client) {
      client.stop();
    }
    connect_to_wifi();
  }

#ifdef BONJOUR_SUPPORT
  // Check for any mDNS queries and send responses
  mdns.update();
#endif

  // Check if a client has connected
  if (!client) {
    // eat any bytes in the serial buffer as there is nothing to see them
    while(Serial.available()) {
      Serial.read();
    }
      
    client = server.available();
    if(!client) {         
      return;
    }

  }
#define min(a,b) ((a)<(b)?(a):(b))
  if(client.connected()) {
    // check the network for any bytes to send to the serial
    int count = client.available();
    if(count > 0) {      

      
      bytes_read = client.read(net_buf, min(count, BUFFER_SIZE));
      Serial.write(net_buf, bytes_read);
      Serial.flush();
    } 
    
    // now check the serial for any bytes to send to the network
    bytes_read = 0;
    while(Serial.available() && bytes_read < BUFFER_SIZE) {
      serial_buf[bytes_read] = Serial.read();
      bytes_read++;
    }
    
    if(bytes_read > 0) {  
      client.write((const uint8_t*)serial_buf, bytes_read);
      client.flush();
    }
  } else {
    client.stop();
  }
}
