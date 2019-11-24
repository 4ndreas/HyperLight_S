#include "Arduino.h"
#include "variant.h"
#include <SPI.h>
#include <Artnet.h>
#include "hyperlight.h"
#include "helper.h"

#define USE_WEBSERVER
#define USE_OLED

#ifdef USE_OLED
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSansBold9pt7b.h>

#define OLED_RESET -1
Adafruit_SSD1306 display(OLED_RESET);

#endif


#ifdef USE_WEBSERVER
#include "website.h"

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):

EthernetServer server(80);

//#define REQ_BUF_SZ   60
//char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
//char req_index = 0;              // index into HTTP_req buffer

#endif


// W5500 Ethernet
// HW-SPI-3 or HW_SPI-1
#define ETH_MOSI_PIN  PB5
#define ETH_MISO_PIN  PB4
#define ETH_SCK_PIN   PB3
#define ETH_SCS_PIN   PA15
#define ETH_INT_PIN   PB8
#define ETH_RST_PIN   PD3

Artnet artnet;
hyperlight leds;

byte ip[] = {192, 168, 2, 80};
byte mac[] = {0x04, 0xE9, 0xE5, 0x80, 0x69, 0xEC};




IPAddress remoteArd;

void setup() {

  leds.begin();

  leds.setOffset(0,1);
  leds.setOffset(1,1);
  leds.setOffset(2,1);
  leds.setOffset(3,2);
  leds.setOffset(4,1);
  leds.setOffset(5,1);
  leds.setOffset(6,1);
  leds.setOffset(7,1);



  pinMode(ETH_RST_PIN, OUTPUT);
  digitalWrite(ETH_RST_PIN, HIGH);

  delay(100);

  SPI.setMISO(ETH_MISO_PIN);
  SPI.setMOSI(ETH_MOSI_PIN);
  SPI.setSCLK(ETH_SCK_PIN);
  Ethernet.init(ETH_SCS_PIN);

  artnet.begin(mac, ip);
  artnet.setArtDmxCallback(onDmxFrame);

#ifdef USE_WEBSERVER
  server.begin();
#endif

#ifdef USE_OLED
  Wire.setSDA(PB7);
  Wire.setSCL(PB6);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  display.clearDisplay();
  updateDisplay();
#endif
}

uint32_t last_update = 0;
uint32_t currentTime = 0;

void loop() {
	currentTime = millis();

	artnet.read();


	if (currentTime - last_update > 33)	// 50 fps
	{
		last_update = currentTime;

		// set status led
		for (int strip = 0; strip < 16; strip++) {
			int lastUpdate = currentTime - leds.getUpdateTime(strip);

			if (lastUpdate < 1000)
			{
				leds.setOffsetColor(strip,0,128,0);
			}
			else
			{
				leds.setOffsetColor(strip,0,128,0);
			}
		}
		//update leds leds
		leds.show();
	}

#ifdef USE_WEBSERVER
	webserverLoop();
#endif
#ifdef USE_OLED
    updateDisplay();
#endif
}


//EthernetClient client;
#ifdef USE_WEBSERVER
void webserverLoop(void)
{
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
	// an http request ends with a blank line
	boolean currentLineIsBlank = true;
	while (client.connected()) {
	  if (client.available()) {
		char c = client.read();

		if (c == '\n' && currentLineIsBlank) {

//			 send a standard http response header
		  client.println("HTTP/1.1 200 OK");
		  client.println("Content-Type: text/html");
		  client.println("Connection: close");  // the connection will be closed after completion of the response
		  client.println("Refresh: 10");  // refresh the page automatically every 5 sec
		  client.println();
		  client.println("<!DOCTYPE HTML>");
		  client.println("<html>");

		  // output the value of each analog input pin
		  client.print("<h1>HyperLight Info </h1>");

		  client.print("My IP: ");
		  client.print(ip[0], DEC);client.print(".");
		  client.print(ip[1], DEC);client.print(".");
		  client.print(ip[2], DEC);client.print(".");
		  client.print(ip[3], DEC);

		  client.println("<br />");
		  client.print("Host: ");
		  client.print(remoteArd[0]);client.print(".");
		  client.print(remoteArd[1]);client.print(".");
		  client.print(remoteArd[2]);client.print(".");
		  client.print(remoteArd[3]);
		  client.println("<br />");
		  client.println("<br />");
		  client.println("<table border=\"1\">");

		  for (int strip = 0; strip < 16; strip++)
		  {
			client.println("<tr>");
			int lastUpdate = currentTime - leds.getUpdateTime(strip);
			client.println("<td>");
			client.print("Universe ");
			client.print(strip);
			client.println("</td>");
			client.println("<td ALIGN=\"CENTER\">");
			if (lastUpdate < 1000)
			{
				client.print("<font color=\"green\"> OK </font>");
			}
			else
			{
				client.print("<font color=\"red\"> error </font>");
			}
			client.println("</td>");
			client.println("<td>");

			client.print("updated ");
			client.print(lastUpdate);
			client.print(" ms ago ");

			client.println("</li>");
			client.println("</td>");
			client.println("</tr>");
		  }


		  client.println("</table>");
		  client.println("</html>");

//            req_index = 0;
//            StrClear(HTTP_req, REQ_BUF_SZ);
            break;
		}

		if (c == '\n') {
		  // you're starting a new line
		  currentLineIsBlank = true;
		} else if (c != '\r') {
		  // you've gotten a character on the current line
		  currentLineIsBlank = false;
		}
	  }
	}
	// give the web browser time to receive the data
	// close the connection:
	client.stop();
  }
}


