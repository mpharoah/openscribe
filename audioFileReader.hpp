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

#ifndef AUDIOFILEREADER_HPP_
#define AUDIOFILEREADER_HPP_

#include <cstring>
#include <cassert>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include <sonic.h>
#include <sox.h>

#include "attributes.hpp"

struct PACKED AudioFileInfo {
	unsigned sampleRate;
	unsigned numChannels;
	size_t numSamples;
};

class AudioFileReader {
  private:
	std::atomic<bool> alive;
	int error;

	sox_format_t *audioFile;
	AudioFileInfo fileInfo;
	char *filename;

	unsigned MAX_REQUEST;
	unsigned MAX_PRE;
	unsigned MAX_POST;
	unsigned BUFFER_SIZE;

	float *circleBuffer;
	int *toConvert;
	float *nil;

	unsigned preValid;
	unsigned postValid;
	unsigned pos;

	std::thread *readerThread;
	std::mutex accessLock;
	std::condition_variable bufferMoved;
	std::condition_variable readRequest;
	std::condition_variable resetRequest;
	unsigned requestingReset;

	HOT void preloaderLoop();
	HOT void readInto(unsigned index, unsigned &head);

  public:
	AudioFileReader(const char *fname, unsigned maxRequestMilliseconds, unsigned maxRememberSeconds, unsigned maxPreloadSeconds);
	~AudioFileReader();

	USERET INLINE const AudioFileInfo &getFileInfo() const { return fileInfo; }

	USERET bool loadFile(char *fname, unsigned maxRequestMilliseconds, unsigned maxRememberSeconds, unsigned maxPreloadSeconds);
	size_t setBufferSettings(unsigned maxRequestMilliseconds, unsigned maxRememberSeconds, unsigned maxPreloadSeconds);

	USERET size_t getMaxRequestBytes() const { return(sizeof(float) * MAX_REQUEST); }

	USERET const void *readData(unsigned position, size_t numBytes);
	INLINE void copyData(void *dest, unsigned position, size_t numBytes) { std::memcpy(dest, readData(position, numBytes), numBytes); }

	INLINE USERET bool isAlive() const { return alive; }
	INLINE USERET int err() const { return error; }

	INLINE void kill() { alive = false; }

	class AudioStretcher {
	  private:
		AudioFileReader *const reader;
		sonicStream stretcher;
		unsigned inPos;
		unsigned outPos;
		float speed;
		const size_t bufferSize; //Uses a 3 second buffer

		void trashStreamData() {
			sonicFlushStream(stretcher);
			const register unsigned size = sonicSamplesAvailable(stretcher);
			float *trash = new float[size * reader->fileInfo.numChannels];
			sonicReadFloatFromStream(stretcher, trash, size);
			delete[] trash;
		}

	  public:
		INLINE AudioStretcher(AudioFileReader *fileReader) : reader(fileReader), bufferSize(fileReader->fileInfo.sampleRate * fileReader->fileInfo.numChannels * 3) {
			inPos = 0xffffffff;
			outPos = 0xffffffff;
			speed = 0.5f;
			stretcher = sonicCreateStream(reader->fileInfo.sampleRate, reader->fileInfo.numChannels);
			sonicSetSpeed(stretcher, speed);
		}

		INLINE unsigned copyData(void *dest, unsigned position, size_t numBytes) {
			assert(numBytes % (sizeof(float) * reader->fileInfo.numChannels) == 0);
			const size_t numSamples = numBytes / sizeof(float);
			const size_t numMultiSamples = numSamples / reader->fileInfo.numChannels;

			if (position != outPos) {
				//we've skipped to a new position, so flush the buffer
				trashStreamData();
				inPos = position;
				outPos = position;
			}

			//top off the buffer
			while (inPos < outPos + bufferSize) {
				const size_t requestMultiSamples = reader->getMaxRequestBytes() / (reader->fileInfo.numChannels * sizeof(float));
				const size_t requestBytes = requestMultiSamples * reader->fileInfo.numChannels * sizeof(float);
				
				sonicWriteFloatToStream(stretcher, (float*)reader->readData(inPos, requestBytes), requestMultiSamples);
				inPos += requestBytes / sizeof(float);
			}

			//if there aren't enough samples ready, wait for sonic to process them
			while ((unsigned) sonicSamplesAvailable(stretcher) < numMultiSamples) std::this_thread::yield();
			sonicReadFloatFromStream(stretcher, (float*)dest, numMultiSamples);
			outPos += (unsigned) ((float) numSamples * speed);
			return (unsigned) ((float) numSamples * speed);
		}

		INLINE void setSpeed(float slow) {
			trashStreamData();
			inPos = 0xffffffff;
			outPos = 0xffffffff;
			speed = slow;
			sonicSetSpeed(stretcher, speed);
		}

		INLINE ~AudioStretcher() { sonicDestroyStream(stretcher); }
	} *audioStretcher;


};


#endif /* AUDIOFILEREADER_HPP_ */
