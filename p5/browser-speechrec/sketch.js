function setup() {
	noCanvas();
	let lang = navigator.language || 'en-US';
	let speechRec = new p5.SpeechRec(lang, gotSpeech);

	let continuous = true;
	let interim = false;
	speechRec.start(continuous, interim);

	function gotSpeech() {
		if (speechRec.resultValue) {
			createP(speechRec.resultString);
		}
	}
}


// hello does this work

// add some more text

// continuous speech

// Jasper come here

// wow this is amazing

// Yakima

// yeah come look

// Hialeah

// Gina what is this

// Nina

// Nina is my dog's name

// web speech API but look at how little code it takes

// that's it

// questions could do that in a second