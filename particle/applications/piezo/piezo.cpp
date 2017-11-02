// 
// pizeo interaction sensor
//
// OSC udp node for machine for living in
//
// Robert Twomey 2017
// rtwomey@ysu.edu
//
//
// to do:
// - check if we lost our connection to the wireless network, then periodically recheck
// - special osc command to set default values for NodeSettings, so they don't have junk

#include "application.h"
#include "OSC/OSCMessage.h"
#include "OSC/OSCBundle.h"


#define READ_EEPROM


// state information
const int MOTION_TIMEOUT = 500UL;  // 0.5 seconds of led everytime motion is detected, outside of the debounce time
const int debounceIgnoreTime = 300UL;
const int vibrationSensor = A0;
unsigned long lastVibrationTime;
bool sent = false;
// outputs
const int ledPin = D7;

// ---- REMOTE SERVER
IPAddress serverAddress = IPAddress(192, 168, 1, 10);  // IP address of blackbox at home
int serverPort = 9999;  // to send data to the computer (from here)

// ---- THIS DEVICE
IPAddress coreAddress;
int localPort = 8888;

// ---- OSC ADDRESS
#define NODE_ID_STR "/piezo"
#define NODE_ID_LENGTH 20
char nodeid[NODE_ID_LENGTH] = NODE_ID_STR;

//----- OSC COMMANDS
char OscCmd_led[5] = "/led"; // led indicator

char OscCmd_SetNodeID[11] = "/setnodeid"; // 10 characters + 1 null terminator
char OscCmd_SetServerIP[13] = "/setserverip"; // 12 characters + 1 null terminator
// char OscCmd_SetNotifyState[11] = "/setnotify";
// char OscCmd_SetThreshold[14] = "/setthreshold";
// char OscCmd_SetAnalogTriggerMode[12] = "/settrigger";

char OscCmd_SetDPin[9] = "/setdpin";
char OscCmd_ReadDPin[10] = "/readdpin";
char OscCmd_ReadAPin[10] = "/readapin";

char OscCmd_UpdateEEPROM[13] = "/writeeeprom";
char OscCmd_ReadEEPROM[12] = "/readeeprom";
char OscCmd_LoadDefaultSettings[14] = "/loaddefaults";
char OscCmd_GetServerIP[13] = "/getserverip";
char OscCmd_GetNotifyState[11] = "/getnotify";
char OscCmd_GetNodeInfo[13] = "/getnodeinfo";
char OscCmd_GetAnalogThreshold[14] = "/getthreshold";
char OscCmd_GetTriggerMode[12] = "/gettrigger";

// osc addresses going to server
char OscAddr_AnalogPin[11] = "/analogpin";
char OscAddr_DigitalPin[12] = "/digitalpin";
char OscAddr_Debug[7] = "/debug";
// char OscAddr_ParticleIP[12] = "/particleip";
char OscAddr_NodeInfo[10] = "/nodeinfo";
char OscAddr_NodeID[8] = "/nodeid";
char OscAddr_Announce[10] = "/announce";

UDP Udp;
SYSTEM_MODE(MANUAL); // do not connect to the cloud (Wi-Fi)
// More on modes here: https://docs.particle.io/reference/firmware/photon/#system-modes



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

// ---------------------------------------- //
// sensor reading
// ---------------------------------------- //

void vibCheck()
{
  // you can debounce this as well:
  if(millis() - lastVibrationTime < debounceIgnoreTime) // will ignore bouncy signals inside 100ms
    return;
  //
  lastVibrationTime = millis();
  sent = false;
}

// ---------------------------------------- //
// OSC messaging
// ---------------------------------------- //

