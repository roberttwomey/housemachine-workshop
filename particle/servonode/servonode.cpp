// 
// servonode
//
// udp/OSC node for machine for living in
// 
// Robert Twomey 2015
// rtwomey@uw.edu
//

#include "application.h"
#include "OSC/OSCMessage.h"
#include "OSC/OSCBundle.h"

//----- OUTPUTS
int ledPin = D7;

//----- OSC COMMANDS
char OscCmd_led[5] = "/led"; // led indicator

char OscCmd_SetNodeID[11] = "/setnodeid"; // 10 characters + 1 null terminator
char OscCmd_SetServerIP[13] = "/setserverip"; // 12 characters + 1 null terminator
char OscCmd_SetNotifyState[11] = "/setnotify"; 
char OscCmd_SetThreshold[14] = "/setthreshold";

char OscCmd_ReadDPin[10] = "/readdpin"; 
char OscCmd_ReadAPin[10] = "/readapin";

char OscCmd_UpdateEEPROM[13] = "/writeeeprom"; 
char OscCmd_ReadEEPROM[12] = "/readeeprom";

char OscCmd_GetServerIP[13] = "/getserverip";
char OscCmd_GetNotifyState[11] = "/getnotify";
char OscCmd_GetNodeID[11] = "/getnodeid";
char OscCmd_GetAnalogThreshold[14] = "/getthreshold";

char OscCmd_AttachServo[13] = "/attachservo";
char OscCmd_DetachServo[13] = "/detachservo";
char OscCmd_SetServo[10] = "/setservo";

Servo servo0;


// ---- REMOTE SERVER
// IPAddress serverIPAddress = IPAddress(10, 1, 10, 78);	// laptop at warehouse
// IPAddress serverIPAddress = IPAddress(192, 168, 0, 2);	// IP address of laptop at home
// IPAddress serverIPAddress = IPAddress(192, 168, 0, 255);	// broadcast on local network

IPAddress serverIPAddress = IPAddress(192, 168, 1, 20);	// IP address of blackbox at home
#define REMOTEPORT 9999		// to send data to the computer (from here)


// ---- THIS DEVICE
IPAddress coreIPAddress;
#define LOCALPORT  8888		// to send data to the Spark Core (from the computer)

// update this id for each node
// #define UNIT_ID 5
#define READ_EEPROM

// string / osc address voodoo using preprocessor
#define xstr(s) str(s)
#define str(s) #s

// OSC ADDRESSES
// #define NODE_ID_STR "photon" xstr(UNIT_ID)
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
	
	Udp.beginPacket(serverIPAddress, REMOTEPORT);
	pinMsg.send(Udp); // send the bytes
	Udp.endPacket();
	pinMsg.empty(); // empty the message to free room for a new one			
}	

void sendNodeID() {
	
	// notify server of new id
	OSCMessage notifyMsg("/nodeid");
	
	notifyMsg.add(nodeid);
	
	Udp.beginPacket(serverIPAddress, REMOTEPORT);
	notifyMsg.send(Udp); // send the bytes
	Udp.endPacket();
	notifyMsg.empty();
}

void sendNodeID( OSCMessage &mess) {
	sendNodeID();
}

void sendParticleIP(IPAddress destAddr) {
	OSCMessage coreIPMessage("/particleip");
	
	coreIPMessage.add(coreIPAddress[0]).add(coreIPAddress[1]).add(coreIPAddress[2]).add(coreIPAddress[3]);
	coreIPMessage.add(nodeid);

	Udp.beginPacket(destAddr, REMOTEPORT);
	coreIPMessage.send(Udp);
	Udp.endPacket();
	coreIPMessage.empty();
	
}

void sendParticleIP() {
	sendParticleIP(serverIPAddress);
}

void sendServerIP() {
	OSCMessage coreIPMessage("/debug");
	
	coreIPMessage.add(serverIPAddress[0]).add(serverIPAddress[1]).add(serverIPAddress[2]).add(serverIPAddress[3]);
	coreIPMessage.add(nodeid);

	Udp.beginPacket(serverIPAddress, REMOTEPORT);
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

	Udp.beginPacket(serverIPAddress, REMOTEPORT);
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

	Udp.beginPacket(serverIPAddress, REMOTEPORT);
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
		serverIPAddress = { b0, b1, b2, b3 };
		
		// coreIPAddress = WiFi.localIP();

		sendParticleIP();
	}
}

void setLEDStatus(OSCMessage &mess)
{
	if (mess.size() == 2 && mess.isInt(0) && mess.isInt(1)) {
		int thisLed = mess.getInt(0);
		int thisStatus = mess.getInt(1);

		digitalWrite(thisLed, thisStatus);
	}
}

