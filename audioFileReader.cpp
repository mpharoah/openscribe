#include "audioFileReader.hpp"

#include <stdexcept>

static const unsigned NO_REQUEST = 0xffffffff;

AudioFileReader::AudioFileReader(const char *fname, unsigned maxRequestMilliseconds, unsigned maxRememberSeconds, unsigned maxPreloadSeconds) {
	alive = true;
	error = 0;

	size_t chars = std::strlen(fname) + 1;
	filename = new char[chars];
	std::memcpy(filename, fname, chars);

	audioFile = sox_open_read(fname, NULL, NULL, NULL);

	if (audioFile == NULL || audioFile->encoding.encoding == SOX_ENCODING_UNKNOWN) {
		throw std::invalid_argument("Error: Unable to decode audio. Either the file is corrupt or you have not installed the codecs required to play it. Install the libsox-fmt-all package, then restart OpenScribe and try again.");
	} else if (!audioFile->seekable) {
		throw std::invalid_argument("Error: Sox is unable to seek in this file. Aborting.");
	}

	fileInfo.sampleRate = (unsigned) audioFile->signal.rate;
	fileInfo.numChannels = audioFile->signal.channels;
	fileInfo.numSamples = audioFile->signal.length;

	if (fileInfo.numSamples >= 0xffffffff) {
		throw std::invalid_argument("Error: File is too big! OpenScribe cannot read audio files with more than 4GB of samples. What are you even trying to play?!?");
	}

	MAX_REQUEST = (unsigned) ((double) maxRequestMilliseconds * (double) fileInfo.sampleRate * (double) fileInfo.numChannels / 1000.0 + 0.5);
	MAX_REQUEST -= MAX_REQUEST % fileInfo.numChannels;

	if (MAX_REQUEST == 0) {
		throw std::invalid_argument("Error: Sample rate is invalid or could not be determined.");
	}

	MAX_PRE = maxRememberSeconds * fileInfo.sampleRate * fileInfo.numChannels;
	MAX_POST = MAX_REQUEST + maxPreloadSeconds * fileInfo.sampleRate * fileInfo.numChannels;
	BUFFER_SIZE = MAX_PRE + MAX_POST;

	circleBuffer = new float[BUFFER_SIZE + MAX_REQUEST];
	toConvert = new int[MAX_REQUEST];
	nil = new float[MAX_REQUEST];
	std::memset(nil, 0, MAX_REQUEST * sizeof(float));

	preValid = postValid = pos = 0;
	requestingReset = NO_REQUEST;

	readerThread = new std::thread(&AudioFileReader::preloaderLoop, this);
	audioStretcher = new AudioStretcher(this);
}

AudioFileReader::~AudioFileReader() {
	accessLock.lock();
	alive = false;
	accessLock.unlock();
	bufferMoved.notify_all();
	readerThread->join();

	delete readerThread;
	delete[] circleBuffer;
	delete[] toConvert;
	delete[] nil;
	delete audioStretcher;
	delete[] filename;

	sox_close(audioFile);
}

const void *AudioFileReader::readData(unsigned at, size_t numBytes) {
	assert(numBytes % sizeof(float) == 0);
	register const size_t request = numBytes / sizeof(float);
	if (at >= fileInfo.numSamples || !alive) return nil;
	std::unique_lock<std::mutex> thisAccess(accessLock);
	if (at >= preValid && (at + request <= postValid || (at + request > fileInfo.numSamples && postValid == fileInfo.numSamples))) {
		//We already have the data ready in the buffer. Nothing needs to be done
	} else if (at >= preValid && at <= postValid && postValid + request <= pos + MAX_POST) {
		//The requested data is being read right now. Just wait for it.
		readRequest.wait(thisAccess);
	} else {
		//We don't have the data yet, and it's not being read at this instant
		requestingReset = at;
		bufferMoved.notify_all();
		resetRequest.wait(thisAccess);
	}

	pos = at + request;
	thisAccess.unlock();
	bufferMoved.notify_all();
	return (void*) &circleBuffer[at % BUFFER_SIZE];
}

