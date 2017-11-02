//
// housenode
//
// machine for living in
//
// dht11 test
//
// Robert Twomey 2016
// rtwomey@uw.edu
//

#include "DHT.h"
#include "application.h"

#include "OSC/OSCMessage.h"
#include "OSC/OSCBundle.h"

#define READ_EEPROM

//----- OUTPUTS
int ledPin = D7;

//----- OSC COMMANDS
char OscCmd_led[5] = "/led"; // led indicator

char OscCmd_SetNodeID[11] = "/setnodeid"; // 10 characters + 1 null terminator
char OscCmd_SetServerIP[13] = "/setserverip"; // 12 characters + 1 null terminator
char OscCmd_SetNotifyState[11] = "/setnotify";
char OscCmd_SetThreshold[14] = "/setthreshold";
char OscCmd_SetHumidity[13] = "/sethumidity";
char OscCmd_SetTemperature[16] = "/settemperature";

char OscCmd_SetAnalogTriggerMode[12] = "/settrigger";

char OscCmd_SetDPin[9] = "/setdpin";
char OscCmd_ReadDPin[10] = "/readdpin";
char OscCmd_ReadAPin[10] = "/readapin";

char OscCmd_UpdateEEPROM[13] = "/writeeeprom";
char OscCmd_ReadEEPROM[12] = "/readeeprom";

char OscCmd_GetServerIP[13] = "/getserverip";
char OscCmd_GetNotifyState[11] = "/getnotify";
char OscCmd_GetNodeID[11] = "/getnodeid";
char OscCmd_GetNodeInfo[13] = "/getnodeinfo";
char OscCmd_GetAnalogThreshold[14] = "/getthreshold";
char OscCmd_GetHumidity[13] = "/gethumidity";
char OscCmd_GetTemperature[9] = "/gettemp";
char OscCmd_GetDHT[8] = "/getdht";
char OscCmd_GetTriggerMode[12] = "/gettrigger";

char OscAddr_AnalogPin[11] = "/analogpin";
char OscAddr_DigitalPin[12] = "/digitalpin";
const char OscAddr_Temperature[13] = "/temperature";
const char OscAddr_Humidity[12] = "/humidity";
const char OscAddr_Debug[7] = "/debug";

char OscAddr_ParticleIP[12] = "/particleip";
char OscAddr_NodeID[8] = "/nodeid";
char OscAddr_NodeInfo[10] = "/nodeinfo";
char OscAddr_Announce[10] = "/announce";

// ---- serverPort SERVER
IPAddress serverAddress = IPAddress(192, 168, 1, 100);	// IP address of blackbox at home
const int serverPort = 9999;		// to send data to the computer (from here)

 // ---- THIS DEVICE
IPAddress coreAddress;
const int corePort = 8888;		// to send data to the Spark Core (from the computer)

// OSC ADDRESSES
#define NODE_ID_STR "photon"
#define NODE_ID_LENGTH 20
char nodeid[NODE_ID_LENGTH] = NODE_ID_STR;

UDP Udp;

//----- PIN REPORTING BEHAVIOR

#define NUM_DIGITAL_PINS 7

int digitalInPins[NUM_DIGITAL_PINS] = { D0, D1, D2, D3, D4, D5, D6 };

int digitalPinValues[NUM_DIGITAL_PINS] = { HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH };

int lastDigitalPinValues[NUM_DIGITAL_PINS] = { HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH };


#define NUM_ANALOG_PINS 6

int analogInPins[NUM_ANALOG_PINS] = { A0, A1, A2, A3, A4, A5 };

int analogPinValues[NUM_ANALOG_PINS] = { 0, 0, 0, 0, 0, 0 };

int lastAnalogPinValues[NUM_ANALOG_PINS] = { 0, 0, 0, 0, 0, 0 };

int analogThreshold[NUM_ANALOG_PINS] = { 0, 0, 0, 0, 0, 0 };

bool bAnalogTriggerMode[NUM_ANALOG_PINS] = {false, false, false, false, false, false };

bool bAnalogTriggered[NUM_ANALOG_PINS] = { false, false, false, false, false, false };


// notification
#define NUM_PINS_TOTAL ( NUM_DIGITAL_PINS + NUM_ANALOG_PINS + 3 )
bool bNotifyPinChange[NUM_PINS_TOTAL] = {
	true, false, false, false, false, false, false, // D0-D6
	false, false, false, //
	false, false, false, false, false, false // A0 - A5
};

#define indicateInterval 500
unsigned long lastChanged = 0;
bool bLightOn = false;


