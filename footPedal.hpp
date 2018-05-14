#ifndef FOOTPEDAL_HPP_
#define FOOTPEDAL_HPP_

#include <thread>
#include <atomic>
#include <mutex>
#include <map>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <cassert>
#include<map>

#include "actions.hpp"
#include "attributes.hpp"

/* Struct for raw input */
struct FPEvent {
	timeval timestamp;
	unsigned short type;
	unsigned short code;
	int value;
};

/* Struct passed to event handler through onPedalEvent() when in configuration mode */
struct PedalEvent {
	static constexpr bool TYPE_BUTTON = false;
	static constexpr bool TYPE_AXIS = true;

	bool type;
	unsigned short index;
	union {
		int isPressed; // button event
		float position; // axis event
	};
};

struct PedalInfo {
	char *name;
	bool isProtected;
	std::map<unsigned short, unsigned short> buttons;
	std::map<unsigned short, unsigned short> axes;

	int *axisMin;
	int *axisMax;

	USERET INLINE unsigned short getNumButtons() const { return (unsigned short)buttons.size(); }
	USERET INLINE unsigned short getNumAxes() const { return (unsigned short)axes.size(); }

	INLINE PedalInfo &operator=(const PedalInfo &other) {
		if (name != NULL) delete[] name;
		if (axisMin != NULL) delete[] axisMin;
		if (axisMax != NULL) delete[] axisMax;

		if (other.name == NULL) {
			name = NULL;
		} else {
			size_t len = std::strlen(other.name) + 1;
			name = new char[len];
			std::memcpy(name, other.name, len);
		}

		if (other.axisMin == NULL) {
			axisMin = NULL;
		} else {
			axisMin = new int[other.axes.size()];
			std::memcpy(axisMin, other.axisMin, sizeof(int) * other.axes.size());
		}

		if (other.axisMax == NULL) {
			axisMax = NULL;
		} else {
			axisMax = new int[other.axes.size()];
			std::memcpy(axisMax, other.axisMax, sizeof(int) * other.axes.size());
		}

		buttons = other.buttons;
		axes = other.axes;

		isProtected = other.isProtected;

		return *this;
	}

	INLINE bool operator==(const PedalInfo &other) const {
		return (isProtected == other.isProtected && buttons.size() == other.buttons.size() && axes.size() == other.axes.size() && std::strcmp(name, other.name) == 0);
	}
	INLINE bool operator!=(const PedalInfo &other) const {
		return !(*this == other);
	}

	INLINE PedalInfo() : name(NULL), isProtected(false), axisMin(NULL), axisMax(NULL) {}
	INLINE PedalInfo(const PedalInfo &other) : name(NULL), axisMin(NULL), axisMax(NULL) { *this = other; }
	~PedalInfo() {
		if (name != NULL) delete[] name;
		if (axisMin != NULL) delete[] axisMin;
		if (axisMax != NULL) delete[] axisMax;
	}
};

struct FootPedalConfiguration {
	PedalInfo info;

	bool *isInverted;
	int *deadzone; // a deadzone of 0x7fffffff indicates that the axis is not configured

	Action *primaryButtonActions;
	Action *primaryAxisActions;

	Action *secondaryButtonActions;
	Action *secondaryAxisActions;

	bool operator==(const FootPedalConfiguration &other) const {
		if (info != other.info) return false;

		assert(info.getNumButtons() == other.info.getNumButtons() && info.getNumAxes() == other.info.getNumAxes());
		for (unsigned char i = 0; i < info.getNumAxes(); i++) {
			if (isInverted[i] != other.isInverted[i] || deadzone[i] != other.deadzone[i] || primaryAxisActions[i] != other.primaryAxisActions[i]
			|| secondaryAxisActions[i] != other.secondaryAxisActions[i]) return false;
		}

		for (unsigned char i = 0; i < info.getNumButtons(); i++) {
			if (primaryButtonActions[i] != other.primaryButtonActions[i] || secondaryButtonActions[i] != other.secondaryButtonActions[i]) return false;
		}

		return true;
	}

	INLINE bool operator!=(const FootPedalConfiguration &other) const {
		return !(*this == other);
	}