HOT void AudioFileReader::preloaderLoop() {
	unsigned head = 0;
	while (alive) {
		std::unique_lock<std::mutex> thisAccess(accessLock);

		//wait until we can read more data without overwriting what we want to keep or until a reset is requested
		while (alive && requestingReset == NO_REQUEST && (postValid + MAX_REQUEST > pos + MAX_POST || postValid == fileInfo.numSamples)) bufferMoved.wait(thisAccess);
		if (!alive) break;

		//handle reset requests
		if (requestingReset != NO_REQUEST) {
			preValid = postValid = pos = requestingReset;
			readInto(requestingReset % BUFFER_SIZE, head);
			postValid += MAX_REQUEST;

			requestingReset = NO_REQUEST;
			thisAccess.unlock();
			resetRequest.notify_all();
			continue;
		}

		//read data into the buffer
		if (postValid + MAX_REQUEST > preValid + BUFFER_SIZE) {
			preValid = postValid + MAX_REQUEST - BUFFER_SIZE;
		}
		thisAccess.unlock();

		unsigned i = postValid % BUFFER_SIZE;
		if (postValid >= fileInfo.numSamples) {
			std::memset((void*) &circleBuffer[i], 0, MAX_REQUEST * sizeof(float));
		} else {
			readInto(i, head);
		}

		thisAccess.lock();
		postValid += MAX_REQUEST;
		if (postValid > fileInfo.numSamples) postValid = fileInfo.numSamples;
		thisAccess.unlock();

		readRequest.notify_all();
	}
}

HOT void AudioFileReader::readInto(unsigned index, unsigned &head) {
	bool retry = false;
	unsigned read = 0;
	do {
		if (head != postValid) {
			//file head is not where we want to read- seek to it
			sox_seek(audioFile, postValid, SOX_SEEK_SET);
			head = postValid;
		}
		read = sox_read(audioFile, toConvert, MAX_REQUEST);

		//SoX reads in signed 32-bit integer format, but I want floating point format. Convert it.
		for (unsigned i = 0; i < read; i++) circleBuffer[index + i] = (float) ((double) toConvert[i] / (double) 0x80000000);

		head += read;
		if (head > fileInfo.numSamples) head = fileInfo.numSamples;

		/*
		 * When seeking to or from the end of an audio file, SoX will stop reading data from the file.
		 * Until I find a better library, I'll work around this by closing the file and opening it again
		 * If we immediately fail again, then an actual error occurred.
		 */
		if (read == 0 && postValid < fileInfo.numSamples) {
			if (retry) {
				//Okay, something actually went wrong here
				error = 1;
				alive = false;
				std::memset((void*) &circleBuffer[index], 0, MAX_REQUEST * sizeof(float));
				return;
			}

			// "have you tried turning it off and on again?"
			sox_close(audioFile);
			audioFile = sox_open_read(filename, NULL, NULL, NULL);
			sox_seek(audioFile, postValid, SOX_SEEK_SET);

			head = postValid;
			retry = true;
		} else break;
	} while (true);

	if (read < MAX_REQUEST) std::memset((void*) &circleBuffer[index + read], 0, (MAX_REQUEST - read)*sizeof(float));

	// wrap around
	if (index + MAX_REQUEST > BUFFER_SIZE) {
		// end -> start
		std::memcpy((void*) circleBuffer, (void*) &circleBuffer[BUFFER_SIZE], (index + MAX_REQUEST - BUFFER_SIZE)*sizeof(float));
	} else if (index < MAX_REQUEST) {
		// start -> end
		std::memcpy((void*) &circleBuffer[BUFFER_SIZE + index], (void*) &circleBuffer[index], (MAX_REQUEST - index)*sizeof(float));
	}
}
