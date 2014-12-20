/**************
 * Adafruit CC300 and 5 neopixels on an Arduino UNO
 * Christmas Tree Star
 * Sparkle nicely the current Cheerlights color
 * @presence
 *************/
 
#include <Adafruit_NeoPixel.h> // 5 Tree-topper lights (or 40 neopixel shield)
#include <Adafruit_CC3000.h> // wifi
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
 
#define PIN 6  // Neopixels connect here
#define Pixels 5  // Neopixel count
#define WEBSITE      "api.thingspeak.com"
#define WEBPAGE      "/channels/1417/field/1/last.txt"
//#define WLAN_SSID       "SBP Guests"  // cannot be longer than 32 characters!
#define WLAN_SSID       "iRev.net"  // cannot be longer than 32 characters!
#define WLAN_PASS       ""
#define WLAN_SECURITY   WLAN_SEC_UNSEC // WLAN_SEC_UNSEC, WLAN_SEC_WPA or WLAN_SEC_WPA2

// Use this Neopixel for normal Shields and Neopixels
//Adafruit_NeoPixel strip = Adafruit_NeoPixel(Pixels, PIN, NEO_GRB + NEO_KHZ800);

// use this Neopixel setup for the "Through-the-hole" 5-mm LED WS2811 critters.
Adafruit_NeoPixel strip = Adafruit_NeoPixel(Pixels, PIN, NEO_RGB + NEO_KHZ400);

#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIV2); // you can change this clock speed
#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data

// Variable declarations
long interval = 15 * 1000; // ms time to refresh CheerLights URL (60k is 1 minute)
unsigned long previousMillis = 0; // last time update
int color = 0; // somehow 32-bit number to represent hex codes?
int pixel = 0; // which pixel are we coloring
int displayColor = 0; // temp after randomizations
float displayFade = 0.000; // temp after randomizations, maybe
int fadeRate = 10;  // 1 low, 10 max // brightness of the ornamental tree topper
uint32_t ip; // IP address


void setup() {
	Serial.begin(9600);  // debugging, but seemingly slows down the LED refresh
	strip.begin();
	strip.show(); // Initialize all pixels to 'off'
	strip.setPixelColor(0,0,0,255); // starting up blue
	strip.show();
	if (!cc3000.begin()) {
		strip.setPixelColor(0,255,0,0); // red for wifi shield failure
		strip.show();
		while(1);
	}
	if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    	strip.setPixelColor(0,255,102,0); // orange for WTF Wifi
    	strip.show();
    	while(1);
	}
	strip.setPixelColor(0,255,222,0); // yellow while waiting on DHCP
	strip.show();
	while (!cc3000.checkDHCP()) {
    	delay(100); // idle till DCHP is delivered
	}
	strip.setPixelColor(0,0,255,0); // green for DHCP Found
	strip.show();
}
 
void loop () {
	unsigned long currentMillis = millis();
	if(currentMillis - previousMillis > interval) {
		previousMillis = currentMillis;
		color = getCheerLightsColor(color);
		Serial.println(color);
	}
 	//color++;  // or whatever the current cheerlights is.  color++ just rainbow cycles
	sparkle(color);
}
 
void sparkle(int color) {
	for(pixel=0; pixel < Pixels; pixel++) {
		if (random(0,1) == 1) {
			displayColor = color + random(20);  // color hue sparkles
			displayFade = fadeRate * random(60,100) * 0.001; // brightness 
		} else {
			displayColor = color - random(20);
			displayFade = fadeRate * random(60,100) * 0.001;
		}
		strip.setPixelColor(pixel, Wheel(displayColor, displayFade));
		strip.show();
		if (Pixels < 10) {
			delay(random(5,40));  // small number of LEDS be less frenetic
		}
	}
	delay(random(10,30)); // dunno, something to minimize the changes
}