// humidity / temperature

#define DHTPIN 0
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

long lastSend = 0;
#define SEND_INTERVAL 2000

float humidityThreshold = 40.0;
float temperatureThreshold = 75.0;

// More on modes here: https://docs.particle.io/reference/firmware/photon/#system-modes
SYSTEM_MODE(MANUAL); // do not connect to the cloud (Wi-Fi)


// ============================================================ //
// ==== send data to server via OSC
// ============================================================ //

void sendPinOSC(const char *addr, int thisPin, int thisVal)
{
	OSCMessage pinMsg(addr);

	pinMsg.add(thisPin);
	pinMsg.add(thisVal);
	pinMsg.add(nodeid);

	Udp.beginPacket(serverAddress, serverPort);
	pinMsg.send(Udp); // send the bytes
	Udp.endPacket();
	pinMsg.empty(); // empty the message to free room for a new one
}

void sendDHT(const char *addr, float h, float f) {
	OSCMessage msg(addr);

	msg.add(h);
	msg.add(f);
	msg.add(nodeid);

	Udp.beginPacket(serverAddress, serverPort);
	msg.send(Udp); // send the bytes
	Udp.endPacket();
	msg.empty(); // empty the message to free room for a new one
}

void sendHumidity(const char *addr, float h) {
	OSCMessage msg(addr);

	msg.add(h);
	msg.add(nodeid);

	Udp.beginPacket(serverAddress, serverPort);
	msg.send(Udp); // send the bytes
	Udp.endPacket();
	msg.empty(); // empty the message to free room for a new one
}

void sendTemperature(const char *addr, float t) {
	OSCMessage msg(addr);

	msg.add(t);
	msg.add(nodeid);

	Udp.beginPacket(serverAddress, serverPort);
	msg.send(Udp); // send the bytes
	Udp.endPacket();
	msg.empty(); // empty the message to free room for a new one
}

void sendHTL(const char *addr, float h, float f, int l) {
	OSCMessage msg(addr);

	msg.add(h);
	msg.add(f);
	msg.add(l);
	msg.add(nodeid);

	Udp.beginPacket(serverAddress, serverPort);
	msg.send(Udp); // send the bytes
	Udp.endPacket();
	msg.empty(); // empty the message to free room for a new one
}

void sendNodeID() {

	// notify server of new id
	OSCMessage notifyMsg("/nodeid");

	notifyMsg.add(nodeid);

	Udp.beginPacket(serverAddress, serverPort);
	notifyMsg.send(Udp); // send the bytes
	Udp.endPacket();
	notifyMsg.empty();
}

void sendNodeID( OSCMessage &mess) {
	sendNodeID();
}

void sendParticleIP(IPAddress destAddr) {
	OSCMessage coreIPMessage("/particleip");

	coreIPMessage.add(coreAddress[0]).add(coreAddress[1]).add(coreAddress[2]).add(coreAddress[3]);
	coreIPMessage.add(nodeid);

	Udp.beginPacket(destAddr, serverPort);
	coreIPMessage.send(Udp);
	Udp.endPacket();
	coreIPMessage.empty();

}

void sendParticleIP() {
	sendParticleIP(serverAddress);
}

void sendServerIP() {
	OSCMessage coreIPMessage("/debug");

	coreIPMessage.add(serverAddress[0]).add(serverAddress[1]).add(serverAddress[2]).add(serverAddress[3]);
	coreIPMessage.add(nodeid);

	Udp.beginPacket(serverAddress, serverPort);
	coreIPMessage.send(Udp);
	Udp.endPacket();
	coreIPMessage.empty();
}

void sendServerIP(OSCMessage &mess) {
	sendServerIP();
}


void sendNotifyState() {
	OSCMessage coreIPMessage("/debug");

	for(int i=0; i < NUM_PINS_TOTAL; i++)
		coreIPMessage.add(bNotifyPinChange[i]);

	coreIPMessage.add(nodeid);

	Udp.beginPacket(serverAddress, serverPort);
	coreIPMessage.send(Udp);
	Udp.endPacket();
	coreIPMessage.empty();
}

void sendNotifyState(OSCMessage &mess) {
	sendNotifyState();
}


void sendAnalogThreshold() {
	OSCMessage coreIPMessage("/debug");

	for(int i=0; i < NUM_ANALOG_PINS; i++)
		coreIPMessage.add(analogThreshold[i]);

	coreIPMessage.add(nodeid);

	Udp.beginPacket(serverAddress, serverPort);
	coreIPMessage.send(Udp);
	Udp.endPacket();
	coreIPMessage.empty();
}

