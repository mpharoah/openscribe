/*
	OpenScribe -- Copyright © 2012–2016 Matt Pharoah

	This file is part of OpenScribe.

    OpenScribe is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenScribe is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenScribe.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "dictation.hpp"

#include <cstring>
#include <cassert>

void Dictation::openFile(const char *fname, const Options &opt) {
	writeLock.lock();
	readLock.lock();

	if (reader != NULL) {
		readLock.unlock();
		writeLock.unlock();
		throw std::logic_error("Cannot open audio file. Another audio file is already open. Call Dictation::closeFile() first.");
	}

	try {
		reader = new AudioFileReader(fname, opt.chunkSize, opt.historySize, opt.preloadSize);
	} catch (const std::invalid_argument &ex) {
		readLock.unlock();
		writeLock.unlock();
		throw ex;
	}
	const size_t BUFFER_BYTES = reader->getMaxRequestBytes();
	BUFFER_FRAMES = BUFFER_BYTES / sizeof(float);

	pa_sample_spec sampleFormat;
	sampleFormat.format = PA_SAMPLE_FLOAT32LE;
	sampleFormat.rate = reader->getFileInfo().sampleRate;
	sampleFormat.channels = reader->getFileInfo().numChannels;

	readLock.unlock();
	writeLock.unlock();

	RWD_SPEED = opt.rewindSpeed;
	FFWD_SPEED = opt.fastForwardSpeed;
	SFX = opt.playSoundEffects;
	slowSpeed = opt.slowSpeed;
	genFX();

	position = 0;
	paused = true;

	const size_t fnl = std::strlen(fname)+1;
	fileName = new char[fnl];
	std::memcpy(fileName, fname, fnl);

	pa_buffer_attr bufferInfo;
	bufferInfo.maxlength = 2*BUFFER_BYTES;
	bufferInfo.tlength = BUFFER_BYTES;
	bufferInfo.minreq = (uint32_t) -1;
	bufferInfo.prebuf = BUFFER_BYTES;

	paLoop = pa_mainloop_new();
	paContext = pa_context_new(pa_mainloop_get_api(paLoop), "OpenScribe Context");
	if (paContext == NULL) throw std::runtime_error("Error connecting to PulseAudio server:\nCould not create context.\n");
	pa_context_connect(paContext, NULL, PA_CONTEXT_NOFLAGS, NULL);

	audioStream = pa_stream_new(paContext, "OpenScribe Audio Stream", &sampleFormat, NULL);
	if (audioStream == NULL) throw std::runtime_error("Error connecting to PulseAudio server:\nCould not create audio stream.\n");

	int unused;
	//wait for for the context to become ready
	while (pa_context_get_state(paContext) != PA_CONTEXT_READY) pa_mainloop_iterate(paLoop, true, &unused);

	pa_stream_connect_playback(audioStream, NULL, &bufferInfo, PA_STREAM_ADJUST_LATENCY, NULL, NULL);
	pa_stream_set_write_callback(audioStream, fetchAudioData, (void*)this);

	//wait for the stream to become ready
	while (pa_stream_get_state(audioStream) != PA_STREAM_READY) pa_mainloop_iterate(paLoop, true, &unused);

	loopThread = new std::thread(&Dictation::mainloop, this);
}

void Dictation::closeFile() {
	writeLock.lock();
	readLock.lock();
	if (reader != NULL) reader->kill();
	readLock.unlock();
	writeLock.unlock();
	pauseWait.notify_all();

	if (loopThread != NULL) {
		loopThread->join();
		delete loopThread;
		loopThread = NULL;

		pa_stream_disconnect(audioStream);
		pa_stream_unref(audioStream);

		pa_context_disconnect(paContext);
		pa_context_unref(paContext);

		pa_mainloop_free(paLoop);

		writeLock.lock();
		readLock.lock();
		delete[] fileName;
		delete[] SFX_RWD;
		delete[] SFX_FFWD;

		fileName = NULL;
		readLock.unlock();
		writeLock.unlock();
	}
}

HOT void Dictation::fetchAudioData(pa_stream *stream, size_t bytes, void *myself) {
	Dictation *me = (Dictation*) myself;

	/*
	 * The value PulseAudio provides for bytes is rubbish. It doesn't matter, though, since
	 * the buffer attributes set minreq to BUFFER_BYTES, and this is the most that audioFileReader
	 * is set to handle, so we always want to reader BUFFER_BYTES = BUFFER_FRAMES * sizeof(float) bytes.
	 *
	 * If it does happen to be too much, pa_stream_begin_write should adjust it to a lower value
	 */
	bytes = me->BUFFER_FRAMES * sizeof(float);

	void *data;
	pa_stream_begin_write(stream, &data, &bytes);

	me->writeLock.lock();
	if (me->reader == NULL || !me->reader->isAlive()) {
		std::memset(data, 0, bytes);
		pa_stream_write(stream, data, bytes, NULL, 0, PA_SEEK_RELATIVE);
		me->writeLock.unlock();
		return;
	}

	size_t request = bytes / sizeof(float);
	request -= request % me->reader->getFileInfo().numChannels;
	if (request == 0) {
		pa_stream_cancel_write(stream);
		me->writeLock.unlock();
		return;
	} else if (request > me->BUFFER_FRAMES) {
		request = me->BUFFER_FRAMES;
	}
	const size_t requestBytes = request * sizeof(float);

	if (me->position > me->reader->getFileInfo().numSamples) {
		me->readLock.lock();
		me->position = me->reader->getFileInfo().numSamples;
		me->paused = true;
		me->readLock.unlock();
	}

	if (me->mode == REWIND) {
		if (me->SFX) {
			std::memcpy(data, (void*)me->SFX_RWD, requestBytes);
		} else {
			std::memset(data, 0, requestBytes);
		}

		me->readLock.lock();
		if (me->position < request * me->RWD_SPEED) {
			me->position = 0;
		} else {
			me->position -= request * me->RWD_SPEED;
		}
		me->readLock.unlock();
	} else if (me->mode == FAST_FORWARD) {
		if (me->SFX) {
			std::memcpy(data, (void*)me->SFX_FFWD, requestBytes);
		} else {
			std::memset(data, 0, requestBytes);
		}
		me->readLock.lock();
		me->position += request * me->FFWD_SPEED;
		me->readLock.unlock();
	} else if (me->paused) {
		std::memset(data, 0, requestBytes);
	} else if (me->slowed && me->slowSpeed != 1.0f) {
		me->readLock.lock();
		me->position += me->reader->audioStretcher->copyData(data, me->position, requestBytes);
		me->readLock.unlock();
	} else {
		me->reader->copyData(data, me->position, requestBytes);
		me->readLock.lock();
		me->position += request;
		me->readLock.unlock();
	}

	me->writeLock.unlock();
	pa_stream_write(stream, data, requestBytes, NULL, 0, PA_SEEK_RELATIVE);
}

