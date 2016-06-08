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

	unsigned chunkSize;
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
