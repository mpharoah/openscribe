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

#ifndef ACTIONS_HPP_
#define ACTIONS_HPP_

#include "attributes.hpp"

struct PACKED Action {
	enum : unsigned char {
		// the following are used by both footpedal configuration and the event handler
		NOOP,
		PLAY,
		TOGGLE_PLAY,
		SLOW,
		TOGGLE_SLOW,
		FAST_FORWARD,
		TOGGLE_FAST_FORWARD,
		REWIND,
		TOGGLE_REWIND,
		SKIP,
		RESTART,
		CHANGE_SLOW_SPEED,
		// the following are only used by the event handlers- they are not stored as pedal mappings
		PAUSE,
		UNSLOW,
		STOP_FAST_FORWARD,
		STOP_REWIND,
		// the following are used in footpedal configuration, but are never passed to the event handler given to FootPedalCoordinator
		TOGGLE_MODIFIER,
		MODIFIER
	} type;
	union {
		signed char deciseconds; // for SKIP
		signed char percent; // for CHANGE_SLOW_SPEED
		signed char amount; // generic access
	};

	USERET INLINE bool operator==(const Action &other) const {
		return (type == other.type && amount == other.amount);
	}
	USERET INLINE bool operator!=(const Action &other) const {
		return !(*this == other);
	}

	INLINE static Action getReleaseAction(const Action &A) {
		// NOTE: assumes you've already handled what happens when A is MODIFIER
		Action B = A;

		switch (A.type) {
			case PLAY: B.type = PAUSE; break;
			case SLOW: B.type = UNSLOW; break;
			case FAST_FORWARD: B.type = STOP_FAST_FORWARD; break;
			case REWIND: B.type = STOP_REWIND; break;
			default: B.type = NOOP; break;
		}
		return B;
	}
};

#endif /* ACTIONS_HPP_ */
