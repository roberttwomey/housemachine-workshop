#!/usr/bin/python

# simple example testing webrtcvad (Voice Activity Detection) with alsaaudio
#
# rtwomey@ysu.edu 2017
# runs on 
# from here https://github.com/alexa-pi/AlexaPi/blob/a10a712ec768a6e323c8cbb11e31bae4ed5fd61d/src/alexapi/capture.py
#
# -works great with usb mic plughw:1,0
# -recording from i2s with python has periodic spikes in file.
# to test recording: 
# arecord -r 16000 -f S16_LE -c 1 test_i2s.wav
#
# to do:
# -use a deque or something to only record voice (non-silence) events
# - OSC to mute, set gain.
# -alternately, fix gain with alsamixer

import RPi.GPIO as GPIO
import time
import alsaaudio
import webrtcvad
import logging
import sys
import os
import wave
import audioop

# import array
from datetime import datetime 
from collections import deque


class Capture(object):

	# VAD_SAMPLERATE = 16000
	VAD_SAMPLERATE = 48000
	
	ALSA_CARD = 'monocard48_32'
	# ALSA_CARD = 'monocard48_16'
	# ALSA_CARD = 'monocard'
	# ALSA_FORMAT = alsaaudio.PCM_FORMAT_S16_LE
	ALSA_FORMAT = alsaaudio.PCM_FORMAT_S32_LE
	
	VAD_FRAME_MS = 30
	VAD_PERIOD = int((VAD_SAMPLERATE / 1000) * VAD_FRAME_MS)


	# how much audio to capture before a Voice event
	VAD_PREROLL_TIME = 1000 # in ms

	# how much silence to capture after a Voice event
	VAD_SILENCE_TIMEOUT = 1000 # in ms
	
	# how much audio to throw away at mic power on (500ms)
	VAD_THROWAWAY_FRAMES = 500 / VAD_FRAME_MS # in frames

		
	SAMPLE_WIDTH = 2
	if ALSA_FORMAT == alsaaudio.PCM_FORMAT_S32_LE:
		SAMPLE_WIDTH = 4

	# compensate for mic sensitivity
	GLOBAL_SCALE = 16

	_vad = None
	_config = None
	_tmp_path = None

	inp = None

	def __init__(self, tmp_path):
		self._tmp_path = tmp_path
		self._input_device = self.ALSA_CARD


	def setup(self):
		# set VAD aggressiveness (1-3) 3 is most aggressive about filtering out non-speech
		self._vad = webrtcvad.Vad(3)


	def start(self):

		# enable reading microphone raw data
		# self.inp = alsaaudio.PCM(alsaaudio.PCM_CAPTURE, alsaaudio.PCM_NONBLOCK, self._input_device)

		self.inp = alsaaudio.PCM(alsaaudio.PCM_CAPTURE, alsaaudio.PCM_NORMAL, self._input_device)
		self.inp.setchannels(1)
		self.inp.setrate(self.VAD_SAMPLERATE)
		self.inp.setformat(self.ALSA_FORMAT)
		self.inp.setperiodsize(self.VAD_PERIOD)


		# read and ignore startup frames
		frames = 0
		while frames < self.VAD_THROWAWAY_FRAMES:
		  	length, data = self.inp.read()
		  	if length > 0:
			  	frames += 1


	def stop(self):
		self.inp.close()


	def silence_listener(self):

		start = time.time()

		PREROLL_BUFFER_SIZE = self.VAD_PREROLL_TIME / 1000 * self.VAD_SAMPLERATE * self.SAMPLE_WIDTH

		preroll = deque(maxlen=PREROLL_BUFFER_SIZE)
		speechaudio = deque()
		# speechaudio = b''

		# Buffer as long as we haven't heard enough silence or the total size is within max size
		thresholdSilenceMet = False
		frames = 0
		numSilenceRuns = 0
		silenceRun = 0
		speechStarted = False
		
		print "listening...",

		while thresholdSilenceMet is False:
		
			# length, data = self.inp.read()
			# if length:

			# 	yield data
			
			length, data = self.inp.read()
			
			if length > 0:

				data = audioop.mul(data, self.SAMPLE_WIDTH, self.GLOBAL_SCALE)

				if speechStarted:
						# speechaudio += data # speech detected
						for d in data:
							speechaudio.append(d)
				else:
					# TODO: faster way to do this
					for d in data:
						preroll.append(d)

				if length == self.VAD_PERIOD:
					# change bitdepth if necessary
					if self.SAMPLE_WIDTH > 2:
						# convert from S32_LE to S16_LE
						vad_data = audioop.lin2lin(data, self.SAMPLE_WIDTH, 2)
						isSpeech = self._vad.is_speech(vad_data, self.VAD_SAMPLERATE)
					else:
						isSpeech = self._vad.is_speech(data, self.VAD_SAMPLERATE)

					# isSpeech = self._vad.is_speech(data, self.VAD_SAMPLERATE)

					if not isSpeech:
						silenceRun += 1
						if speechStarted:
							sys.stdout.write("-")
					else:
						silenceRun = 0
						numSilenceRuns += 1
						if not speechStarted:
							GPIO.output(23, GPIO.HIGH)						
							speechStarted = True
						sys.stdout.write("*")

					sys.stdout.flush()
				
				# only count silence runs after the first one
				# (allow user to speak for total of max recording length if they haven't said anything yet)
				if (numSilenceRuns != 0) and ((silenceRun * self.VAD_FRAME_MS) > self.VAD_SILENCE_TIMEOUT):
					thresholdSilenceMet = True

		if speechStarted:
			GPIO.output(23, GPIO.LOW)						
		print " done.",

		if numSilenceRuns > 0:
			
			# setup file output
			filename = datetime.now().strftime('%H%M%S.wav')
			directory = os.path.join(self._tmp_path, datetime.now().strftime('%Y%m%d'))
			if not os.path.exists(directory):
				os.makedirs(directory)
			filename = os.path.join(directory,filename)

			# open file
			wav_output = wave.open(filename, 'w')
			wav_output.setparams((1, self.SAMPLE_WIDTH, self.VAD_SAMPLERATE, 0, 'NONE', 'not compressed'))

			# write audio data
			wav_output.writeframes(''.join(preroll))
			wav_output.writeframes(''.join(speechaudio))
			# wav_output.writeframes(speechaudio)
			wav_output.close()

			print "detected speech."
			print "wrote to:", filename
		else:
			print "all silence"


