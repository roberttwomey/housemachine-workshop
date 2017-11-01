Workshop: Reading and Writing Inhabited Space 
========

A workshop and demo exploring artistic applications of smart home technologies. Topics include wireless sensing, computer vision, and machine listening to narrate inhabited space. We consider the creative possibilities and personal consequences of life with ubiquitous sensing, perceiving machines. 

## Details

Instructor: [Robert Twomey](mailto:robert@roberttwomey.com) ([website](http://roberttwomey.com))

Talk:	10-11am

Workshop: 11:30-2pm

Date: Friday Nov 4, 2017

Location: [CMU STUDIO for Creative Inquiry](http://studioforcreativeinquiry.org/)


Technologies
========

*Hardware*

* [Particle Photon](https://store.particle.io/#photon)
* [Raspberry Pi](https://www.raspberrypi.org/products/raspberry-pi-3-model-b/)
  * [Fisheye Pi Camera](https://www.amazon.com/gp/product/B013JWEGJQ/)
  * [MEMS microphone](https://github.com/roberttwomey/ics43432-pi), ([alternative from adafruit](https://www.adafruit.com/product/3421))
* Various sensors
  * PIR, IR rangefinder, beam break, reed switch, DHT, piezo, force sensor

*Software*

* Python
* OSC
* C++/Particle cli

*Networking*

* Server
  * server port: 9999
  * server address: 192.168.1.20
  * broadcast address: 192.168.1.255
    * broadcast addresses are used to announce the nodes
* Network
* OSC


Schedule
========

## Connect to WiFI

ssid: 'housemachine'
pwd: 2029973952

## Run the server program

## Testing and naming a photon

Power on the photon by plugging in an AC adapter. Set a new name for your photon.

!! make an app for renaming photon with python(?) !!

## Physically staging the photon

## Simple sensing / receiving setup



## Data logging

## Outputs

Physical outputs, speech synthesis, dot matrix printers, screens

## Processing data


## Going Further

* Developing with the Particle Photon locally
* Other Client/Server possibilities with Particle
* 

## Provisioning


Reference
=====

## Serving P5.js on localhost

1. Setup local [HTTP server](https://www.npmjs.com/package/http-server):

`npm install http-server -g`

2. Change directory to path of your program, f.ex.:

`cd housemachine-workshop\browser-speechrec`

3. Run the server:

`http-server`

4. Open your browser to view the page and run the sketch. Open address:

`http://localhost.com:8080`


Leftovers
=======

## Compiling and uploading code to Particle Photon
I am using a simplified "firmata-like" program running on the particle photon. You are able to configure this program somewhat through OSC commands. 

The instructions below are for compiling and uploading code directly to the Particle Photon over USB. This circumvents the usual particle cloud compile and upload process. There is also some way to set up a local cloud to communicate with the Photon.

1. Setup Particle Photon
From https://docs.particle.io/guide/getting-started/connect/core/

2. Installing Node.js

The Particle CLI runs with Node.js. Grab the latest version from [the Node.js website](https://nodejs.org/en/download/) Launch the installer and follow the instructions to install node.js. Next, open your terminal, or preferred terminal program.

3. Installing the Particle CLI

Type: `npm install -g particle-cli`

Note: You may need to update xcode at this time.


## Upload your first program

## Receiving Messages

## Triggering Responses

## Logging Data

## Data Analysis