void sendAnalogThreshold(OSCMessage &mess) {
	sendAnalogThreshold();
}

// ============================================================ //
// ==== read digital / analog pins and send to server
// ============================================================ //

void readDigitalOSC(OSCMessage &mess)
{
	if (mess.size() == 1 && mess.isInt(0)) {
		OSCMessage pinMsg("/digitalpin");

		int thisPin = mess.getInt(0);
		int thisVal = digitalRead(thisPin);

		// sendDigitalOSC(thisPin, thisVal);
		sendPinOSC("/digitalpin", thisPin, thisVal);
	}
}

void readAnalogOSC(OSCMessage &mess)
{
	if (mess.size() == 1 && mess.isInt(0)) {
		// OSCMessage pinMsg("/analogpin");

		int thisPin = mess.getInt(0);
		int thisVal = analogRead(thisPin);

		sendPinOSC("/analogpin", thisPin, thisVal);
	}
}


// ============================================================ //
// ==== set notification behavior, threshold, etc for pins
// ============================================================ //

void setThreshold(OSCMessage &mess) {
	if (mess.size() == 2 && mess.isInt(0) && mess.isInt(1)) {
		int thisPin = mess.getInt(0);
		int thisThreshold = mess.getInt(1);
		analogThreshold[thisPin - A0] = thisThreshold;
	}
}

void setHumidityThreshold(OSCMessage &mess) {
	if (mess.size() == 1 && mess.isFloat(0)) {
		float thisThreshold = mess.getFloat(0);
		humidityThreshold = thisThreshold;
	}
}

void setTemperatureThreshold(OSCMessage &mess) {
	if (mess.size() == 1 && mess.isFloat(0)) {
		float thisThreshold = mess.getFloat(0);
		temperatureThreshold = thisThreshold;
	}
}

void setNotifyState(OSCMessage &mess) {
	if (mess.size() == 2 && mess.isInt(0) && mess.isInt(1)) {
		int thisPin = mess.getInt(0);
		bool thisNotifyState = (mess.getInt(1) == 1);
		bNotifyPinChange[thisPin] = thisNotifyState;
	}
}

void setNodeID(OSCMessage &mess) {
	if (mess.size() == 1 && mess.isString(0)) {
		// set nodeid to received string
		char thisID[NODE_ID_LENGTH];

		int bytes = mess.getString(0, thisID, NODE_ID_LENGTH);
		// nodeid = char[20];

		strcpy(nodeid, thisID);

		sendNodeID();
	}
}

void setServerIP(OSCMessage &mess) {
	if (mess.size() == 4 && mess.isInt(0) && mess.isInt(1)
		&& mess.isInt(2) && mess.isInt(3)) {

		uint8_t b0 = mess.getInt(0);
		uint8_t b1 = mess.getInt(1);
		uint8_t b2 = mess.getInt(2);
		uint8_t b3 = mess.getInt(3);


		// set udp to send to new address
		serverAddress = { b0, b1, b2, b3 };

		// coreAddress = WiFi.localIP();

		sendParticleIP();
	}
}

void setLEDStatus(OSCMessage &mess)
{
	if (mess.size() == 2 && mess.isInt(0) && mess.isInt(1)) {
		int thisLed = mess.getInt(0);
		int thisStatus = mess.getInt(1);

		// check pin mode
		pinMode(thisLed, OUTPUT);
		digitalWrite(thisLed, thisStatus);
	}
}

void setDigitalOut(OSCMessage &mess)
{
	if (mess.size() == 2 && mess.isInt(0) && mess.isInt(1)) {
		int thisPin = mess.getInt(0);
		int thisVal = mess.getInt(1);

		// check pin mode
		if(getPinMode(thisPin) != OUTPUT)
			pinMode(thisPin, OUTPUT);

		digitalWrite(thisPin, thisVal);
	}
}


void helloWink() {
	for (int i=0; i < 10; i ++) {
		digitalWrite(D7, LOW);
		delay(200);
		digitalWrite(D7, HIGH);
		delay(200);
	}
	digitalWrite(D7, LOW);
}

// void setServo(OSCMessage &mess) {
// 	if (mess.size() == 2 && mess.isInt(0) && mess.isInt(1)) {
// 		int thisPin = mess.getInt(0);
// 		int thisMs = mess.getInt(1);
// }

// ============================================================ //
// ==== read/write node-specific settings from EEPROM
// ============================================================ //

