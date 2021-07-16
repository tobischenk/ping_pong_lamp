// Library for ESP8266 Support
#include <ESP8266WiFi.h>
// Wifi Client Library
#include <WiFiClient.h>
// Include Timer Library
#include <FireTimer.h>
// Include SerialTransfer
#include <SerialTransfer.h>

#include "CommandParsing.h"
#include "TransferData.h"

// Timeout for Arduino Reply
#define MILLIS_UART 2000

// Maximum Characters of Messages between Arduino and ESP
#define BUFFER_SIZE 64


#define DEBUG true

// WiFi Data
const char* ssid = "WiiLan Sports";
const char* password = "langundweitbringtsicherheit";

// WiFi Objects
WiFiClient client;
WiFiServer server(1026);

// Timer to wait for Arduino Reply to send request
FireTimer uartTimer;

SerialTransfer myTransfer;

TransferData send_data;


void setup() {
  Serial.begin(115200);
  myTransfer.begin(Serial);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // LED does not Blink
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  if(DEBUG) Serial.print("Waiting to connect");

  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    if(DEBUG) Serial.print(".");
  }

  if(DEBUG) {
    Serial.print("\nConnected to: "); Serial.println(ssid);
    Serial.print("IP Address: "); Serial.println(WiFi.localIP());
  }

  server.begin();

  // LED Blinks when connected to WLAN
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  readTCP();
}

void readTCP() {
  client = server.available();

  if(client && client.connected()) {

    send_data.clear();
    
    client.write("Client Connected");

    // Read TCP Input as String
    String text = client.readStringUntil('\n');
    // Convert to Array
    char textArray[BUFFER_SIZE];
    memset(textArray, NULL, sizeof(char));
    for(int i = 0; i < text.length(); i++) {
      textArray[i] = text[i];
    }

    setStruct(textArray);

    uint16_t sendSize = 0;

    sendSize = myTransfer.txObj(send_data);
    myTransfer.sendData(sendSize);
    

    if (DEBUG) {
      Serial.print("\nPocessByte: "); Serial.print(send_data.processByte);
      Serial.print("\nMode: "); Serial.print(send_data.mode);
      Serial.print("\nBrightness: "); Serial.print(send_data.brightness);
      Serial.print("\nHours: "); Serial.print(send_data.hours);
      Serial.print("\nMinutes: "); Serial.print(send_data.minutes);
      Serial.print("\nSeconds: "); Serial.print(send_data.seconds);
      Serial.print("\nColor: "); Serial.print(send_data.color);
      Serial.print("\nSaturation: "); Serial.print(send_data.saturation);
    }

    delay(100);

    client.flush();
    client.stop();
  }
}

void setStruct(char text[BUFFER_SIZE]) {
    char *ptr;
    char delimiter[] = " ";

    ptr = strtok(text, delimiter);

    int processByte;

    int i = 0;

    while(ptr != NULL) {
        int ptrValueInt = atoi(ptr);
        byte ptrValue = byte(ptrValueInt);
        
        if (i == 0) {
          processByte = ptrValue;
          send_data.processByte = processByte;
        } else {
          switch(processByte) {
              case CP_MODE:
                  if(i == 1) {
                      send_data.mode = ptrValue;
                  }
                  break;
              case CP_BRIGHTNESS:
                  if(i == 1) {
                      send_data.brightness = ptrValue;
                  }
                  break;
              case CP_COLOR:
                  if(i == 1) {
                      send_data.color = ptrValue;
                  }
                  break;
              case CP_SATURATION:
                  if(i == 1){
                    send_data.saturation = ptrValue;
                  }
                  break;
              case CP_TIMER:
                  if(i == 1){
                      send_data.hours = ptrValue;
                  } else if(i == 2) {
                      send_data.minutes = ptrValue;
                  } else if(i == 3) {
                      send_data.seconds = ptrValue;
                  }
                  break;
          }
        }
        
        

        ptr = strtok(NULL, delimiter);
        i++;
    }
}
