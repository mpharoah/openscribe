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

#include "version.hpp"

#include <stdexcept>
#include <cstring>

std::ostream &operator<<(std::ostream &out, const Version &ver) {
	out << ver.major << '.' << ver.minor << '.' << ver.patch;
	return out;
}

std::istream &operator>>(std::istream &in, Version &ver) {
	std::string vstring;
	in >> vstring;

	vstring = vstring.substr(vstring.find_first_not_of(" \n\t"));
	vstring = vstring.substr(0, vstring.find_last_not_of(" \n\t")+1);
	
	ver.major = 0;
	ver.minor = 0;
	ver.patch = 0;

	if (vstring.size() == 0) {
		in.setstate(std::istream::failbit);
		return in;
	}

	unsigned long major, minor, patch;

	size_t dot = vstring.find_first_of('.');
	try {
		major = std::stoul(vstring.substr(0, dot));
	} catch (const std::invalid_argument&) {
		in.setstate(std::istream::failbit);
		return in;
	} catch (const std::out_of_range&) {
		in.setstate(std::istream::failbit);
		return in;
	}

	if (major > 0xffff) {
		in.setstate(std::istream::failbit);
		return in;
	}

	ver.major = (unsigned short) major;
	if (dot == std::string::npos) {
		ver.minor = 0;
		ver.patch = 0;
		return in;
	}

	vstring = vstring.substr(dot+1);
	if (vstring.size() == 0) {
		in.setstate(std::istream::failbit);
		return in;
	}

	dot = vstring.find_first_of(".-");
	try {
		minor = std::stoul(vstring.substr(0, dot));
	} catch (const std::invalid_argument&) {
		in.setstate(std::istream::failbit);
		return in;
	} catch (const std::out_of_range&) {
		in.setstate(std::istream::failbit);
		return in;
	}

	if (minor > 0xffff) {
		in.setstate(std::istream::failbit);
		return in;
	}

	ver.minor = (unsigned short) minor;
	if (dot == std::string::npos) {
		ver.patch = 0;
		return in;
	}

	vstring = vstring.substr(dot+1);
	if (vstring.find("0ubuntu") == 0) {
		//Compatability with earlier version numbering scheme
		vstring = vstring.substr(7);
	}
	
	if (vstring.size() == 0) {
		in.setstate(std::istream::failbit);
		return in;
	}

	try {
		patch = std::stoul(vstring);
	} catch (const std::invalid_argument&) {
		in.setstate(std::istream::failbit);
		return in;
	} catch (const std::out_of_range&) {
		in.setstate(std::istream::failbit);
		return in;
	}

	if (patch > 0xffff) {
		in.setstate(std::istream::failbit);
		return in;
	}

	ver.patch = (unsigned short)patch;
	
	return in;
}

