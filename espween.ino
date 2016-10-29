#include <WiFiClientSecure.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiSTA.h>
#include <WiFiUdp.h>
#include <WiFiServer.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <ESP8266WiFiScan.h>

#include <DNSServer.h>

#include <FastLED.h>
#include <ESP8266WebServer.h>

#define NUM_LEDS 50
#define NUM_LETTERS 26
#define NUM_COLORS 5

#define PHRASE_DELAY 3000
#define LETTER_DURATION 600
#define SPACE_DURATION 400
#define SHORT_SPACE_DURATION 200

//                                         A   B   C   D   E   F   G   H   I   J   K   L   M   N   O   P   Q   R   S   T   U   V   W   X   Y   Z
const int letterLedIndex[NUM_LETTERS] = { 46, 45, 44, 42, 41, 40, 39, 37, 19, 21, 22, 23, 24, 25, 27, 28, 31, 14, 12, 10,  9,  8,  6,  5,  4,  1 };
const CRGB letterColors[NUM_COLORS] = { CRGB::Red, CRGB::Yellow, CRGB::Green, CRGB::Blue, CRGB::Purple };

struct Display {
  CRGB m_leds[NUM_LEDS];
  int m_nextCharacterIndex;
  char m_message[128];
  unsigned long m_nextUpdate;
  unsigned int m_attractStage;
  
  Display() { }

  void setup() {
    m_nextUpdate = 0;
    m_nextCharacterIndex = 0;
    m_attractStage = 0;
    FastLED.addLeds<WS2812, 0>(m_leds, NUM_LEDS);
    clearMessage();
  }

  void setMessage( const String &str ) {
    char *out = m_message;
    char prev_char = '\0';
    const char *endOut = m_message + sizeof( m_message ) - 2;
    const char *in = str.c_str();
    
    while( *in && out < endOut )
    {
      char c = toupper( *in );
      if( c >= 'A' && c <= 'Z' ) {
        if( c == prev_char ) {
          *out = '_';
          out++;
        }
        *out = c;
        out++;
      } else if( c == ' ' ) {
        *out = c;
        out++;
      }
      prev_char = c;
      in++;
    }
    *out = '\0';
  
    Serial.printf( "Message changed: %s", m_message );
    Serial.println();

    m_nextCharacterIndex = 0;
    m_nextUpdate = millis() + 1000;
  }

  void clearMessage() {
    m_message[0] = '\0';
    m_nextUpdate = millis();
    m_attractStage = 0;
  }
  
  const char *getMessage() {
    return m_message;
  }

  void clearAll() {
    fill_solid( m_leds, NUM_LEDS, CRGB::Black );
  }

  void attractMode() {
    int start = ( m_attractStage % 2 ) * 23;
    fill_rainbow( m_leds, NUM_LEDS, 0, 23 );
    nscale8( m_leds, NUM_LEDS, 96 );
    FastLED.show();
    m_nextUpdate = millis() + 1000;
    m_attractStage++;
  }
  
  void update() {
    unsigned long ms = millis();

    if( ms < m_nextUpdate ) {
      return;
    }

    if( m_message[0] == '\0' ) {
      attractMode();
      return;
    }
    
    clearAll();

    int len = strlen( m_message );
    if( m_nextCharacterIndex >= len ) {
      m_nextUpdate = ms + PHRASE_DELAY;
      m_nextCharacterIndex = 0;
    } else {
      char c = m_message[m_nextCharacterIndex];
      if( c >= 'A' && c <= 'Z' ) {
        int idx = letterLedIndex[ c - 'A' ];
        m_leds[idx] = letterColors[ idx % NUM_COLORS ];
        m_nextUpdate = ms + LETTER_DURATION;
      } else if( c == '_' ) {
        m_nextUpdate = ms + SHORT_SPACE_DURATION;
      } else {
        m_nextUpdate = ms + SPACE_DURATION;
      }
      
      m_nextCharacterIndex++;
    }
    FastLED.show();
  }
};


ESP8266WebServer server(80);
Display display;
const byte DNS_PORT = 53;
DNSServer dnsServer;


void handleUpdate() {
  String msg = server.arg( "message" );
  display.setMessage( msg );

  server.sendHeader( "Location", "http://stranger.things/" );
  server.send( 301 );
}

void handleRoot() {
  char output[1024];
  sprintf( output, "\
<!doctype html>\
<html lang='en'>\
<head>\
  <meta charset='utf-8'>\
  <meta name='viewport' content='width=device-width'>\
  <title>Strange Things</title>\
</head>\
<body>\
  <h2>Current: %s</h2>\
  <form method='POST' action='/update'>\
    <input type='text' name='message'>\
    <input type='submit' value='Update'>\
  </form>\
</body>\
</html>", display.getMessage() );

  server.send(200, "text/html", output);
}

void setup() {
  delay(1000);
  
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");
  WiFi.softAP("StrangerThings");
  
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  dnsServer.start(DNS_PORT, "*", myIP);

  server.on("/", handleRoot);
  server.on("/update", handleUpdate);
  server.onNotFound( []() {
    server.sendHeader( "Location", "http://stranger.things/" );
    server.send( 301 );
  } );
  
  server.begin();
  Serial.println("HTTP server started");

  display.setup();
}

void loop() {
  server.handleClient();
  dnsServer.processNextRequest();
  display.update();
}