	INLINE FootPedalConfiguration() {
		primaryButtonActions = secondaryButtonActions = primaryAxisActions = secondaryAxisActions = NULL;
		isInverted = NULL;
		deadzone = NULL;
	};

	INLINE FootPedalConfiguration(const FootPedalConfiguration &other) {
		info = other.info;

		isInverted = new bool[info.getNumAxes()];
		deadzone = new int[info.getNumAxes()];
		primaryAxisActions = new Action[info.getNumAxes()];
		secondaryAxisActions = new Action[info.getNumAxes()];

		primaryButtonActions = new Action[info.getNumButtons()];
		secondaryButtonActions = new Action[info.getNumButtons()];

		std::memcpy(isInverted, other.isInverted, sizeof(bool) * (size_t)info.getNumAxes());
		std::memcpy(deadzone, other.deadzone, sizeof(int) * (size_t)info.getNumAxes());
		std::memcpy(primaryAxisActions, other.primaryAxisActions, sizeof(Action) * (size_t)info.getNumAxes());
		std::memcpy(secondaryAxisActions, other.secondaryAxisActions, sizeof(Action) * (size_t)info.getNumAxes());

		std::memcpy(primaryButtonActions, other.primaryButtonActions, sizeof(Action) * (size_t)info.getNumButtons());
		std::memcpy(secondaryButtonActions, other.secondaryButtonActions, sizeof(Action) * (size_t)info.getNumButtons());

	}

	INLINE ~FootPedalConfiguration() {
		if (primaryButtonActions != NULL) {
			assert(secondaryButtonActions != NULL);
			delete[] primaryButtonActions;
			delete[] secondaryButtonActions;
		}

		if (primaryAxisActions != NULL) {
			assert(secondaryAxisActions != NULL);
			assert(isInverted != NULL);
			assert(deadzone != NULL);
			delete[] primaryAxisActions;
			delete[] secondaryAxisActions;
			delete[] isInverted;
			delete[] deadzone;
		}
	}
};

std::vector<FootPedalConfiguration*> loadFootpedalConfiguration(); //defined at end of file

class FootPedalCoordinator {
  private:
	static const bool DICTATION = true;
	static const bool CONFIGURATION = false;

	std::atomic<bool> alive;
	bool resync;
	bool mode;
	std::mutex syncLock;
	std::thread *loopThread;
	std::vector<FootPedalConfiguration*> configs;
	std::map<int,bool> deviceConnected;
	std::map<int,std::thread*> deviceThreads;
	std::mutex dcLock;
	std::mutex eventFunnel; // one event at a time!

	// Dictation mode event handler
	void (*eventHandler)(Action);

	// Configuration mode event handlers
	void (*onDeviceConnect)(const PedalInfo &, int);
	void (*onDeviceDisconnect)(int);
	void (*onPedalEvent)(const PedalEvent &, int);


	static PedalInfo getPedalInfo(char *fname, bool wait, char *name = NULL);

	//Loop for processing events for each footpedal while in DICTATION mode
	void footPedalLoop(int port, int fd, FootPedalConfiguration* conf);
	//Loop for each footpedal while in CONFIGURATION mode
	void footPedalConfLoop(int port, int fd, PedalInfo info);

	void coordinatorLoop();

	void deviceChange(char *fname, const uint32_t &event, bool fromSync);


  public:
	INLINE FootPedalCoordinator() : loopThread(NULL) {}
	INLINE ~FootPedalCoordinator() { assert(loopThread == NULL); }

	bool start(void (*pedalDictationEventHandler)(Action), void (*connectionHandler)(const PedalInfo &, int), void (*disconnectionHandler)(int), void (*pedalConfigEventHandler)(const PedalEvent &, int));
	void stop();

	void dictationMode(const std::vector<FootPedalConfiguration*> &newConfigs);
	void configurationMode();
	void syncDevices();
};

void saveFootpedalConfiguration(const std::vector<FootPedalConfiguration*> &configs);
std::vector<FootPedalConfiguration*> loadFootpedalConfiguration();
unsigned char getFootpedalConfigurationFileVersion();

#endif /* FOOTPEDAL_HPP_ */
