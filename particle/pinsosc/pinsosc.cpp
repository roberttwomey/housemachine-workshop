// helloosc
//
//   a simple udp/OSC example to announce presence of particle
//
// Robert Twomey 2017
// rtwomey@ysu.edu

#include "application.h"
#include "OSC/OSCMessage.h"
#include "OSC/OSCBundle.h"

// server
IPAddress serverAddress = IPAddress(192, 168, 1, 20);	// IP address of blackbox at home
const int serverPort = 9999;		// to send data to the computer (from here)

 // this device
IPAddress coreAddress;
const int corePort = 8888;		// to send data to the Spark Core (from the computer)

// OSC ADDRESSES
#define NODE_ID_STR "/mynewphoton"
#define NODE_ID_LENGTH 20
char nodeid[NODE_ID_LENGTH] = NODE_ID_STR;

// interface
UDP Udp;

// pin changing
#define indicateInterval 500
unsigned long lastChanged = 0;
bool bLightOn = false;

// do not connect to the cloud (Wi-Fi)
SYSTEM_MODE(MANUAL); 
// More on modes here: https://docs.particle.io/reference/firmware/photon/#system-modes


// ============================================================ //
// ==== osc messaging
// ============================================================ //

void sendAnnounce() {
  // set broadcast address 192.168.1.255
  IPAddress broadcastAddr = IPAddress(coreAddress[0], coreAddress[1], coreAddress[2], 255);

  // notify server of info about node
  OSCMessage notifyMsg("/announce");
  notifyMsg.add(nodeid);
  notifyMsg.add(coreAddress[0]).add(coreAddress[1]).add(coreAddress[2]).add(coreAddress[3]);
  
  // transmit packet
  Udp.beginPacket(broadcastAddr, serverPort);
  notifyMsg.send(Udp);
  Udp.endPacket();
  notifyMsg.empty();
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

// ============================================================ //
// ==== setup
// ============================================================ //

void setup()
{
	// set D7 as LED output
	pinMode(D7, OUTPUT);

	// Connect to WiFi
	WiFi.on();
	WiFi.setCredentials("housemachine", "2029973952");
	WiFi.connect();
	waitUntil(WiFi.ready); 

	// necessary to update localIP
	Particle.process();

	// Start UDP
	Udp.begin(corePort);

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
	// receiving messages
	OSCMessage msg_Received;

	int bytesToRead = Udp.parsePacket();	// how many bytes are available via UDP

	if (bytesToRead > 0) {
		while(bytesToRead--) {
			msg_Received.fill(Udp.read());	// filling the OSCMessage with the incoming data
		}
		if(!msg_Received.hasError()) {

			// set pin state
			msg_Received.dispatch("/led", setLEDStatus);
		}
	}
}
