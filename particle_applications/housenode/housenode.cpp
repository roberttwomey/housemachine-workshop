//
// housenode
//
// udp/OSC node for machine for living in
//
// Robert Twomey 2015
// rtwomey@uw.edu
//

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
char OscCmd_GetTriggerMode[12] = "/gettrigger";

char OscAddr_AnalogPin[11] = "/analogpin";
char OscAddr_DigitalPin[12] = "/digitalpin";
char OscAddr_Debug[7] = "/debug";

char OscAddr_ParticleIP[12] = "/particleip";
char OscAddr_NodeID[8] = "/nodeid";
char OscAddr_NodeInfo[10] = "/nodeinfo";
char OscAddr_Announce[10] = "/announce";

// ---- REMOTE SERVER
IPAddress serverAddress = IPAddress(192, 168, 1, 20);	// IP address of blackbox at home
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

void sendNodeID() {

	// notify server of new id
	OSCMessage notifyMsg(OscAddr_NodeID);

	notifyMsg.add(nodeid);

	Udp.beginPacket(serverAddress, serverPort);
	notifyMsg.send(Udp); // send the bytes
	Udp.endPacket();
	notifyMsg.empty();
}

void sendNodeID( OSCMessage &mess) {
	sendNodeID();
}


void sendNodeInfo(IPAddress destAddr, int destPort) {

  // notify server of info about node
  OSCMessage notifyMsg(OscAddr_NodeInfo);
  notifyMsg.add(nodeid);
  notifyMsg.add(serverAddress[0]).add(serverAddress[1]).add(serverAddress[2]).add(serverAddress[3]);

  Udp.beginPacket(destAddr, destPort);
  notifyMsg.send(Udp); // send the bytes
  Udp.endPacket();
  notifyMsg.empty();
}

void sendNodeInfo() {
  sendNodeInfo(serverAddress, serverPort);
}

void sendNodeInfo( OSCMessage &mess) {
  sendNodeInfo(serverAddress, serverPort);
}

