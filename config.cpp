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

#include "config.hpp"

#include <ios>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>

#include "attributes.hpp"

void touchOptionsFolder() {
	// Create the ~/.config/OpenScribe folder if it does not exist
	struct stat unused;
	std::string homeDir = std::string(getenv("HOME"));

	if (stat((homeDir + "/.config").c_str(), &unused) != 0) {
		mkdir((homeDir + "/.config").c_str(), 0700);
	}

	if (stat((homeDir + "/.config/OpenScribe").c_str(), &unused) != 0) {
		mkdir((homeDir + "/.config/OpenScribe").c_str(), 0700);
	}
}

INLINE static void trim(char *str) {
	int i = -1; while (str[++i] != '\0');
	while (i > 0 && (str[--i] == ' ' || str[i] == '\t')) str[i] = '\0';
}

Options loadOptions(const Version &version) {
	std::fstream conf;
	if (version < Version(1,0,0)) { // User is upgrading from QTranscribe
		((std::string(getenv("HOME")) + "/.config/qtranscribe/settings.conf").c_str(), std::ios_base::in);
	} else {
		conf.open((std::string(getenv("HOME")) + "/.config/OpenScribe/settings.conf").c_str(), std::ios_base::in);
	}
	char line[256];

	if (!conf.is_open()) {
		try {
			saveOptions(DefaultOptions);
		} catch (const std::ios_base::failure &ex) {}
		return DefaultOptions;
	}

	Options opt = DefaultOptions;
	while (conf.good() && !conf.eof()) {
		conf.getline(line, 256, '=');
		trim(line);

		if (std::strcmp(line, "Playback Skip") == 0) {
			conf >> opt.skipBackOnPlay;
			if (version < Version(1,0,0)) { opt.skipBackOnPlay *= -1; } // QTranscribe options uses negative value
			if (!conf.good() || opt.skipBackOnPlay > 10000) opt.skipBackOnPlay = DefaultOptions.skipBackOnPlay;
		} else if (std::strcmp(line, "Rewind Speed") == 0) {
			conf >> opt.rewindSpeed;
			if (!conf.good() || opt.rewindSpeed > 64 || opt.rewindSpeed < 1) opt.rewindSpeed = DefaultOptions.rewindSpeed;
		} else if (std::strcmp(line, "Fast Forward Speed") == 0) {
			conf >> opt.fastForwardSpeed;
			if (!conf.good() || opt.fastForwardSpeed > 64 || opt.fastForwardSpeed < 2) opt.fastForwardSpeed = DefaultOptions.fastForwardSpeed;
		} else if (std::strcmp(line, "Slow Speed") == 0) {
			conf >> opt.slowSpeed;
			if (!conf.good() || opt.slowSpeed < 0.2 || opt.slowSpeed > 1.0) opt.slowSpeed = DefaultOptions.slowSpeed;
		} else if (std::strcmp(line, "Rewind / Fast Forward Sound Effects") == 0) {
			conf >> opt.playSoundEffects;
			if (!conf.good()) opt.playSoundEffects = DefaultOptions.playSoundEffects;
		} else if (std::strcmp(line, "Chunk Size") == 0) {
			conf >> opt.chunkSize;
			if (!conf.good() || opt.chunkSize < 10 || opt.chunkSize > 60) opt.chunkSize = DefaultOptions.chunkSize;
		} else if (std::strcmp(line, "Buffer Remember Size") == 0) {
			conf >> opt.historySize;
			if (!conf.good() || opt.historySize > 13 || opt.historySize == 0) opt.historySize = DefaultOptions.historySize;
		} else if (std::strcmp(line, "Buffer Preprocess Size") == 0) {
			conf >> opt.preloadSize;
			if (!conf.good() || opt.preloadSize > 13 || opt.preloadSize == 0) opt.preloadSize = DefaultOptions.preloadSize;
		}
		conf.getline(line, 256, '\n');
	}
	conf.close();

	bool optionsChanged = false;

	if (version < Version(1,0,0)) {
		/* When upgrading from QTranscribe, reset the advanced options to their new defaults */
		opt.chunkSize = DefaultOptions.chunkSize;
		opt.historySize = DefaultOptions.historySize;
		opt.preloadSize = DefaultOptions.preloadSize;
		optionsChanged = true;
	}

	if (version < Version(1,1,2)) {
		/* Version 1.1-2 changed default chunk size from 15ms to 25ms */
		if (opt.chunkSize == 15) opt.chunkSize = 25;
		optionsChanged = true;
	}

	if (optionsChanged) saveOptions(opt);

	return opt;
}

void saveOptions(const Options &opt) {
	touchOptionsFolder();
	std::fstream conf((std::string(getenv("HOME")) + "/.config/OpenScribe/settings.conf").c_str(), std::ios_base::out);
	if (!conf.is_open()) throw std::ios_base::failure("Error: Could not save settings to ~/.config/OpenScribe/settings.conf");

	conf << "Rewind / Fast Forward Sound Effects = " << opt.playSoundEffects << std::endl;
	conf << "Playback Skip = " << opt.skipBackOnPlay << std::endl;
	conf << "Rewind Speed = " << opt.rewindSpeed << std::endl;
	conf << "Fast Forward Speed = " << opt.fastForwardSpeed << std::endl;
	conf << "Slow Speed = " << opt.slowSpeed << std::endl;
	conf << "Chunk Size = " << opt.chunkSize << std::endl;
	conf << "Buffer Remember Size = " << opt.historySize << std::endl;
	conf << "Buffer Preprocess Size = " << opt.preloadSize << std::endl;
	conf.close();
}

Version getLastVersionUsed() {
	std::fstream vfile((std::string(getenv("HOME")) + "/.config/OpenScribe/version").c_str(), std::ios_base::in);
	if (!vfile.is_open()) return Version(0);
	Version version;
	vfile >> version;
	if (!vfile.good()) return Version(0);
	return version;
}
void saveVersionFile(const Version &version) {
	touchOptionsFolder();
	std::fstream vfile((std::string(getenv("HOME")) + "/.config/OpenScribe/version").c_str(), std::ios_base::out);
	if (!vfile.is_open()) throw std::ios_base::failure("Error: Could not save version file");

	vfile << version << std::endl;
	vfile.close();
}

bool QTranscribeInstalled() {
	struct stat unused;
	return (stat((std::string(getenv("HOME")) + "/.config/qtranscribe").c_str(), &unused) == 0);
}