if __name__ == '__main__':

	# GPIO setup
	GPIO.setmode(GPIO.BCM)
	GPIO.setwarnings(False)
	GPIO.setup(23, GPIO.OUT)

	# hello flash
	for x in range(0,3):
		GPIO.output(23, GPIO.HIGH)
		time.sleep(0.1)
		GPIO.output(23, GPIO.LOW)
		time.sleep(0.1)

	# print "available pcms:", alsaaudio.pcms()

	# init capture
	# capture = Capture('/home/pi/housemachine/data/vad', 'plughw:0,0') 
	# capture = Capture('/home/pi/housemachine/data/vad', 'plughw:1,0') 
	# capture = Capture('/home/pi/housemachine/data/vad', 'hw:1,0') 
	# capture = Capture('/home/pi/housemachine/data/vad', 'sysdefault') 
	# capture = Capture('/home/pi/housemachine/data/vad', 'monocard') 
	capture = Capture('/home/pi/housemachine/data/vad', 'monocard48_32') 
	# capture = Capture('/home/pi/housemachine/data/vad', 'monocard48_16') 
	# capture = Capture('/home/pi/housemachine/data/vad') 

	# setup VAD
	capture.setup()

	# listen loop
	capture.start()
	try:

		while True:
			capture.silence_listener()

	except (KeyboardInterrupt):
		print "(exiting...)"
		pass
	
	capture.stop()


# ~/.asoundrc for 16K S16_LE:

"""
pcm.monocard {
   type plug
   slave.pcm "plughw:1,0"
   slave.channels  2
   slave.rate 16000
 }

 ctl.monocard {
   type hw
   card 1
   device 0
 }

 pcm.!default {
	type asym
	playback.pcm {
	   type hw
	   card 0
	}
	capture.pcm {
	   type plug
	   slave.pcm "monocard"
	}
 }

 ctl.!default {
	type asym
	playback.pcm {
	   type hw
	   card 0
	}
	capture.pcm {
	   type plug
	   slave.pcm "monocard"
	}
 }
 
"""


# ~/.asoundrc for 48k S32_LE
"""
pcm.monocard {
 type plug
 slave.pcm "plughw:1,0"
 slave.channels  2
 slave.rate 16000
}

pcm.monocard48_32 {
 type plug
 slave.pcm "plughw:1,0"
 slave.channels  2
 slave.rate 48000
 slave.format S32_LE
}

pcm.monocard48_16 {
 type plug
 slave.pcm "plughw:1,0"
 slave.channels  2
 slave.rate 48000
}

ctl.monocard {
 type hw
 card 1
 device 0
}

pcm.!default {
  type asym
  playback.pcm {
     type hw
     card 0
  }
  capture.pcm {
     type plug
     slave.pcm "monocard"
  }
}

ctl.!default {
  type asym
  playback.pcm {
     type hw
     card 0
  }
  capture.pcm {
     type plug
     slave.pcm "monocard"
  }
}
"""
# 