struct NodeSettings {
	IPAddress serverAddress;
	// int serverPort;
	char nodeid[NODE_ID_LENGTH];
	bool bNotifyPinChange[NUM_PINS_TOTAL];
	int analogThreshold[NUM_ANALOG_PINS];
};

void readEEPROMSettings() {

	/*
	emulated EEPROM storage
	addr: value

	0-3: server IP address
	4-23: nodeid as character array
	24-39: notifyPinChange flags
	40-45: analogThresholds
	*/

	int i = 0;

	// // server IP address
	// uint8_t b0 = EEPROM.read(0);
	// uint8_t b1 = EEPROM.read(1);
	// uint8_t b2 = EEPROM.read(2);
	// uint8_t b3 = EEPROM.read(3);

	// // server port
	// uint8_t p0 = EEPROM.read(4);
	// uint8_t p1 = EEPROM.read(5);

	// // serverAddress = { b0, b1, b2, b3 };
	// serverAddress = { b0, b1, b2, b3 };

	// // nodeid
	// for(i = 0; i < 20; i++)
	// 	nodeid[i] = EEPROM.read(i);

	NodeSettings newsettings;

	EEPROM.get(1, newsettings);

	// copy into variables
	strcpy(nodeid, newsettings.nodeid);
	serverAddress = newsettings.serverAddress;
	memcpy(bNotifyPinChange, newsettings.bNotifyPinChange, NUM_PINS_TOTAL * sizeof(bool));
	memcpy(analogThreshold, newsettings.analogThreshold, NUM_ANALOG_PINS * sizeof(int));

	// char newid[NODE_ID_LENGTH];

	// for(i=0; i < NODE_ID_LENGTH; i++)
	// 	newid[i] = (char)EEPROM.read(4+i);

	// strcpy(nodeid, newid);

	// nodeid = mynodeid;

	// // pin change flags
	// for(i = 0; i < 16; i++)
	// 	bNotifyPinChange[i] = bool(EEPROM.read(i + 24));

	// // analog thresholds
	// for(i = 0; i < 6; i++)
	// 	analogThreshold[i] = EEPROM.read(i + 40);
}

void readEEPROMSettings(OSCMessage &mess) {
	readEEPROMSettings();
}

void writeEEPROMSettings() {

	NodeSettings mysettings;
	mysettings.serverAddress = serverAddress;
	strcpy(mysettings.nodeid, nodeid);
	memcpy(mysettings.bNotifyPinChange, bNotifyPinChange, NUM_PINS_TOTAL * sizeof(bool));
	memcpy(mysettings.analogThreshold, analogThreshold, NUM_ANALOG_PINS * sizeof(int));

	// // server IP address
	// EEPROM.update(0, serverAddress[0]);
	// EEPROM.update(1, serverAddress[1]);
	// EEPROM.update(2, serverAddress[2]);
	// EEPROM.update(3, serverAddress[3]);

	// write nodeid
	// for(int i=0; i < NODE_ID_LENGTH; i++)
	// 	EEPROM.update(4+i, nodeid[i]);

	EEPROM.write(0, true); // store bool flag that we have initialized eeprom
	EEPROM.put(1, mysettings);
}

void writeEEPROMSettings(OSCMessage &mess) {
	writeEEPROMSettings();
}


// ============================================================ //
// ==== setup
// ============================================================ //

void setup()
{

	// set D0-D6 as input
	for(int i=1; i < 7; i++)
		pinMode(i, INPUT_PULLUP);

	// set D7 as LED output
	pinMode(D7, OUTPUT);

	// // wink to say hello at start
	// helloWink();

	// Connect to WiFi
	WiFi.on();

	#ifdef DXWAREHOUSE
		WiFi.setCredentials("DXARTS Ballard", "dxartsword");
	#else
		WiFi.setCredentials("housemachine", "2029973952");
	#endif

	WiFi.connect();

	waitUntil(WiFi.ready);
	// necessary to update localIP

	Particle.process();

	// Start UDP
	Udp.begin(corePort);

	// load settings
	#ifdef READ_EEPROM
	readEEPROMSettings();
	#endif

	// get the IP address of the device and send it as an OSC Message
	coreAddress = WiFi.localIP();

	// broadcast address on local subnet
	IPAddress broadcastAddr = IPAddress(coreAddress[0], coreAddress[1], coreAddress[2], 255);
	sendParticleIP(broadcastAddr); // announce IP address

	dht.begin();

}


