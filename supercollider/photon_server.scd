// Robert Twomey | 2017-11-03
// rtwomey@ysu.edu
//
//  photon_server.scd
//
//  from the housemachine-workshop @ CMU
//
//  Description:
//  - this program is supercollider code to work with Partice Photon processors and OSC messaging
//
//   Directions:
//   - run each of the following sections below
//      (highlight and hit cmd-Enter to execute code)



// ======================
// A. SETUP SERVER AND DATA LOGGING
// ======================

// 1. Start the Server
(
s = Server.default;
s.boot;
~bDoLogging = false;
)


// 2. Set up log files
(
~bDoLogging = false;

// change ~datapath below to something appropriate for your computer
~datapath = "/Users/rtwomey/Desktop/logs/";
// ~datapath  = Document.current.path + "/logs/";


~logroutine = Routine( {
	inf.do {
		var waitTime = 10.0 * 60;
		var logfilename;

		// open new file
		logfilename = ~datapath++Date.getDate.stamp++".txt";
		~logfile = File(logfilename, "w");

		// start logging
		~bDoLogging = true;

		// log for long time

		waitTime.wait;

		~bDoLogging = false;
		~logfile.close;
	}
}).play();

~startTime = Date.getDate.rawSeconds;
)

~logroutine.free();


// 3. Create OSC Responders
(
// OSC responders
~pathkeys = [ \piezo, \chair, \door, \table, \toilet, \washingmachine, \motion, \frontoor, \humidity, \occupancy, \temperature, \shower, \microwave, \announce, \debug, \particleip, \nodeid, \nodeinfo, \testbundle, \digitalpins, \analogpins, \digitalpin, \analogpin, \dht11];

~oscfuncs = Dictionary.new;

~pathkeys.do { |path|
	var thisaddr = '/'++path;
	// path.postln;

	~oscfuncs.put(path,
		[{|msg, time, addr, recvPort|
			var thisDate, thisSeconds, tstampstring, msgstring, addrstring;

			msgstring = msg.join("\t");
			addrstring = [addr.ip, addr.port].join("\t");

			thisDate = Date.getDate;
			thisSeconds = "%.%".format(thisDate.second.asString.padLeft(2, "0"), thisDate.rawSeconds.frac.trunc(1e-4).asString.padRight(6, "0").split($.)[1]);
			tstampstring = thisDate.format("%H:%M:")++thisSeconds;

			[tstampstring, msg, addrstring, recvPort].postln;
			// [msg, time, addr, recvPort].postln;
			// [tstampstring, msg.reverse, addrstring, recvPort].postln;
			~bDoLogging.if {
				~logfile.write( "%\t%\t%\t%\n".format(tstampstring, msgstring, addrstring, recvPort) );
				~logfile.flush();
			};

			},
	thisaddr.asSymbol]);
	thisaddr.postln;
};

~oscresponders = ();

~oscfuncs.keys.do{ | key |
	var specs, func, addr;

	specs = ~oscfuncs[key];
	# func, addr = specs;
	// specs.postln;
	~oscresponders.put(key, OSCFunc.newMatching(func, addr, recvPort: 9999));

};
)

~logroutine.play()
~logroutine.stop()


// stop receiving messages from photon
(

~bDoLogging = false;
~oscresponders.do{ | resp | resp.remove };
~oscfuncs.do{ | func | func.remove };
~oscfuncs.postln;
)



// =======================
// B. COMMUNICATING WITH PHOTONS
// =======================

// 1. using broadcast addresses to communicate with the photons

NetAddr.broadcastFlag = true;
m = NetAddr("192.168.1.255", 8888);

// query node info
m.sendMsg("/getnodeinfo");


// setting server ip (this will change for all particles)
m.sendMsg("/setserverip", 192, 168, 1, 10); //blackbox
m.sendMsg("/setserverip", 192, 168, 1, 20); //laptop

// reading and writing settings, like node name
m.sendMsg("/writeeeprom");
m.sendMsg("/readeeprom");


// 2. communicating with individual photon
m = NetAddr("192.168.1.104", 8888);
m.sendMsg("/led", 7, 1); // on
m.sendMsg("/led", 7, 0); // on

