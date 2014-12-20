// Presence's attempt at an Arduino Ethernet + Adafruit RGB LCD Shield + CheerLights
// https://github.com/paulgorman/cheerlights
// Most notable challenge was that I use the String object to pull apart content, and I kept running out of RAM after a few loops.
// Also using HTTP/1.1 to protect against VirtualHost same-IP webhosts
// 20141219 Copied to Codebender.cc for convenience in hacking in updates

#include <SPI.h>
#include <Ethernet.h>
#include <WString.h>

// byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x2D, 0x54 };  // pres arduino
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x67, 0x74 };  // pres ethernet
EthernetClient client; // Creates a client which can connect to a specified internet IP address and port defined in client.connect() 

// include the AdaFruit LCD Shield library code:
#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// These #defines make it easy to set the backlight/Cheerlight color
#define OFF 0x0
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define PURP 0x5
#define WHITE 0x7

// ThingSpeak Settings
const int thingSpeakInterval = 24 * 1000; // Time interval in milliseconds to get data from ThingSpeak (number of seconds * 1000 = interval)
long lastConnectionTime = 0;  // For ThingSpeak refresh
int failedCounter = 0;  // How many web client attempts before we freak out

const char headerBreak[] = { 13, 10 };  // what separates http header from body, \r\n

void setup() {
  lcd.begin(16, 2);
  showStatus("Presence Cheer",WHITE);
  startEthernet();  // this really should only happen once.  If the web breaks, then reset the arduino manually.
  lastConnectionTime = millis();
}

void loop() {
  if(client.available() > 0) {
    // There's data waiting to be processed!
    lcd.setCursor(14,0);
    lcd.print("R");  // Receiving data
    String response = String("");  // seriously, the most amazing RAM leak
    char charIn;  // incoming byte from the http result
    char prevChar[4];  // buffer of characters to compare with the headerbreak characters
    boolean gogo = false;  // when its okay to start reading into the result String object
    delay(100);  // allow time for the etherweb module receive all the content
    while(client.available() > 0) {
      charIn = client.read(); // read a char from the ethernet buffer
      if (gogo) {
        if (response.length() < 50) { // Limit the response String object to only 50 characters.
          response.concat(charIn); // append that char to the string response
          // lcd.setCursor(15,0);
          // lcd.print("B");  // Body
        }
      } else {
        // This is a bunch of http header uselessness
        // lcd.setCursor(15,0);
        // lcd.print("H");  // Header
        prevChar[3] = prevChar[2];
        prevChar[2] = prevChar[1];
        prevChar[1] = prevChar[0];
        prevChar[0] = charIn;
        if ((prevChar[3] == headerBreak[0]) && (prevChar[2] == headerBreak[1]) && (prevChar[1] == headerBreak[0]) && (prevChar[0] == headerBreak[1])) {
          // I just saw the \r\n\r\n break between http header and content!
          gogo = true;
        }
      }
    }
    if (response.length() < 49) {  // if the response buffer is huge, then it was an error page.
      processLightCommand(response);  // Send the http response body content over to parse for a color
      lcd.setCursor(15,1);
      lcd.print("  ");  // Wipe out any extra notice flags
    } else {
      lcd.setCursor(15,1);
      lcd.print("X");  // This CheerLights query attempt 503'd on us, probably.
    } 
    // now disconnect from the webserver, if they haven't already disconnected us.
    if (client.connected()) {
      lcd.setCursor(14,0);
      lcd.print("d");  // they disconnected us after our request completed
      client.flush();  // ignore any remaining content I missed earlier
      client.stop();  // disconnect from the webserver
    } else {
      // webserver didn't disconnect us, probably using KeepAlive
      lcd.setCursor(14,0);
      lcd.print("D");  // we forcibly Disconnected on them
      client.flush();  // ignore any remaining content may have missed earlier
      client.stop();  // disconnect from the web server, redundantly
    }
  }
  // is it time to update color status?
  if(millis() - lastConnectionTime > thingSpeakInterval) {
    if (!client.connected()) {
      // we're not connected to the webserver, its time to connect and get the current CheerLights Color!
      connectThingSpeak();
    } else {
      // we're connected to something, uhm, how?
      lcd.setCursor(15,0);
      lcd.print("!");
      delay(500);
    }
  } else {
    // chill here for a bit, the mainstay of our Arduino Cheerlights lifetime
    uptime();  // update the LCD to show we're not frozen
    delay(500);
    lcd.setCursor(14,0);
    lcd.print("  ");  // happy that it worked
  }
  // Check if Arduino Ethernet needs to be restarted
  if (failedCounter > 3 ) {
    stopEthernet();  // Does this even work?  Really?
    delay(500);
    startEthernet();
    delay(500);
    failedCounter = 0;
  }
} // End loop