void attachServo(OSCMessage &mess) {
	if (mess.size() == 1 && mess.isInt(0)) {
		int thisPin = mess.getInt(0);
		servo0.attach(thisPin);
	}
}

void detachServo(OSCMessage &mess) {
	if (mess.size() == 1 && mess.isInt(0)) {
		int thisPin = mess.getInt(0);
		servo0.detach();
	}
}

void setServo(OSCMessage &mess) {
	if (mess.size() == 2 && mess.isInt(0) && mess.isInt(1)) {
		int thisPin = mess.getInt(0);
		int thisUs = mess.getInt(1);
		servo0.writeMicroseconds(thisUs);
	}
}

// ============================================================ //
// ==== read/write node-specific settings from EEPROM
// ============================================================ //

struct NodeSettings {
	IPAddress serverIPAddress;
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

	// // serverIPAddress = { b0, b1, b2, b3 };
	// serverIPAddress = { b0, b1, b2, b3 };

	// // nodeid
	// for(i = 0; i < 20; i++)
	// 	nodeid[i] = EEPROM.read(i);
	
	NodeSettings newsettings;

	EEPROM.get(1, newsettings);

	// copy into variables
	strcpy(nodeid, newsettings.nodeid);
	serverIPAddress = newsettings.serverIPAddress;
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
	mysettings.serverIPAddress = serverIPAddress;
	strcpy(mysettings.nodeid, nodeid);
	memcpy(mysettings.bNotifyPinChange, bNotifyPinChange, NUM_PINS_TOTAL * sizeof(bool));
	memcpy(mysettings.analogThreshold, analogThreshold, NUM_ANALOG_PINS * sizeof(int));

	// // server IP address
	// EEPROM.update(0, serverIPAddress[0]);
	// EEPROM.update(1, serverIPAddress[1]);
	// EEPROM.update(2, serverIPAddress[2]);
	// EEPROM.update(3, serverIPAddress[3]);

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
	for(int i=0; i < 7; i++)
		pinMode(i, INPUT_PULLUP);
	
	// set D7 as LED output
	pinMode(D7, OUTPUT); 
	
	// // wink to say hello at start
	// for (int i=0; i < 10; i ++) {
	// 	digitalWrite(ledPin, LOW);
	// 	delay(100);
	// 	digitalWrite(ledPin, HIGH);
	// 	delay(100);
	// }
	
	// readEEPROMSettings();

	// Connect to WiFi
	WiFi.on();
	WiFi.setCredentials("housemachine", "forliving");
	// WiFi.setCredentials("DXARTS Ballard", "dxartsword");
	WiFi.connect();

	waitUntil(WiFi.ready);
	// necessary to update localIP

	Particle.process();

	// Start UDP
	Udp.begin(LOCALPORT);
	
	// load settings
	#ifdef READ_EEPROM
	readEEPROMSettings();
	#endif

	// get the IP address of the device and send it as an OSC Message
	coreIPAddress = WiFi.localIP();

	// broadcast address on local subnet
	IPAddress broadcastAddr = IPAddress(coreIPAddress[0], coreIPAddress[1], coreIPAddress[2], 255);
	sendParticleIP(broadcastAddr); // announce IP address
}


// ============================================================ //
// ==== main 
// ============================================================ //
void loop()
{
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

	// // need a delay
	// delay(1);
	
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
			msg_Received.dispatch(OscCmd_ReadDPin , readDigitalOSC);
			msg_Received.dispatch(OscCmd_ReadAPin , readAnalogOSC);
			msg_Received.dispatch(OscCmd_SetNotifyState , setNotifyState);
			msg_Received.dispatch(OscCmd_SetNodeID , setNodeID);								
			msg_Received.dispatch(OscCmd_SetServerIP , setServerIP);								
			msg_Received.dispatch(OscCmd_SetThreshold , setThreshold);								
			msg_Received.dispatch(OscCmd_SetServo , setServo);								
			msg_Received.dispatch(OscCmd_AttachServo , attachServo);								
			// msg_Received.dispatch(OscCmd_DetachServo , detachServo);								
			msg_Received.dispatch(OscCmd_UpdateEEPROM , writeEEPROMSettings);								
			msg_Received.dispatch(OscCmd_ReadEEPROM , readEEPROMSettings);	
			msg_Received.dispatch(OscCmd_GetServerIP , sendServerIP);								
			msg_Received.dispatch(OscCmd_GetNotifyState , sendNotifyState);
			msg_Received.dispatch(OscCmd_GetAnalogThreshold , sendAnalogThreshold);
			msg_Received.dispatch(OscCmd_GetNodeID , sendNodeID);								
				
		}
	}
}
