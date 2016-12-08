/*
| This is GPSBuddy Ver 0.2.0 by ProckNation Labs 2016
| Coded by Jason Procknow.
| Bits of this code has been sourced from TinyGPS++, Adafruit, and other
| sources as detailed in the code comments.  I will try to keep better comments
| as I refine the code.
|
*/

/*
-----------------------------------------------------------------------------
Version 0.2.0 of GPSBuddy adds two buttons to the hardware and the code to
support the actions of those two buttons.  The actions will include changing
the brightness of the 7 segment LED display, change display mode such as
speedometer, date, time, heading (letters and degrees), and more as I see
needed.  Buttons will act as mode and select.  More details will be in the wiki
at https://www.procknation.com/wiki/index.php?title=GPSBuddy.
-----------------------------------------------------------------------------
*/

#include <TinyGPS++.h>
//#include <SoftwareSerial.h>
#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
#include "MTK3339_SIM808_Commands.h"
#include <Bounce2.h>

Adafruit_7segment matrix = Adafruit_7segment();
/*
   This sample sketch demonstrates the normal use of a TinyGPS++ (TinyGPSPlus) object.
   It requires the use of SoftwareSerial, and assumes that you have a
   4800-baud serial GPS device hooked up on pins 4(rx) and 3(tx).
*/
//static const int RXPin = 4, TXPin = 3;
uint32_t GPSBaud = 9600; //This is the default gps speed
//static const uint32_t GPSBaud = 115200; //This is the fast custom speed.
//static const uint32_t GPSBaud = 57600; //This is the kinda fast custom speed.

#define BUTTON_PIN_1 48
#define BUTTON_PIN_2 50

// Instantiate a Bounce object
Bounce debouncer1 = Bounce();

// Instantiate another Bounce object
Bounce debouncer2 = Bounce();

int currentBrightness = 15;
// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
//SoftwareSerial ss(RXPin, TXPin);
HardwareSerial GPSSerial = Serial1;

/*
For antenna status:
Response packet:
$PGTOP,11,value*checksum
Value:
 1=>Active Antenna Shorted
 2=>Using Internal Antenna
 3=>Using Active Antenna
 */


//TinyGPSCustom pmtk(gps, "PMTK", 1); // $PGTOP sentence, 2nd element
//TinyGPSCustom magneticVariation(gps, "GPRMC", 1);

char fl[10];        // long enough to hold complete floating string
float x;
float speedMPH;

void setup()
{
	Serial.begin(115200);
	GPSSerial.begin(GPSBaud);
	sendCommand(PMTK_SET_BAUD_57600);
	delay(500);
	GPSSerial.end();
	delay(500);
	GPSSerial.begin(57600);
	delay(300);
	sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
	delay(30);
	sendCommand(PMTK_SET_NMEA_UPDATE_5HZ);
	delay(30);
	sendCommand(PMTK_API_SET_FIX_CTL_5HZ);
	delay(30);
	sendCommand(PMTK_SET_NAV_THRESHOLD_04_MPS);
	delay(30);

	// Setup the first button with an internal pull-up :
	pinMode(BUTTON_PIN_1, INPUT_PULLUP);
	// After setting up the button, setup the Bounce instance :
	debouncer1.attach(BUTTON_PIN_1);
	debouncer1.interval(5); // interval in ms

	// Setup the second button with an internal pull-up :
	pinMode(BUTTON_PIN_2, INPUT_PULLUP);
	// After setting up the button, setup the Bounce instance :
	debouncer2.attach(BUTTON_PIN_2);
	debouncer2.interval(5); // interval in ms



	matrix.begin(0x70);
	matrix.print(0);
	matrix.writeDisplay();
	//sendCommand("$PMTK000*32");
	//sendCommand("$PMTK251,9600*17");
	//sendCommand("$PMTK251,57600*2C");
	//sendCommand(PMTK_API_SET_FIX_CTL_5HZ);
}

void loop()
{
	// This sketch displays information every time a
	// new sentence is correctly encoded.
	while (GPSSerial.available() > 0)
		if (gps.encode(GPSSerial.read()))
			displayInfo();

// Update the Bounce instances :
	debouncer1.update();
	debouncer2.update();

	// Get the updated value :
	int value1 = debouncer1.fell();
	int value2 = debouncer2.fell();

	// Turn on the LED if either button is pressed :
	if ( value2 == HIGH )
	{
		Serial.println("Increasing display brightness");
		if (currentBrightness < 15 )
		{
			matrix.setBrightness(++currentBrightness);
			matrix.writeDisplay();
		}
		else
		{
			currentBrightness = 0;
			matrix.setBrightness(currentBrightness);
			matrix.writeDisplay();
		}
		
	}



	if (millis() > 5000 && gps.charsProcessed() < 10)
	{
		Serial.println(F("No GPS detected: check wiring."));
		matrix.print(1010);
		matrix.writeDisplay();
		while(true);
	}
}

void displayInfo()
{
	if (gps.speed.isValid())
	{
		x = gps.speed.mph();
		dtostrf(x, 6, 3, fl);  // -n.nnn Use this for a consistent float format
		//Serial.println(fl);
		speedMPH = gps.speed.mph();
		Serial.print(speedMPH);
		matrix.print(speedMPH);
		matrix.writeDisplay();
	}
	else
	{
		Serial.print(F("INVALID"));
		matrix.print(1010);
		matrix.writeDisplay();
	}

	Serial.print(F("Location: "));
	if (gps.location.isValid())
	{
		Serial.print(gps.location.lat(), 6);
		Serial.print(F(","));
		Serial.print(gps.location.lng(), 6);
	}
	else
	{
		Serial.print(F("INVALID"));
		matrix.print(1010);
		matrix.writeDisplay();
	}

	Serial.print(F("  Date/Time: "));
	if (gps.date.isValid())
	{
		Serial.print(gps.date.month());
		Serial.print(F("/"));
		Serial.print(gps.date.day());
		Serial.print(F("/"));
		Serial.print(gps.date.year());
	}
	else
	{
		Serial.print(F("INVALID"));
		matrix.print(1010);
		matrix.writeDisplay();
	}

	Serial.print(F(" "));
	if (gps.time.isValid())
	{
		if (gps.time.hour() < 10) Serial.print(F("0"));
		Serial.print(gps.time.hour());
		Serial.print(F(":"));
		if (gps.time.minute() < 10) Serial.print(F("0"));
		Serial.print(gps.time.minute());
		Serial.print(F(":"));
		if (gps.time.second() < 10) Serial.print(F("0"));
		Serial.print(gps.time.second());
		Serial.print(F("."));
		if (gps.time.centisecond() < 10) Serial.print(F("0"));
		Serial.print(gps.time.centisecond());
	}
	else
	{
		Serial.print(F("INVALID"));
		matrix.print(1010);
		matrix.writeDisplay();
	}
	//sendCommand("$PMTK000*32");
	//Serial.print("Antenna Status: ");


	//Serial.print(pmtk.value());
	/*switch (pgtop.value())
	{
		case 1:
			//1=>Active Antenna Shorted
			Serial.print("Fault. Antenna Shorted");
			break;
		case 2:
			//2=>Using Internal Antenna
			Serial.print("Using Internal Antenna");
			break;
		case 3:
			//3=>Using Active Antenna
			Serial.print("Using Active External Antenna");
		default:
			// default action when no case match
			Serial.print("Unable to get antenna status.");
			break;
	}*/
	Serial.println();

}

void sendCommand(const char *str)
{
	GPSSerial.println(str);
}