void sendPinOSC(const char *addr, int thisPin, int thisVal)
{
  OSCMessage pinMsg(addr);
  pinMsg.add(thisPin);
  pinMsg.add(thisVal);

  Udp.beginPacket(serverAddress, serverPort);
  pinMsg.send(Udp); // send the bytes
  Udp.endPacket();
  pinMsg.empty(); // empty the message to free room for a new one
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

void sendTriggered() {
  OSCMessage notifyMsg(OscAddr_NodeInfo);

  notifyMsg.add("triggered");
  Udp.beginPacket(serverAddress, serverPort);
  notifyMsg.send(Udp); // send the bytes
  Udp.endPacket();
  notifyMsg.empty();
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

// ------------------------------------------------------------ //
// ---- set notification behavior, threshold, etc for pins
// ------------------------------------------------------------ //

void setNodeID(OSCMessage &mess) {
  if (mess.size() == 1 && mess.isString(0)) {
    // set nodeid to received string
    char thisID[NODE_ID_LENGTH];

    int bytes = mess.getString(0, thisID, NODE_ID_LENGTH);
    // nodeid = char[20];

    strcpy(nodeid, thisID);

    sendNodeInfo();
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

    sendNodeInfo();
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

NodeSettings defaultSettings;

void storeAsDefaultSettings() {

  defaultSettings.serverAddress = serverAddress;
  strcpy(defaultSettings.nodeid, nodeid);
  memcpy(defaultSettings.bNotifyPinChange, bNotifyPinChange, NUM_PINS_TOTAL * sizeof(bool));
  memcpy(defaultSettings.analogThreshold, analogThreshold, NUM_ANALOG_PINS * sizeof(int));
  memcpy(defaultSettings.bAnalogTriggerMode, bAnalogTriggerMode, NUM_ANALOG_PINS * sizeof(bool));
}

void loadDefaultSettings() {

  strcpy(nodeid, defaultSettings.nodeid);
  serverAddress = defaultSettings.serverAddress;
  memcpy(bNotifyPinChange, defaultSettings.bNotifyPinChange, NUM_PINS_TOTAL * sizeof(bool));
  memcpy(analogThreshold, defaultSettings.analogThreshold, NUM_ANALOG_PINS * sizeof(int));
  memcpy(bAnalogTriggerMode, defaultSettings.bAnalogTriggerMode, NUM_ANALOG_PINS * sizeof(bool));

  sendNodeInfo();
}

void loadDefaultSettings(OSCMessage &mess) {
  loadDefaultSettings();
}

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

  sendNodeInfo();
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


// ---------------------------------------- //
// setup
// ---------------------------------------- //

void setup() {
  // store hard coded settings as default
  storeAsDefaultSettings();

  // read settings from eeprom
    // load settings
  #ifdef READ_EEPROM
  readEEPROMSettings();
  #endif
  // readEEPROMSettings();

    // Connect to WiFi
  WiFi.on();
  WiFi.setCredentials("housemachine", "2029973952");
  WiFi.connect();
  waitUntil(WiFi.ready);

  // necessary to update localIP
  Particle.process();

  // Start UDP
  Udp.begin(localPort);

  // get the IP address of the device
  coreAddress = WiFi.localIP();

  // broadcast my IP address on local subnet
  sendAnnounce();

  // setup inputs
  pinMode(vibrationSensor, INPUT); //Piezo vibration sensor is connected

  // setup outputs
  pinMode(ledPin, OUTPUT);
  attachInterrupt(vibrationSensor, vibCheck, CHANGE);
}

// ---------------------------------------- //
// main loop
// ---------------------------------------- //

void loop()
{
  if((millis() - lastVibrationTime < indicateInterval))
  {
    // sendTriggered();
    if(!sent) {
      sendPinOSC(nodeid, vibrationSensor, 1);
      sent = true;
    }
    if(digitalRead(ledPin) == LOW)
      digitalWrite(ledPin, HIGH);
  }
  else
  {
    digitalWrite(ledPin, LOW);
  }

  // receiving messages
  OSCMessage msg_Received;

  int bytesToRead = Udp.parsePacket();  // how many bytes are available via UDP

  if (bytesToRead > 0) {
    while(bytesToRead--) {
      msg_Received.fill(Udp.read());  // filling the OSCMessage with the incoming data
    }
    if(!msg_Received.hasError()) {
      // if the address corresponds to a command, we dispatch it to the corresponding function
      // set internal values
      msg_Received.dispatch(OscCmd_led , setLEDStatus);
      msg_Received.dispatch(OscCmd_SetNodeID , setNodeID);
      msg_Received.dispatch(OscCmd_SetServerIP , setServerIP);

      // read and write settings
      msg_Received.dispatch(OscCmd_UpdateEEPROM , writeEEPROMSettings);
      msg_Received.dispatch(OscCmd_ReadEEPROM , readEEPROMSettings);
      msg_Received.dispatch(OscCmd_LoadDefaultSettings, loadDefaultSettings);

      // send information about node to server
      msg_Received.dispatch(OscCmd_GetNodeInfo , sendNodeInfo);

      // from housenode.cpp, not used:

      // msg_Received.dispatch(OscCmd_SetDPin , setDigitalOut);
      // msg_Received.dispatch(OscCmd_ReadDPin , readDigitalOSC);
      // msg_Received.dispatch(OscCmd_ReadAPin , readAnalogOSC);
      // msg_Received.dispatch(OscCmd_SetNotifyState , setNotifyState);
      // msg_Received.dispatch(OscCmd_SetAnalogTriggerMode , setAnalogTriggerMode);
      // msg_Received.dispatch(OscCmd_SetThreshold , setThreshold);


      // msg_Received.dispatch(OscCmd_GetServerIP , sendServerIP);
      // msg_Received.dispatch(OscCmd_GetNotifyState , sendNotifyState);
      // msg_Received.dispatch(OscCmd_GetAnalogThreshold , sendAnalogThreshold);
      // msg_Received.dispatch(OscCmd_GetTriggerMode , sendAnalogTriggerMode);
      // msg_Received.dispatch(OscCmd_GetNodeID , sendNodeID);

    }
  }

}
