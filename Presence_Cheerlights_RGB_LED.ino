// Presence's attempt at an Arduino Ethernet + RGB LED + CheerLights
// 20121218 presence@irev.net
// Most notable challenge was that I use the String object to pull apart content, and I kept running out of RAM after a few loops.
// Also using HTTP/1.1 to protect against VirtualHost same-IP webhosts

#include <SPI.h>
#include <Ethernet.h>
#include <WString.h>

// byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x2D, 0x54 };  // pres arduino
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x67, 0x74 };  // pres ethernet
EthernetClient client; // Creates a client which can connect to a specified internet IP address and port defined in client.connect() 

// ThingSpeak Settings
char thingSpeakAddress[] = "api.thingspeak.com";
const int thingSpeakInterval = 11 * 1000; // Time interval in milliseconds to get data from ThingSpeak (number of seconds * 1000 = interval)
long lastConnectionTime = 0;  // For ThingSpeak refresh
int failedCounter = 0;  // How many web client attempts before we freak out

// common anode RGB LED
int redPin = 6;
int greenPin = 5;
int bluePin = 3;

void setup() {
  startEthernet();  // this really should only happen once.  If the web breaks, then reset the arduino manually.
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  if(client.available() > 0) {
    // There's data waiting to be processed!
    String response = String("");  // seriously, the most amazing RAM leak
    char charIn;  // incoming byte from the http result
    char headerBreak[] = { 13, 10 };  // what separates http header from body, \r\n
    char prevChar[4];  // buffer of characters to compare with the headerbreak characters
    boolean gogo = false;  // when its okay to start reading into the result String object
    delay(100);  // allow time for the etherweb module receive all the content
    while(client.available() > 0) {
      charIn = client.read(); // read a char from the ethernet buffer
      if (gogo) {
        if (sizeof(response) < 50) { // Limit the response String object to only 50 characters.
          response.concat(charIn); // append that char to the string response
        }
      } else {
        // This is a bunch of http header uselessness
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
    if (sizeof(response) < 49) {  // if the response buffer is huge, then it was an error page.
      processLightCommand(response);  // Send the http response body content over to parse for a color
      Serial.println(response);
    }
    // now disconnect from the webserver, if they haven't already disconnected us.
    if (client.connected()) {
      client.flush();  // ignore any remaining content I missed earlier
      client.stop();  // disconnect from the webserver
    } else {
      // webserver didn't disconnect us, probably using KeepAlive
      client.flush();  // ignore any remaining content may have missed earlier
      client.stop();  // disconnect from the web server, redundantly
    }
  }
  // is it time to update color status?
  if(millis() - lastConnectionTime > thingSpeakInterval) {
    if (!client.connected()) {
      // we're not connected to the webserver, its time to connect and get the current CheerLights Color!
      connectThingSpeak();
    }
  } else {
    // chill here for a bit, the mainstay of our Arduino Cheerlights lifetime
    delay(1000);
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
  if (client.connect(thingSpeakAddress, 80)) {
    failedCounter = 0;
    client.println("GET /channels/1417/field/1/last.txt HTTP/1.1");
    client.println("Host: api.thingspeak.com");
    client.println("Accept: text/html, plain/text");
    client.println("User-Agent: Presence's Arduino Cheerlights (presence@irev.net)");
    client.println();
    lastConnectionTime = millis();
  } else {
    failedCounter++;
    lastConnectionTime = millis();
  }
}

void processLightCommand(String &response) {
  int Counter;
  if (response.indexOf("white") > 0) {
    analogWrite(redPin,0);
    analogWrite(greenPin,0);
    analogWrite(bluePin,0);
  } else if (response.indexOf("black") > 0) {
    analogWrite(redPin,255);
    analogWrite(greenPin,255);
    analogWrite(bluePin,255);
  } else if (response.indexOf("red") > 0) {
    analogWrite(redPin,0);
    analogWrite(greenPin,255);
    analogWrite(bluePin,255);
  } else if (response.indexOf("green") > 0) {
    analogWrite(redPin,255);
    analogWrite(greenPin,0);
    analogWrite(bluePin,255);
  } else if (response.indexOf("blue") > 0) {
    analogWrite(redPin,255);
    analogWrite(greenPin,255);
    analogWrite(bluePin,0);
  } else if (response.indexOf("cyan") > 0) {
    analogWrite(redPin,255);
    analogWrite(greenPin,0);
    analogWrite(bluePin,0);
  } else if (response.indexOf("magenta") > 0) {
    analogWrite(redPin,0);
    analogWrite(greenPin,255);
    analogWrite(bluePin,0);
  } else if (response.indexOf("yellow") > 0) {
    analogWrite(redPin,0);
    analogWrite(greenPin,0);
    analogWrite(bluePin,255);
  } else if (response.indexOf("purple") > 0) {
    analogWrite(redPin,0);
    analogWrite(greenPin,255);
    analogWrite(bluePin,0);
  } else if (response.indexOf("orange") > 0) {
    analogWrite(redPin,0);
    analogWrite(greenPin,96);
    analogWrite(bluePin,255);
  } else if (response.indexOf("warmwhite") > 0) {
    analogWrite(redPin,64);
    analogWrite(greenPin,192);
    analogWrite(bluePin,192);
  }
}

void stopEthernet() {
  client.stop();
  delay(100);
}

void startEthernet() {
  Ethernet.begin(mac);
  // give the Ethernet shield a second to initialize:
  delay(1000);
}