void connectThingSpeak() {
  if (client.connect("api.thingspeak.com", 80)) {
    lcd.setCursor(14,0);
    lcd.print("C ");
    failedCounter = 0;
    client.println("GET /channels/1417/field/1/last.txt HTTP/1.1");
    client.println("Host: api.thingspeak.com");
    client.println("Accept: text/html, plain/text");
    client.println("User-Agent: Presence's Arduino Cheerlights (presence@irev.net)");
    client.println("Connection: close"); // make web server disconnect me
    client.println();
  } else {
    failedCounter++;
    lcd.setCursor(14,0);
    lcd.print("F"+String(failedCounter, DEC));
  }
  lastConnectionTime = millis();
}

void processLightCommand(String &response) {
  if (response.indexOf("white") > 0) {
    showStatus("WHITE",WHITE);
  } else if (response.indexOf("black") > 0) {
    showStatus("BLACK",OFF);
  } else if (response.indexOf("red") > 0) {
    showStatus("RED",RED);
  } else if (response.indexOf("green") > 0) {
    showStatus("GREEN",GREEN);
  } else if (response.indexOf("blue") > 0) {
    showStatus("BLUE",BLUE);
  } else if (response.indexOf("cyan") > 0) {
    showStatus("CYAN",TEAL);
  } else if (response.indexOf("magenta") > 0) {
    showStatus("MAGENTA",PURP);
  } else if (response.indexOf("yellow") > 0) {
    showStatus("YELLOW",YELLOW);
  } else if (response.indexOf("purple") > 0) {
    showStatus("PURPLE",PURP);
  } else if (response.indexOf("orange") > 0) {
    showStatus("ORANGE",YELLOW);
  } else if (response.indexOf("warmwhite") > 0) {
    showStatus("WARM WHITE",WHITE);
  } else if (response.indexOf("oldlace") > 0) {
  	showStatus("OLD LACE",WHITE);
  } else if (response.indexOf("pink") > 0) {
  	showStatus("PINK",RED);
  } else {
    showStatus("(no match)" + response,BLUE);
  }
}

void showStatus(String msg, int bgColor) {
  uptime();
  lcd.setBacklight(bgColor);
  lcd.setCursor(0,1);
  for (int i = 0; i < 17; i++) {
    lcd.print(" ");
  }
  lcd.setCursor(0,1);
  lcd.print(msg.substring(0,16));
}

void stopEthernet() {
  client.stop();
  lcd.setCursor(14,0);
  lcd.print("S");
  delay(100);
}

void startEthernet() {
  lcd.setCursor(0,1);
  lcd.print("Initializing... ");
  if(Ethernet.begin(mac) == 0) {
    do {
      showStatus("DHCP Failed- Waiting",RED);
      delay(1000);
      showStatus("DHCP Failed- Retrying",RED);
    } while(Ethernet.maintain() & 0x0101);
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);
  lcd.setCursor(0,1);
  String tempbuff = String("");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    tempbuff.concat(String(Ethernet.localIP()[thisByte], DEC));
    tempbuff.concat(".");
  }
  tempbuff.concat("    ");
  lcd.setCursor(0,1);
  lcd.print(tempbuff.substring(0,16));
}

void uptime() {
  long days=0;
  long hours=0;
  long mins=0;
  long secs=0;
  secs = millis()/1000; //convect milliseconds to seconds
  mins=secs/60; //convert seconds to minutes
  hours=mins/60; //convert minutes to hours
  days=hours/24; //convert hours to days
  secs=secs-(mins*60); //subtract the coverted seconds to minutes in order to display 59 secs max
  mins=mins-(hours*60); //subtract the coverted minutes to hours in order to display 59 minutes max
  hours=hours-(days*24); //subtract the coverted hours to days in order to display 23 hours max
  lcd.setCursor(0,0);
  if (days>0) {
    lcd.print(String(days));
    lcd.print("d,");
  }
  lcd.print(hours);
  lcd.print(":");
  if (mins < 10) {
    lcd.print("0");
  }
  lcd.print(mins);
  lcd.print(":");
  if (secs < 10) {
    lcd.print("0");
  }
  lcd.print(secs);
  lcd.setCursor(13,1);
  lcd.print(freeRam());  // fascinated by how much free RAM there may be left on the Arduino
}

int freeRam () {
  // thanks to: http://www.controllerprojects.com/2011/05/23/determining-sram-usage-on-arduino/
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}