#endif

#ifdef USE_OLED


void updateDisplay()
{
  static uint32_t lastDisplayUpdate = 0;
  static uint32_t lastDisplayStatus = 0;
  uint32_t displayStatus = 0;

  if(( currentTime - lastDisplayUpdate) > 1000){
	  lastDisplayUpdate = currentTime;

	  display.clearDisplay();

	  display.setCursor(0,6);
	  display.setTextColor(WHITE);
	  display.setFont(&FreeSansBold9pt7b);

	  display.print(ip[0], DEC);display.print(".");
	  display.print(ip[1], DEC);display.print(".");
	  display.print(ip[2], DEC);display.print(".");
	  display.print(ip[3], DEC);display.println("");

	  display.setFont();

	  display.setCursor(0,32);
	  display.println("|-E1-|-E2-|-E3-|-E4-|");
	  display.setCursor(0,48);

	  for (int strip = 0; strip < 16; strip++) {
		if((strip%4) == 0)
		{
			display.print("|");
		}
		int lastUpdate = currentTime - leds.getUpdateTime(strip);
		if (lastUpdate < 1000)
		{
			display.print("O");
			displayStatus |= (1<<strip);
		}
		else
		{
			display.print("X");
		}
	  }
	  display.print("|");

	  display.setCursor(0,16);
	  display.print("Host:");

	  if(displayStatus == 0)
	  {
		  display.print("*.*.*.*");
	  }
	  else
	  {
		  display.print(remoteArd[0]);display.print(".");
		  display.print(remoteArd[1]);display.print(".");
		  display.print(remoteArd[2]);display.print(".");
		  display.print(remoteArd[3]);
	  }

	  if((displayStatus != lastDisplayStatus) || (displayStatus == 0))
	  {
		  // only update if something changed
		  // update causes a flicker
		  lastDisplayStatus = displayStatus;
		  display.display();
	  }
  }
}
#endif


void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{
  remoteArd = artnet.remote;
  // 8x - One ArtNet universe per Output (Maiskolben)
  if(universe < 8)
  {
	  leds.setStripLED(universe, data, length, 1);
  }
  // Ping Pong Panel 1 16x16 RGB leds
  else if(universe == 8)
  {
	  leds.setSnakeLED(8, data, length, 0, 16);	// 170 leds
  }
  else if(universe == 9)
  {
	  leds.setSnakeLED(8, data, length, 170, 16);	// 86 leds
  }
  // Ping Pong Panel 2 16x16 RGB leds
  else if(universe == 10)
  {
	  leds.setSnakeLED(9, data, length, 0, 16);	// 170 leds
  }
  else if(universe == 11)
  {
	  leds.setSnakeLED(9, data, length, 170, 16);	// 86 leds
  }
  // RGBW panel 17x17 RGBW leds
  else if(universe == 12)
  {
	  for(int i = 0; i < length/3; i++)
	  {
		  leds.setLED(10, i, data[i*3], data[i*3+1], data[i*3+2] , 0);
	  }
  }
  else if(universe == 13)
  {
	  for(int i = 0; i < length/3; i++)
	  {
		  leds.setLED(10, 170 + i, data[i*3], data[i*3+1], data[i*3+2] , 0);
	  }
  }
  // Rotating Wheel
  else if(universe == 14)
  {
	  leds.setStripLED(11, data, length, 1,RBG);
  }
  else if(universe == 15)
  {
	  leds.setStripLED(11, data, length, 170,RBG);
  }

  else if(universe == 256+0)
  {
	  leds.setStripLED(12, data, length, 1,RBG);
  }
  else if(universe == 256+1)
  {
	  leds.setStripLED(13, data, length, 1,RBG);
  }
  else if(universe == 256+2)
  {
	  leds.setStripLED(14, data, length, 1,RBG);
  }
  else if(universe == 256+3)
  {
	  leds.setStripLED(15, data, length, 1,RBG);
  }
}





