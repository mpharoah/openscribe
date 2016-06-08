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

#ifndef DICTATION_HPP_
#define DICTATION_HPP_

#include "audioFileReader.hpp"
#include "config.hpp"

#include <mutex>

extern "C" {
#include <pulse/stream.h>
#include <pulse/sample.h>
#include <pulse/def.h>
#include <pulse/mainloop.h>
}

class Dictation {
  private:
	AudioFileReader *reader;
	void (*onReaderError)(int);
	std::mutex errorLock;

	unsigned short RWD_SPEED;
	unsigned short FFWD_SPEED;
	bool SFX;

	unsigned BUFFER_FRAMES;

	float *SFX_RWD;
	float *SFX_FFWD;

	std::thread *loopThread;
	pa_stream *audioStream;
	pa_mainloop *paLoop;
	pa_context *paContext;

	unsigned position;
	float slowSpeed;
	bool paused;
	bool slowed;

	char *fileName;

	mutable std::mutex readLock;
	std::mutex writeLock;
	std::condition_variable pauseWait;

	enum PACKED {
		REWIND,
		NORMAL,
		FAST_FORWARD
	} mode;

	HOT static void fetchAudioData(pa_stream *stream, size_t bytes, void *myself);
	void genFX();

	HOT void mainloop();

  public:
	INLINE Dictation() :
		reader(NULL), RWD_SPEED(8), FFWD_SPEED(8), SFX(true), SFX_RWD(NULL), SFX_FFWD(NULL),
		loopThread(NULL), position(0), slowSpeed(0.5f), paused(true), slowed(false), fileName(NULL), mode(NORMAL) { onReaderError = NULL; }
	INLINE ~Dictation() { closeFile(); }

	INLINE void connectErrorHandler(void (*errorHandler)(int)) {
		errorLock.lock();
		onReaderError = errorHandler;
		errorLock.unlock();
	}

	void openFile(const char *fname, const Options &opt);
	void closeFile();
	USERET INLINE bool isFileOpen() const { return(reader != NULL); }
	void getFilename(char **dest) const;

	/*
	 * I could modify AudioFileReader to allow for chunkSize, historySize, and preloadSize
	 * to be changed without restarting the dictation, but this function is only called
	 * when the user closes the options window, so I'll take the easy route and just close
	 * and re-open the file
	 */
	INLINE void setOptions(const Options &opt) {
		readLock.lock();

		if (fileName == NULL) {
			readLock.unlock();
			return;
		}

		const size_t fnl = std::strlen(fileName)+1;
		char prevName[fnl];
		std::memcpy(prevName, fileName, fnl);

		unsigned bookmark = position;
		bool wasPaused = paused;

		readLock.unlock();

		closeFile();
		openFile(prevName, opt);

		writeLock.lock();
		readLock.lock();

		position = bookmark;
		paused = wasPaused;

		readLock.unlock();
		writeLock.unlock();

	}

	void setSlowSpeed(float v);
	float increaseSlowSpeed(float dv); // increases/decreases slow speed and returns the new speed
	void setPositionMilliseconds(unsigned ms);
	void setPositionPercentage(double p);
	void skipForward(int ms);
	INLINE void skipBack(int ms) { skipForward(-ms); }

	INLINE void play() {
		writeLock.lock();
		readLock.lock();
		paused = false;
		readLock.unlock();
		writeLock.unlock();
		pauseWait.notify_all();
	}
	INLINE void pause() {
		writeLock.lock();
		readLock.lock();
		paused = true;
		readLock.unlock();
		writeLock.unlock();
	}
	INLINE void togglePlay() {
		writeLock.lock();
		readLock.lock();
		paused = !paused;
		readLock.unlock();
		writeLock.unlock();
		pauseWait.notify_all();
	}

	INLINE void slow() {
		writeLock.lock();
		readLock.lock();
		slowed = true;
		readLock.unlock();
		writeLock.unlock();
	}
	INLINE void unslow() {
		writeLock.lock();
		readLock.lock();
		slowed = false;
		readLock.unlock();
		writeLock.unlock();
	}
	INLINE void toggleSlow() {
		writeLock.lock();
		readLock.lock();
		slowed = !slowed;
		readLock.unlock();
		writeLock.unlock();
	}

	INLINE void startRewind() {
		writeLock.lock();
		readLock.lock();
		mode = REWIND;
		readLock.unlock();
		writeLock.unlock();
		pauseWait.notify_all();
	}
	INLINE void stopRewind() {
		writeLock.lock();
		readLock.lock();
		if (mode == REWIND) mode = NORMAL;
		readLock.unlock();
		writeLock.unlock();
	}
	INLINE void toggleRewind() {
		writeLock.lock();
		readLock.lock();
		mode = (mode == REWIND) ? NORMAL : REWIND;
		readLock.unlock();
		writeLock.unlock();
		pauseWait.notify_all();
	}


	INLINE void startFastForward() {
		writeLock.lock();
		readLock.lock();
		mode = FAST_FORWARD;
		readLock.unlock();
		writeLock.unlock();
		pauseWait.notify_all();
	}
	INLINE void stopFastForward() {
		writeLock.lock();
		readLock.lock();
		if (mode == FAST_FORWARD) mode = NORMAL;
		readLock.unlock();
		writeLock.unlock();
	}
	INLINE void toggleFastForward() {
		writeLock.lock();
		readLock.lock();
		mode = (mode == FAST_FORWARD) ? NORMAL : FAST_FORWARD;
		readLock.unlock();
		writeLock.unlock();
		pauseWait.notify_all();
	}

	USERET INLINE bool isPaused() const {
		std::unique_lock<std::mutex> rLock(readLock);
		return paused;
	}
	USERET INLINE bool isSlowed() const {
		std::unique_lock<std::mutex> rLock(readLock);
		return slowed;
	}
	USERET INLINE bool isRewinding() const {
		std::unique_lock<std::mutex> rLock(readLock);
		return (mode == REWIND);
	}
	USERET INLINE bool isFastForwarding() const {
		std::unique_lock<std::mutex> rLock(readLock);
		return (mode == FAST_FORWARD);
	}

	USERET INLINE unsigned getPositionMilliseconds() const {
		std::unique_lock<std::mutex> rLock(readLock);
		if (reader == NULL) return 0;
		return ((unsigned) ((double) 1000 * ((double) position / (double) (reader->getFileInfo().sampleRate * reader->getFileInfo().numChannels))));
	}

	USERET INLINE unsigned getLengthMilliseconds() const {
		std::unique_lock<std::mutex> rLock(readLock);
		if (reader == NULL) return 0;
		return((unsigned)(1000.0 * ((double)reader->getFileInfo().numSamples / (double)(reader->getFileInfo().sampleRate * reader->getFileInfo().numChannels))));
	}

	USERET INLINE double getPositionPercentage() const {
		std::unique_lock<std::mutex> rLock(readLock);
		if (reader == NULL) return 0;
		return (((double) position) / (double) reader->getFileInfo().numSamples);
	}



};


#endif /* DICTATION_HPP_ */
