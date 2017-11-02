## Workshop: Reading and Writing Inhabited Space 

A workshop and demo exploring artistic applications of smart home technologies. Topics include wireless sensing, computer vision, and machine listening to narrate inhabited space. We consider the creative possibilities and personal consequences of life with ubiquitous sensing, perceiving machines. 

**Details:**

* Instructor: [Robert Twomey](mailto:robert@roberttwomey.com) ([website](http://roberttwomey.com))
* Date: Friday Nov 4, 2017
* Talk:	10-11am
* Workshop: 11:30-2pm
* Location: [CMU STUDIO for Creative Inquiry](http://studioforcreativeinquiry.org/)

**Technologies:**

*Hardware*
* Physical Sensing: [Particle Photon](https://store.particle.io/#photon) with various sensors (PIR, IR rangefinder, beam break, reed switch, DHT, piezo, force sensor)
* Machine Listeners: [Raspberry Pi](https://www.raspberrypi.org/products/raspberry-pi-3-model-b/) with [MEMS microphone](https://github.com/roberttwomey/ics43432-pi), ([alternative from adafruit](https://www.adafruit.com/product/3421)) Python and [webrtcvad](https://pypi.python.org/pypi/webrtcvad).
* Machine Watchers: [Raspberry Pi](https://www.raspberrypi.org/products/raspberry-pi-3-model-b/) with [Fisheye Camera](https://www.amazon.com/gp/product/B013JWEGJQ/) and [pikrellcam](https://github.com/billw2/pikrellcam)

*Software*
* Server software in Python, Supercollider, p5, Processing
* Sensor software in C++/Wiring/Particle HAL
* Data analysis in Python with [OpenCV](https://pypi.python.org/pypi/pyopencv/2.1.0.wr1.2.0) and [librosa](https://github.com/librosa/librosa), Supercollider with [SCMIR](https://composerprogrammer.com/code.html).

*Communication*
* Server: port 9999, address 192.168.1.20, broadcast addr 192.168.1.255
  * broadcast addresses are used to announce the nodes
* Sensor nodes: port 8888, addresses vary on the 192.168.1.xxx 
* Local WiFi network: ssid housemachine, pwd 2029973952
* Messaging: OSC

## Demos

1. Machine Listener
   * "voice" triggered recording with webrtcVAD
2. Machine Watcher
   * motion triggered recording with pikrellcam
3. Physical Sensor
4. Audio Analysis
   * feature analysis (SCMIR or librosa)
   * clustering (k-Means, HDBSCAN)
   * visualization 
   * Google TTS, Kaldi, CMU Sphinx
5. Video analysis
   * Background subtraction (KNN, MOG, etc) with OpenCV
   * inhabitance maps
   * ROIs (future directions)
6. Sensor analysis
   * cleaning data
   * meaningful clustering, etc (future directions)
6. Representational strategies
   * audio synthesis
   * video synthesis
   * sensor narrative
   * speech transcripts
   * robotic re-emobidment

## Workshop


0. Hardware used
1. [Setup](#Setup)
2. [Compiling and uploading to the photon](#compile-and-upload-to-the-photon)
3. [Communicating with the photon](#communicating-with-the-photon)
4. [Concept and applications](#concept-and-applications)
5. [Going further](#going-further)

## Setup
### Software
Install the following software if you do not already have it:
* SuperCollider: http://supercollider.github.io/
* Particle Photon CLI: https://docs.particle.io/guide/getting-started/connect/core/
* A text editor like [Sublime](https://www.sublimetext.com/) or [Atom](https://atom.io/). I will use sublime for this workshop.
* git

### Download the source code for the Particle Photon

In terminal, clone the particle software:

`git clone https://github.com/spark/firmware`

NOTE: all further paths are relative to wherever you downloaded the particle source.

## Compile and Upload to the Photon

### DFU (Device Firmware Upgrade) mode
In order to update the firmware and upload programs, you need to place the particle in [DFU mode](https://docs.particle.io/guide/getting-started/modes/photon/#dfu-mode-device-firmware-upgrade-). Follow these setups:

1. Hold down BOTH buttons
2. Release only the RESET button, while holding down the SETUP button.
3. Wait for the LED to start flashing yellow (it will flash magenta first)
4. Release the SETUP button

The light should be flashing yellow. This means it is ready to upload new programs.

### Compile and upload firmware

1. Change directory to the particle firmware modules:

`cd modules`

2. Place the particle in DFU mode (see above)

3. Build the firmware:

`make all PLATFORM=photon -s program-dfu`

When done building the firmware, it will upload to the particle. It will do two separate uploads. You will see it cycle back to blinking yellow DFU mode twice.

### Compile and upload your application:


1. Write your application

2. Link it to the firmware/applications/ directory:

`ln -s path/to/myApplication firmware/applications/myApplication`

3. From the particle firmware directory, change to main:

`cd firmware/main`

4. Place the particle in DFU mode (see above)

5. Compile and upload your application (f.ex., mine is called "housenode"):

`make PLATFORM=photon APP=housenode program-dfu`


## Communicating with the Photon
### Connect to the local WiFi network

Select the housemachine network:

ssid: 'housemachine'

pwd: 2029973952

### Run the server program

The server program in either supercollider, p5, processing or python will listen for broadcast messages on the local network. When a particle turns on it will announce it's presence.

### Detect the Photon when it goes live

1. Power on the photon by plugging in an AC adapter. It will go through it's startup routine. When it finally connects to the network, it should show a slow green pulse.

2. Did your server program see the announce message? If so, what is the IP address and name for the photon?


### Renaming your photon

Set a new name for your photon.

### Read Sensor Data from Photon

Data logging
Processing data

### Send Commands to Photon

Physical outputs, speech synthesis, dot matrix printers, screens

### Physically staging the photon
Issues with physically staging the photon

## Concept and Applications

How can we use these wireless sensors? 

### Brainstorm on uses of wireless sensor technology

What would you monitor? What physical gestures / operations?
What would you control with a wireless processor? What actuator would you use?

### Try it

## Going Further

* Using the Particle [Desktop IDE](https://docs.particle.io/guide/tools-and-features/dev/#getting-started)

* Other Client/Server possibilities with Particle



