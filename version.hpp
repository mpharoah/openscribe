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
#ifndef VERSION_HPP_
#define VERSION_HPP_

#include <glibmm/ustring.h>

#include <istream>
#include <ostream>

#include "attributes.hpp"

/* xxx remember to change CURRENT_VERSION below when updating xxx */

struct Version {
	unsigned short major;
	unsigned short minor;
	unsigned short ubuntu;

	INLINE Version() {}
	INLINE Version(unsigned short maj, unsigned short min = 0, unsigned short ub = 0) {
		major = maj;
		minor = min;
		ubuntu = ub;
	}
	INLINE ~Version() {}

	INLINE operator Glib::ustring() const {
		if (ubuntu == 0) return Glib::ustring::compose("%1.%2", major, minor);
		return Glib::ustring::compose("%1.%2-0ubuntu%3", major, minor, ubuntu);
	}

	INLINE bool operator<(const Version &other) const {
		return (major < other.major || (major == other.major && (minor < other.minor || (minor == other.minor && ubuntu < other.ubuntu))));
	}
	INLINE bool operator>(const Version &other) const {
		return (major > other.major || (major == other.major && (minor > other.minor || (minor == other.minor && ubuntu > other.ubuntu))));
	}
	INLINE bool operator==(const Version &other) const {
		return (major == other.major && minor == other.minor && ubuntu == other.ubuntu);
	}
	INLINE bool operator!=(const Version &other) const {
		return (major != other.major || minor != other.minor || ubuntu != other.ubuntu);
	}
	INLINE bool operator<=(const Version &other) const {
		return (major < other.major || (major == other.major && (minor < other.minor || (minor == other.minor && ubuntu <= other.ubuntu))));
	}
	INLINE bool operator>=(const Version &other) const {
		return (major > other.major || (major == other.major && (minor > other.minor || (minor == other.minor && ubuntu >= other.ubuntu))));
	}
};

static const Version CURRENT_VERSION = Version{1,1,2};

std::ostream &operator<<(std::ostream &out, const Version &ver);
std::istream &operator>>(std::istream &in, Version &ver);


#endif /* VERSION_HPP_ */
