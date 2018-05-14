#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#include <istream>
#include <ostream>

#include "version.hpp"

struct Options {
	unsigned short rewindSpeed;
	unsigned short fastForwardSpeed;
	bool playSoundEffects;

	unsigned skipBackOnPlay;
	float slowSpeed;

	unsigned latency;
	unsigned historySize;
	unsigned preloadSize;
};

const Options DefaultOptions{ 8, 8, true, 1000, 0.5f, 25, 6, 2 };

void touchOptionsFolder();

Options loadOptions(const Version &version = CURRENT_VERSION);
void saveOptions(const Options &opt);

Version getLastVersionUsed();
void saveVersionFile(const Version &version);

// Functions for users upgrading from QTranscribe
bool QTranscribeInstalled(); //More accurately, are the configuration files still there?

#endif /* CONFIG_HPP_ */