int getCheerLightsColor(int color) {
	strip.setPixelColor(0,0,0,0); // go black on first pixel to show something happening
	strip.show();
	if (!cc3000.checkConnected()) {
		// get back on Wifi
		strip.setPixelColor(0,255,0,100); // Purple for Wifi Go
      	strip.show();
		cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);
		while (!cc3000.checkDHCP()) {
      		strip.setPixelColor(0,255,102,0);
      		strip.show();
    		delay(50); // idle till DCHP is delivered
    		strip.setPixelColor(0,255,0,0);
    		strip.show();
    		delay(50);
		}
	}
	while (ip == 0) {
		if (! cc3000.getHostByName(WEBSITE, &ip)) {
      		strip.setPixelColor(0,255,0,0); // can't resolve ip addy
      		strip.show();
		}
    	delay(500); // wait and try again.
	}
	strip.setPixelColor(0,0,200,0); // green for IP found
	strip.show();
	Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80);
	if (www.connected()) {
		strip.setPixelColor(0,0,0,200); // connected, fetching data
		strip.show();
		www.fastrprint(F("GET "));
		www.fastrprint(WEBPAGE);
		www.fastrprint(F(" HTTP/1.1\r\n"));
		www.fastrprint(F("Host: ")); www.fastrprint(WEBSITE); www.fastrprint(F("\r\n"));
		www.fastrprint(F("User-Agent: Presence's Arduino Cheerlights (presence@irev.net)\r\n"));
		www.fastrprint(F("Connection: close\r\n")); // make web server disconnect me, since cc3000 sucks at disconnecting itself
		www.fastrprint(F("\r\n"));
		www.println();
	} else {
		strip.setPixelColor(0,255,102,0); // orange for waiting on connection
		strip.show();
	}
	/* Read data until either the connection is closed, or the idle timeout is reached. */ 
	unsigned long lastRead = millis();
	String response = String("");
	while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
		char headerBreak[] = { 13, 10 }; // what separates http header from body, \r\n
		char prevChar[4]; // buffer of characters to compare with the headerbreak characters
		boolean gogo = false; // when its okay to start reading into the result String object
		while (www.available()) {
			char c = www.read();
			if (gogo) {
				strip.setPixelColor(0,0,200,0); // flash with data received
				strip.show();
				response.concat(c); // append that char to the string response
				Serial.print(c);
			} else {
				strip.setPixelColor(0,0,200,200); // flash with header received
				strip.show();
				// This is a bunch of http header uselessness
				prevChar[3] = prevChar[2];
				prevChar[2] = prevChar[1];
				prevChar[1] = prevChar[0];
				prevChar[0] = c;
				if ((prevChar[3] == headerBreak[0]) && (prevChar[2] == headerBreak[1]) && (prevChar[1] == headerBreak[0]) && (prevChar[0] == headerBreak[1])) {
					// I just saw the \r\n\r\n break between http header and content!
					gogo = true;
				}
			}
			lastRead = millis();
		}
	}
	strip.setPixelColor(0,0,0,0); // all done with data fetch
	strip.show();
	if (sizeof(response) < 49) { // if the response buffer is huge, then it was an error page.
		int newColor = processLightCommand(response); // Send the http response body content over to parse for a color
		color = newColor;
	}
	www.close(); // this is important: only 3 connections at once.
	//cc3000.disconnect();  // no, don't wanna disconnect from wifi.
	return (color);
}

int processLightCommand(String &response) {
	Serial.println(response);
	int color = 0;
	if (response.indexOf("white") > 0) {
		color = 230;
	}
	if (response.indexOf("black") > 0) {
		color = 120;
	}
	if (response.indexOf("red") > 0) {
		color = 90;
	}
	if (response.indexOf("green") > 0) {
		color = 1;
	}
	if (response.indexOf("blue") > 0) {
		color = 180;
	}
	if (response.indexOf("cyan") > 0) {
		color = 220;
	}
	if (response.indexOf("magenta") > 0) {
		color = 160;
	}
	if (response.indexOf("yellow") > 0) {
		color = 50;
	}
	if (response.indexOf("purple") > 0) {
		color = 130;
	}
	if (response.indexOf("orange") > 0) {
		color = 70;
	}
	if (response.indexOf("warmwhite") > 0) {
		color = 40;
	}
	if (response.indexOf("pink") > 0) {
		color = 120;
	}
	if (response.indexOf("oldlace") >0) {
		color = 40;
	}
	return(color);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos, float opacity) {
  
  if(WheelPos < 85) {
    return strip.Color((WheelPos * 3) * opacity, (255 - WheelPos * 3) * opacity, 0);
  } 
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color((255 - WheelPos * 3) * opacity, 0, (WheelPos * 3) * opacity);
  } 
  else {
    WheelPos -= 170;
    return strip.Color(0, (WheelPos * 3) * opacity, (255 - WheelPos * 3) * opacity);
  }
}