void sendParticleIP(IPAddress destAddr) {
	OSCMessage coreIPMessage(OscAddr_ParticleIP);

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
	OSCMessage coreIPMessage(OscAddr_Debug);

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
	OSCMessage coreIPMessage(OscAddr_Debug);

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

void sendAnalogTriggerMode() {
	OSCMessage coreIPMessage(OscAddr_Debug);

	for(int i=0; i < NUM_ANALOG_PINS; i++)
		coreIPMessage.add(bAnalogTriggerMode[i]);

	coreIPMessage.add(nodeid);

	Udp.beginPacket(serverAddress, serverPort);
	coreIPMessage.send(Udp);
	Udp.endPacket();
	coreIPMessage.empty();
}

void sendAnalogTriggerMode(OSCMessage &mess) {
	sendAnalogTriggerMode();
}

void sendAnalogThreshold() {
	OSCMessage coreIPMessage(OscAddr_Debug);

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

void sendAnnounce() {
  IPAddress broadcastAddr = IPAddress(coreAddress[0], coreAddress[1], coreAddress[2], 255);

  // notify server of info about node
  OSCMessage notifyMsg(OscAddr_Announce);
  notifyMsg.add(nodeid);
  notifyMsg.add(coreAddress[0]).add(coreAddress[1]).add(coreAddress[2]).add(coreAddress[3]);
  // notifyMsg.add(serverAddress[0]).add(serverAddress[1]).add(serverAddress[2]).add(serverAddress[3]);

  Udp.beginPacket(broadcastAddr, serverPort);
  notifyMsg.send(Udp); // send the bytes
  Udp.endPacket();
  notifyMsg.empty();
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

void setAnalogTriggerMode(OSCMessage &mess) {
	if (mess.size() == 2 && mess.isInt(0) && mess.isInt(1)) {
		int thisPin = mess.getInt(0);
		bool bThisTriggerMode = (mess.getInt(1) == 1);
		bAnalogTriggerMode[thisPin - A0] = bThisTriggerMode;
	}
}

void setNotifyState(OSCMessage &mess) {
	if (mess.size() == 2 && mess.isInt(0) && mess.isInt(1)) {
		int thisPin = mess.getInt(0);
		bool bThisNotifyState = (mess.getInt(1) == 1);
		bNotifyPinChange[thisPin] = bThisNotifyState;
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
	bool bAnalogTriggerMode[NUM_ANALOG_PINS];
};

void readEEPROMSettings() {

	int i = 0;

	NodeSettings newsettings;

	EEPROM.get(1, newsettings);

	// copy into variables
	strcpy(nodeid, newsettings.nodeid);
	serverAddress = newsettings.serverAddress;
	memcpy(bNotifyPinChange, newsettings.bNotifyPinChange, NUM_PINS_TOTAL * sizeof(bool));
	memcpy(analogThreshold, newsettings.analogThreshold, NUM_ANALOG_PINS * sizeof(int));
	memcpy(bAnalogTriggerMode, newsettings.bAnalogTriggerMode, NUM_ANALOG_PINS * sizeof(bool));

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
	memcpy(mysettings.bAnalogTriggerMode, bAnalogTriggerMode, NUM_ANALOG_PINS * sizeof(bool));

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

  // broadcast my IP address on local subnet
  sendAnnounce();
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
				sendPinOSC(OscAddr_DigitalPin, thisPin, digitalPinValues[i]);
			}
		}
	}

	// update analog pins
	for(int i=0; i < NUM_ANALOG_PINS; i++) {

		int thisPin = analogInPins[i];

		// store last value
		lastAnalogPinValues[i] = analogPinValues[i];

		// read new values
		analogPinValues[i] = analogRead(thisPin);

		if(bNotifyPinChange[thisPin]) {

			// are we in analog triggered mode
			if(bAnalogTriggerMode[i]) {
				if(analogPinValues[i] > analogThreshold[i]) {
					if(!bAnalogTriggered[i]) {
						bAnalogTriggered[i]=true;
						bPinsChanged=true;
						sendPinOSC(OscAddr_AnalogPin, thisPin, analogPinValues[i]);
						delay(10);
					}
					// keeps indicator light on while above threshold
					bPinsChanged=true;
				} else {
					if(bAnalogTriggered[i]) {
						bAnalogTriggered[i]=false;
						bPinsChanged=true;
						sendPinOSC(OscAddr_AnalogPin, thisPin, analogPinValues[i]);
						delay(10);
					}
				}
			} else {
				// simple analog mode
				bPinsChanged=true;
				sendPinOSC(OscAddr_AnalogPin, thisPin, analogPinValues[i]);
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
	OSCMessage msg_Received;

	int bytesToRead = Udp.parsePacket();	// how many bytes are available via UDP

	if (bytesToRead > 0) {
		while(bytesToRead--) {
			msg_Received.fill(Udp.read());	// filling the OSCMessage with the incoming data
		}
		if(!msg_Received.hasError()) {
			// set internal values
			msg_Received.dispatch(OscCmd_SetNodeID , setNodeID);
			msg_Received.dispatch(OscCmd_SetServerIP , setServerIP);

			// set pin state
			msg_Received.dispatch(OscCmd_led , setLEDStatus);
			msg_Received.dispatch(OscCmd_SetDPin , setDigitalOut);

			// read pin
			msg_Received.dispatch(OscCmd_ReadDPin , readDigitalOSC);
			msg_Received.dispatch(OscCmd_ReadAPin , readAnalogOSC);

			// set pin notification behavior
			msg_Received.dispatch(OscCmd_SetNotifyState , setNotifyState);
			msg_Received.dispatch(OscCmd_SetAnalogTriggerMode , setAnalogTriggerMode);
			msg_Received.dispatch(OscCmd_SetThreshold , setThreshold);

			// read and write settings
			msg_Received.dispatch(OscCmd_UpdateEEPROM , writeEEPROMSettings);
			msg_Received.dispatch(OscCmd_ReadEEPROM , readEEPROMSettings);

      		// send information about node to server
      		msg_Received.dispatch(OscCmd_GetNodeInfo , sendNodeInfo);
			msg_Received.dispatch(OscCmd_GetNodeID , sendNodeID);
			msg_Received.dispatch(OscCmd_GetServerIP , sendServerIP);
			msg_Received.dispatch(OscCmd_GetNotifyState , sendNotifyState);
			msg_Received.dispatch(OscCmd_GetAnalogThreshold , sendAnalogThreshold);
			msg_Received.dispatch(OscCmd_GetTriggerMode , sendAnalogTriggerMode);
		}
	}
}
