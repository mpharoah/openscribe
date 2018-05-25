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
	unsigned short patch;

	INLINE Version() {}
	INLINE Version(unsigned short maj, unsigned short min = 0, unsigned short pch = 0) {
		major = maj;
		minor = min;
		patch = pch;
	}
	INLINE ~Version() {}

	INLINE operator Glib::ustring() const {
		return Glib::ustring::compose("%1.%2.%3", major, minor, patch);
	}

	INLINE bool operator<(const Version &other) const {
		return (major < other.major || (major == other.major && (minor < other.minor || (minor == other.minor && patch < other.patch))));
	}
	INLINE bool operator>(const Version &other) const {
		return (major > other.major || (major == other.major && (minor > other.minor || (minor == other.minor && patch > other.patch))));
	}
	INLINE bool operator==(const Version &other) const {
		return (major == other.major && minor == other.minor && patch == other.patch);
	}
	INLINE bool operator!=(const Version &other) const {
		return (major != other.major || minor != other.minor || patch != other.patch);
	}
	INLINE bool operator<=(const Version &other) const {
		return (major < other.major || (major == other.major && (minor < other.minor || (minor == other.minor && patch <= other.patch))));
	}
	INLINE bool operator>=(const Version &other) const {
		return (major > other.major || (major == other.major && (minor > other.minor || (minor == other.minor && patch >= other.patch))));
	}
};

static const Version CURRENT_VERSION = Version{1,3,1};

std::ostream &operator<<(std::ostream &out, const Version &ver);
std::istream &operator>>(std::istream &in, Version &ver);


#endif /* VERSION_HPP_ */