void Dictation::genFX() {
	SFX_RWD = new float[BUFFER_FRAMES];
	SFX_FFWD = new float[BUFFER_FRAMES];

	unsigned const &CHANNELS = reader->getFileInfo().numChannels;

	for (unsigned i = 0; i < BUFFER_FRAMES; i += CHANNELS) {
		for (unsigned j = 0; j < CHANNELS; j++) {
			SFX_RWD[i+j] = 0.125f * (i % 100 / 100.f - 0.5);
			SFX_FFWD[i+j] = 0.125f * (i % 80 / 80.f - 0.5);
		}
	}
}

HOT void Dictation::mainloop() {
	int unused;
	std::unique_lock<std::mutex> wLock(writeLock, std::defer_lock);
	while (true) {
		wLock.lock();
		while (paused && mode == NORMAL && reader != NULL && reader->isAlive()) pauseWait.wait(wLock);

		if (reader == NULL) {
			break;
		} else if (!reader->isAlive()) {
			if (reader->err() != 0) {
				errorLock.lock();
				if (onReaderError != NULL) onReaderError(reader->err());
				errorLock.unlock();
			}
			readLock.lock();
			delete reader;
			reader = NULL;
			readLock.unlock();
			break;
		}

		if (paused && mode == NORMAL) continue;
		wLock.unlock();

		pa_mainloop_iterate(paLoop, 1, &unused);

		//Let other threads have a turn
		std::this_thread::yield();
	}
}

void Dictation::getFilename(char **dest) const {
	readLock.lock();
	if (fileName == NULL) {
		*dest = NULL;
	} else {
		const size_t len = std::strlen(fileName) + 1;
		*dest = new char[len];
		std::memcpy(*dest, fileName, len);
	}
	readLock.unlock();
}

void Dictation::setSlowSpeed(float v) {
	if (v < 0.2) { v = 0.2; }
	else if (v > 1.0) { v = 1.0; }

	writeLock.lock();
	readLock.lock();

	if (slowSpeed != v && reader != NULL && reader->isAlive()) {
		reader->audioStretcher->setSpeed(v);
		slowSpeed = v;
	}

	readLock.unlock();
	writeLock.unlock();
}

float Dictation::increaseSlowSpeed(float dv) {
	float ret;
	writeLock.lock();
	readLock.lock();

	slowSpeed += dv;
	if (slowSpeed < 0.2) slowSpeed = 0.2;
	if (slowSpeed > 1.0) slowSpeed = 1.0;
	ret = slowSpeed;

	readLock.unlock();
	writeLock.unlock();

	return ret;
}

void Dictation::setPositionMilliseconds(unsigned ms) {
	writeLock.lock();
	readLock.lock();

	if (reader != NULL && reader->isAlive()) {
		pa_stream_flush(audioStream, NULL, NULL);
		position = (unsigned)(((double)ms/1000.0) * (double)reader->getFileInfo().sampleRate * (double)reader->getFileInfo().numChannels + 0.5);
		position -= position % reader->getFileInfo().numChannels;
	}

	readLock.unlock();
	writeLock.unlock();
}

void Dictation::setPositionPercentage(double p) {
	writeLock.lock();
	readLock.lock();

	if (p < 0.0) { p = 0.0; }
	else if (p > 1.0) { p = 1.0; }

	if (reader != NULL && reader->isAlive()) {
		pa_stream_flush(audioStream, NULL, NULL);
		position = (unsigned)(p*(double)reader->getFileInfo().numSamples + 0.5);
		position -= position % reader->getFileInfo().numChannels;
	}

	readLock.unlock();
	writeLock.unlock();
}

void Dictation::skipForward(int ms) {
	writeLock.lock();
	readLock.lock();

	if (ms == 0 || reader == NULL || !reader->isAlive()) {
		/* Do nothing */
	} else if (ms < 0) {
		const unsigned back = (unsigned) (reader->getFileInfo().sampleRate * reader->getFileInfo().numChannels * (unsigned)((double)-ms/1000.0));
		if (back >= position) {
			position = 0;
		} else {
			position -= back;
		}
	} else {
		position += (unsigned) (reader->getFileInfo().sampleRate * reader->getFileInfo().numChannels * (unsigned)((double)ms/1000.0));
		if (position > reader->getFileInfo().numSamples) {
			position = reader->getFileInfo().numSamples;
		}
	}

	readLock.unlock();
	writeLock.unlock();
}


