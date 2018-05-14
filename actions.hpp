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