m.sendMsg("/analogpin", 1, 100, "/toilet"); // on
m.sendMsg("/led", 7, 1); // on
m.sendMsg("/led", 7, 0); // off

m.sendMsg("/setnotify", 7, 0);
m.sendMsg("/settrigger", 7, 0);

m.sendMsg("/setnodeid", "/toilet");

m.sendMsg("/setserverip", 192, 168, 1, 10); //blackbox
m.sendMsg("/setserverip", 192, 168, 1, 20); //laptop

m.sendMsg("/writeeeprom");
m.sendMsg("/readeeprom");
m.sendMsg("/loaddefaults");
m.sendMsg("/getnodeinfo");

m.sendMsg("/settemperature", 80.0)
m.sendMsg("/sethumidity", 35.0)
m.sendMsg("/getdht")


m.sendMsg("/readapin", 10); // 10 - 16 are A0 - A5

// devices with dictionary
(
~devices=Dictionary.new();

~deviceAddresses = [
	[ \lamp, "192.168.1.30"],
	[ \hotplate, "192.168.1.31", \on, 0, 0, \off, 0, 1],
	[\desklamp, "192.168.1.32", \on, 0, 0, \off, 0, 1]
];

~deviceAddresses.do{ |dev|
	var name, addr;
	name = dev[0];
	addr = dev[1];
	~devices[name]=NetAddr(addr, 8888);
	postln("created netaddr"+name);
};
)

~devices[\hotplate].sendMsg("/setdpin", 0, 0); // on
~devices[\hotplate].sendMsg("/setdpin", 0, 1); // off

~devices[\lamp].sendMsg("/setdpin", 0, 0); // on
~devices[\lamp].sendMsg("/setdpin", 0, 1); // off

~devices[\desklamp].sendMsg("/setdpin", 0, 0); // on
~devices[\desklamp].sendMsg("/setdpin", 0, 1); // off


 store device behaviors
~devices[\lamp].sendMsg("/setserverip", 192, 168, 1, 10);
~devices[\lamp].sendMsg("/setnotify", 0, 1);
~devices[\lamp].sendMsg("/writeeeprom");
~devices[\desklamp].sendMsg("/setserverip", 192, 168, 1, 10);
~devices[\desklamp].sendMsg("/writeeeprom");

// hotplate
m = NetAddr("192.168.1.109", 8888); // hotplate
m.sendMsg("/setdpin", 0, 0); // on
m.sendMsg("/setdpin", 0, 1); // off

// lamp
m = NetAddr("192.168.1.105", 8888); // lamp
m.sendMsg("/setdpin", 0, 0); // on
m.sendMsg("/setdpin", 0, 1); // off

j = Array.series(10);
postln(j[2..])

array
// digital notify
m.sendMsg("/setnotify", 0, 0);
Routine( { (0..15).do { |i| m.sendMsg("/settrigger", i, 0); i.postln; 1.0.wait};}).play
Routine( { (100..115).do { |i| m = NetAddr(format("192.168.1.%", i), 8888); m.sendMsg("/setdpin", 0, 0); i.postln; 0.5.wait};}).play


// analog notify
m.sendMsg("/setnotify", 0, 0);
m.sendMsg("/settrigger", 10, 1);
m.sendMsg("/setthreshold", 10, 2000);
m.sendMsg("/setnotify", 10, 0);
m.sendMsg("/setnotify", 10, 1);

m.sendMsg("/getnodeinfo");
m.sendMsg("/getnotify")
m.sendMsg("/getthreshold")
m.sendMsg("/gettrigger")
m.sendMsg("/getnodeid")

Date.getDate.rawSeconds.postln; Date.getDate.secStamp.postln; Date.getDate.rawSeconds.frac.trunc(1e-4).asString.padRight(6, "0").split($.)[1].postln;
m.disconnect()

n = NetAddr("192.168.1.100", 9999);
n.sendMsg("/chair", 0);

OSCFunc.trace(true, false)
OSCFunc.trace(true, true)
OSCFunc.trace(false, true)
OSCFunc.trace(false, false)

"ssh pi@narrator.local 'festival --tts soft_rains.txt'".unixCmdGetStdOut