void readDHT() {
	// Reading temperature or humidity takes about 250 milliseconds!
	// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
	float h = dht.readHumidity();
	// Read temperature as Celsius (the default)
	float t = dht.readTemperature();
	// Read temperature as Fahrenheit (isFahrenheit = true)
	float f = dht.readTemperature(true);

	int l = analogRead(A0);

	// Check if any reads failed and exit early (to try again).
	if (isnan(h) || isnan(t) || isnan(f)) {
		// sendHTL("/dht11", -1, -1, l);
	} else {
		// sendDHT("/dht11", h, f, l);
		// sendHTL("/dht11", h, f, l);
		if (h > humidityThreshold)
			sendHumidity(OscAddr_Humidity, h);
		if (f > temperatureThreshold)
			sendTemperature(OscAddr_Temperature, f);
	}
	// sendDHT("/dht11", h, f);
}
// ============================================================ //
// ==== main
// ============================================================ //
void loop()
{
	if(millis() - lastSend > SEND_INTERVAL) {

		readDHT();

		lastSend=millis();
	}

	bool bPinsChanged = false;

	// update digital pins
	for(int i=0; i < NUM_DIGITAL_PINS; i++) {

		int thisPin = digitalInPins[i];

		// store last value
		lastDigitalPinValues[i] = digitalPinValues[i];

		// read new values
		digitalPinValues[i] = digitalRead(thisPin);

		// flag if changed
		if(digitalPinValues[i] != lastDigitalPinValues[i]) {

			if(bNotifyPinChange[thisPin]) {
				bPinsChanged=true;
				sendPinOSC("/digitalpin", thisPin, digitalPinValues[i]);
			}
		}
	}


	// update digital pins
	for(int i=0; i < NUM_ANALOG_PINS; i++) {

		int thisPin = analogInPins[i];

		// store last value
		lastAnalogPinValues[i] = analogPinValues[i];

		// read new values
		analogPinValues[i] = analogRead(thisPin);

		if(bNotifyPinChange[thisPin]) {

			// flag if changed
			if(analogPinValues[i] > analogThreshold[i]) {
				bPinsChanged=true;
				sendPinOSC("/analogpin", thisPin, analogPinValues[i]);
				delay(10);
			}
		}

	}

	// indicator light
	if(bPinsChanged == true) {
		digitalWrite(ledPin, HIGH);
		lastChanged = millis();
		bLightOn = true;
	} else if ( millis() - lastChanged > indicateInterval && bLightOn == true) {
		digitalWrite(ledPin, LOW);
		bLightOn = false;
	}

	// receiving messages
	//===== TEST : receiving and dispatching an OSC Message
	OSCMessage msg_Received;

	int bytesToRead = Udp.parsePacket();	// how many bytes are available via UDP

	if (bytesToRead > 0) {
		while(bytesToRead--) {
			msg_Received.fill(Udp.read());	// filling the OSCMessage with the incoming data
		}
		if(!msg_Received.hasError()) {
			// if the address corresponds to a command, we dispatch it to the corresponding function
			msg_Received.dispatch(OscCmd_led , setLEDStatus);
			msg_Received.dispatch(OscCmd_SetDPin , setDigitalOut);
			msg_Received.dispatch(OscCmd_ReadDPin , readDigitalOSC);
			msg_Received.dispatch(OscCmd_ReadAPin , readAnalogOSC);
			msg_Received.dispatch(OscCmd_SetNotifyState , setNotifyState);
			msg_Received.dispatch(OscCmd_SetNodeID , setNodeID);
			msg_Received.dispatch(OscCmd_SetServerIP , setServerIP);
			msg_Received.dispatch(OscCmd_SetThreshold , setThreshold);
			msg_Received.dispatch(OscCmd_SetTemperature , setTemperatureThreshold);
			msg_Received.dispatch(OscCmd_SetHumidity , setHumidityThreshold);
			msg_Received.dispatch(OscCmd_UpdateEEPROM , writeEEPROMSettings);
			msg_Received.dispatch(OscCmd_ReadEEPROM , readEEPROMSettings);
			msg_Received.dispatch(OscCmd_GetServerIP , sendServerIP);
			msg_Received.dispatch(OscCmd_GetNotifyState , sendNotifyState);
			msg_Received.dispatch(OscCmd_GetAnalogThreshold , sendAnalogThreshold);
			// msg_Received.dispatch(OscCmd_GetHumidity , sendHumidity);
			// msg_Received.dispatch(OscCmd_GetTemperature , sendTemperature);
			// msg_Received.dispatch(OscCmd_GetDHT , sendDHT);
			msg_Received.dispatch(OscCmd_GetAnalogThreshold , sendAnalogThreshold);
			msg_Received.dispatch(OscCmd_GetNodeID , sendNodeID);

		}
	}
}
