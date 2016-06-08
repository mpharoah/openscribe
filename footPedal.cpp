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

#include "footPedal.hpp"

#include <linux/joystick.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <libudev.h>
#include <linux/input.h>

#include <cstdio>
#include <ios>
#include <fstream>
#include <stdexcept>
#include <chrono>
#include <system_error>

#include "config.hpp"

/*
 * Get the name of the device from udev
 * Does not require read permission on the device
 *
 * Returns the device name or NULL on an error
 */
USERET char *getDeviceName(const char *devnode) {
	udev *context = udev_new();

	udev_enumerate *search = udev_enumerate_new(context);
	udev_enumerate_add_match_property(search, "DEVNAME", devnode);
	udev_enumerate_scan_devices(search);

	udev_list_entry *result = udev_enumerate_get_list_entry(search); // should only be 1 result (or zero results)

	if (result == NULL) {
		udev_enumerate_unref(search);
		udev_unref(context);
		return NULL;
	}

	udev_device *dev = udev_device_new_from_syspath(context, udev_list_entry_get_name(result));
	if (result == NULL) {
		udev_device_unref(dev);
		udev_enumerate_unref(search);
		udev_unref(context);
		return NULL;
	}

	udev_list_entry *serial = udev_list_entry_get_by_name(udev_device_get_properties_list_entry(dev), "ID_SERIAL");
	if (serial == NULL) {
		udev_device_unref(dev);
		udev_enumerate_unref(search);
		udev_unref(context);
		return NULL;
	}

	const char *udev_name = udev_list_entry_get_value(serial);
	size_t len = std::strlen(udev_name);
	if (len == 0 || std::strcmp(udev_name, "noserial") == 0) {
		udev_device_unref(dev);
		udev_enumerate_unref(search);
		udev_unref(context);
		return NULL;
	}

	char *name = new char[len+1];
	for (size_t i = 0; i < len; i++) {
		name[i] = (udev_name[i] == '_') ? ' ' : udev_name[i];
	}
	name[len] = '\0';

	udev_device_unref(dev);
	udev_enumerate_unref(search);
	udev_unref(context);

	return name;
}

PedalInfo FootPedalCoordinator::getPedalInfo(char *fname, bool wait, char *name) {
	PedalInfo info;

	int fd = open(fname, O_RDONLY);

	if (fd < 0 && wait && errno == EACCES) {
		/*
		 * If we get an EACCES error, this can mean that the file has been created,
		 * (as ioctl reported), but we are not allowed to open it quite yet. We will
		 * be able to open it shortly, however. (Once a device is connected, there is
		 * a short delay until its permissions are set)
		 *
		 * Alternatively, it could mean we don't have permission to read the requested
		 * device file, and will never be given permission
		 *
		 * We need to wait for a bit to figure out which one it is
		 */
	    std::chrono::milliseconds cs(10);
	    for (int i = 0; i < 10 && fd < 0; i++) {
		    std::this_thread::sleep_for(cs);
			fd = open(fname, O_RDONLY);
	    }

	}

	if (fd < 0) {
		if (errno == EACCES && name != NULL) {
			info.name = name;
			info.isProtected = true;
			info.axisMin = info.axisMax = NULL;
			return info;
		} else throw std::system_error(std::error_code(errno, std::system_category()));
	}

	info.isProtected = false;

	/* Get device name */
	if (name == NULL) {
		char temp[256];
		if (ioctl(fd, EVIOCGNAME(256), temp) < 0 || temp[0] == '\0') {
			info.name = new char[15];
			std::strcpy(info.name, "Unknown Device");
		} else {
			info.name = new char[1 + std::strlen(temp)];
			std::strcpy(info.name, temp);
		}
	} else {
		info.name = name;
	}

	/* Get buttons and axes */

	/*
	 * ioctl is just returning a bitset, with its size being rounded up to
	 * the next word of memory, so I have to use a array of longs instead
	 * of chars. It looks more complicated than it actually is.
	 */
	constexpr size_t longbits = 8 * sizeof(long);
	unsigned long codes[1 + (KEY_MAX - 1) / longbits]; // number of bits divided by number of bits in long, rounded up

	/* Buttons */
	std::memset(codes, 0, sizeof(codes));
	ioctl(fd, EVIOCGBIT(EV_KEY, KEY_MAX), codes);
	// Go through each bit representing a button code, and see if such a button exists
	unsigned short buttonNum = 0;
	for (unsigned short i = 0; i < KEY_MAX; i++) {
		if (codes[i / longbits] & (1ul << (unsigned long)(i % longbits))) { // check the i'th bit
			info.buttons[i] = buttonNum++;
		}
	}

	/* Axes */
	std::memset(codes, 0, sizeof(codes));
	ioctl(fd, EVIOCGBIT(EV_ABS, KEY_MAX), codes);
	// Go through each bit representing an axis code, and see if such an axis exists
	unsigned short axisNum = 0;
	for (unsigned short i = 0; i < KEY_MAX; i++) {
		if (codes[i / longbits] & (1ul << (unsigned long)(i % longbits))) { // check the i'th bit
			info.axes[i] = axisNum++;
		}
	}

	/* Get min and max of each axis */
	if (axisNum > 0) {
		info.axisMin = new int[axisNum];
		info.axisMax = new int[axisNum];

		for (auto mapping : info.axes) {
			assert(mapping.second < axisNum);
			int axisInfo[6]; // axisInfo[1] is minimum, axisInfo[2] is maximum. Don't care about other entries
			ioctl(fd, EVIOCGABS(mapping.first), axisInfo);
			if (axisInfo[1] != axisInfo[2]) {
				info.axisMin[mapping.second] = axisInfo[1];
				info.axisMax[mapping.second] = axisInfo[2];
			} else {
				info.axisMin[mapping.second] = -0x80000000;
				info.axisMax[mapping.second] =  0x7fffffff;
			}
		}
	} else {
		info.axisMin = NULL;
		info.axisMax = NULL;
	}

	close(fd);

	if (info.buttons.empty() && info.axes.empty()) {
		// Use error code ENOTTY ("Not a typewritter") to mean "not a footpedal"
		throw std::system_error(std::error_code(ENOTTY, std::system_category()), "Device is not a footpedal (no buttons or axes detected)");
	}

	return info;
}

//Loop for processing events for each footpedal while in DICTATION mode
void FootPedalCoordinator::footPedalLoop(int port, int fd, FootPedalConfiguration* conf) {
	if (fd < 0) return;

	timeval blockTime; // maximum (approximately) time it can take for a footpedal to respond to a request to stop processing events and close.
	blockTime.tv_sec = 0;
	blockTime.tv_usec = 100000; // 1/10th of a second sounds good. The delay only occurs when exiting the program or opening the footpedal configuration window

	bool buttonDown[conf->info.getNumButtons()];
	bool axisDown[conf->info.getNumAxes()];

	for (int i = 0; i < conf->info.getNumButtons(); i++) buttonDown[i] = false;
	for (int i = 0; i < conf->info.getNumAxes(); i++) axisDown[i] = false;

	fd_set fds;
	bool mod = false;
	FPEvent ev;
	while (alive) {
		dcLock.lock();
		assert(deviceConnected.count(port) == 1);
		if (!deviceConnected[port]) {
			dcLock.unlock();
			break;
		}
		dcLock.unlock();

		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		timeval timeout = blockTime;

		register const int status = select(fd+1, &fds, NULL, NULL, &timeout);
		if (status > 0) {
			ssize_t bytesRead = read(fd, &ev, sizeof(FPEvent));
			if (bytesRead <= 0) break;
			assert(bytesRead == sizeof(FPEvent));

			/* Process Event */
			Action cmd;
			cmd.type = Action::NOOP;
			cmd.amount = 0;

			bool isPress; // true = PRESS, false = RELEASE
			bool chmod = false;
			if (ev.type == EV_KEY) {
				/* Button Press/Release Event */
				if (!conf->info.buttons.count(ev.code)) continue;
				const unsigned short button = conf->info.buttons[ev.code];

				isPress = (ev.value > 0);
				buttonDown[button] = isPress;

				if (!mod || conf->primaryButtonActions[button].type == Action::MODIFIER || conf->primaryButtonActions[button].type == Action::TOGGLE_MODIFIER) {
					cmd = conf->primaryButtonActions[button];
				} else {
					cmd = conf->secondaryButtonActions[button];
				}
			} else if (ev.type == EV_ABS) {
				/* Axis move event */
				if (!conf->info.axes.count(ev.code)) continue;
				const unsigned short axis = conf->info.axes[ev.code];
				if (conf->deadzone[axis] == 0x7fffffff) continue;

				isPress = ((ev.value > conf->deadzone[axis]) != conf->isInverted[axis]);
				if (isPress != axisDown[axis]) {
					// changed from press to release or vice versa
					axisDown[axis] = isPress;

					if (!mod || conf->primaryAxisActions[axis].type == Action::MODIFIER || conf->primaryAxisActions[axis].type == Action::TOGGLE_MODIFIER) {
						cmd = conf->primaryAxisActions[axis];
					} else {
						cmd = conf->secondaryAxisActions[axis];
					}
				} else {
					// no change
					continue;
				}
			} else continue;

			if (cmd.type == Action::NOOP) continue;

			if (isPress) {
				/* Press */

				if (cmd.type == Action::MODIFIER) {
					chmod = !mod;
					cmd.type = Action::NOOP;
				} else if (cmd.type == Action::TOGGLE_MODIFIER) {
					chmod = true;
					cmd.type = Action::NOOP;
				}
				// otherwise, leave the action as it is
			} else {
				/* Release */

				if (cmd.type == Action::MODIFIER) {
					chmod = mod;
					cmd.type = Action::NOOP;
				} else {
					cmd = Action::getReleaseAction(cmd);
				}

			}

			if (chmod) {
				// Modifier was just activated or deactivated

				Action *buttonActions, *axisActions;

				eventFunnel.lock();
				/* Simulate release of all pressed buttons and axes */
				buttonActions = mod ? conf->secondaryButtonActions : conf->primaryButtonActions;
				axisActions = mod ? conf->secondaryAxisActions : conf->primaryAxisActions;

				for (int i = 0; i < conf->info.getNumButtons(); i++) {
					if (buttonDown[i] && conf->primaryButtonActions[i].type != Action::MODIFIER && conf->primaryButtonActions[i].type != Action::TOGGLE_MODIFIER) {
						Action RA = Action::getReleaseAction(buttonActions[i]);
						if (RA.type != Action::NOOP) eventHandler(RA);
					}
				}
				for (int i = 0; i < conf->info.getNumAxes(); i++) {
					if (axisDown[i] && conf->primaryAxisActions[i].type != Action::MODIFIER && conf->primaryAxisActions[i].type != Action::TOGGLE_MODIFIER) {
						Action RA = Action::getReleaseAction(axisActions[i]);
						if (RA.type != Action::NOOP) eventHandler(RA);
					}
				}

				/* Activate or deactivate modifier */
				mod = !mod;

				/* Simulate press of all currently pressed buttons and axes */
				buttonActions = mod ? conf->secondaryButtonActions : conf->primaryButtonActions;
				axisActions = mod ? conf->secondaryAxisActions : conf->primaryAxisActions;

				for (int i = 0; i < conf->info.getNumButtons(); i++) {
					if (buttonDown[i] && buttonActions[i].type != Action::NOOP && conf->primaryButtonActions[i].type != Action::MODIFIER && conf->primaryButtonActions[i].type != Action::TOGGLE_MODIFIER) {
						eventHandler(buttonActions[i]);
					}
				}
				for (int i = 0; i < conf->info.getNumAxes(); i++) {
					if (axisDown[i] && axisActions[i].type != Action::NOOP && conf->primaryAxisActions[i].type != Action::MODIFIER && conf->primaryAxisActions[i].type != Action::TOGGLE_MODIFIER) {
						eventHandler(axisActions[i]);
					}
				}
				eventFunnel.unlock();

				continue;
			}

			if (cmd.type == Action::NOOP) continue;

			eventFunnel.lock(); // only let one event through at a time
			eventHandler(cmd);
			eventFunnel.unlock();
		} else if (status < 0) { break; }
	}
	FD_ZERO(&fds);
	close(fd);

	/* Simulate release of all pedals */
	Action *buttonActions = mod ? conf->secondaryButtonActions : conf->primaryButtonActions;
	Action *axisActions = mod ? conf->secondaryAxisActions : conf->primaryAxisActions;

	for (int i = 0; i < conf->info.getNumButtons(); i++) {
		if (buttonDown[i] && conf->primaryButtonActions[i].type != Action::MODIFIER && conf->primaryButtonActions[i].type != Action::TOGGLE_MODIFIER) {
			Action RA = Action::getReleaseAction(buttonActions[i]);
			if (RA.type != Action::NOOP) eventHandler(RA);
		}
	}
	for (int i = 0; i < conf->info.getNumAxes(); i++) {
		if (axisDown[i] && conf->primaryAxisActions[i].type != Action::MODIFIER && conf->primaryAxisActions[i].type != Action::TOGGLE_MODIFIER) {
			Action RA = Action::getReleaseAction(axisActions[i]);
			if (RA.type != Action::NOOP) eventHandler(RA);
		}
	}
}

//Loop for each footpedal while in CONFIGURATION mode
void FootPedalCoordinator::footPedalConfLoop(int port, int fd, PedalInfo info) {
	if (fd < 0) return;

	timeval blockTime; // maximum (approximately) time it can take for a footpedal to respond to a request to stop processing events and close.
	blockTime.tv_sec = 0;
	blockTime.tv_usec = 100000; // 1/10th of a second sounds good. The delay only occurs when exiting the program or closing the footpedal configuration window

	fd_set fds;
	FPEvent ev;

	while (alive) {
		dcLock.lock();
		assert(deviceConnected.count(port) == 1);
		if (!deviceConnected[port]) {
			dcLock.unlock();
			break;
		}
		dcLock.unlock();

		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		timeval timeout = blockTime;

		register const int status = select(fd+1, &fds, NULL, NULL, &timeout);
		if (status > 0) {
			ssize_t bytesRead = read(fd, &ev, sizeof(FPEvent));
			if (bytesRead <= 0) break;
			assert(bytesRead == sizeof(FPEvent));

			if (ev.type == EV_KEY && info.buttons.count(ev.code)) {
				PedalEvent ret;
				ret.type = PedalEvent::TYPE_BUTTON;
				ret.index = info.buttons[ev.code];
				ret.isPressed = ev.value;
				onPedalEvent(ret, port);
			} else if (ev.type == EV_ABS && info.axes.count(ev.code)) {
				PedalEvent ret;
				ret.type = PedalEvent::TYPE_AXIS;
				ret.index = info.axes[ev.code];
				ret.position = (float)( ((double)ev.value - (double)info.axisMin[ret.index]) / ((double)info.axisMax[ret.index] - (double)info.axisMin[ret.index]));
				onPedalEvent(ret, port);
			}

		} else if (status < 0) { break; }
	}
	FD_ZERO(&fds);
	close(fd);
}

void FootPedalCoordinator::coordinatorLoop() {
	/*
	'devices' is a file stream that outputs data when a file is created or
	removed in /dev/input. (When a joystick device is connected or removed)
	*/
	int devices = inotify_init();
	int watchpost = inotify_add_watch(devices, "/dev/input/", IN_CREATE | IN_DELETE);
	if (devices <= 0 || watchpost <= 0) throw std::runtime_error("Error attempting to watch /dev/input for changes");

	// Limit how long SELECT can block for (need to unblock to see if the user has closed the program)
	timeval blockTime;
	blockTime.tv_sec = 0;
	blockTime.tv_usec = 250000;

	inotify_event ev;
	/*
	We need the buffer to be big enough to hold an entire event (including
	the name of the file created/removed) or things go screwy. Pretty much
	every filesystem that matters limits filenames to 255 bytes, which
	gives a bound for our buffer size. In practice, the filenames that will
	appear here will be no more than 7 bytes (event##), but we want to make
	sure the program won't mess up if the user, for some incomprehensible
	reason, manually creates a 255-character-long file in /dev/input.
	*/
	char buff[sizeof(inotify_event)+256];

	fd_set watchedFiles; //this set only contains one "file"
	syncLock.lock();
	while (alive) {
		// select modifies the timeout paramater, so make a copy
		timeval timeout = blockTime;
		FD_ZERO(&watchedFiles);
		FD_SET(devices, &watchedFiles);

		syncLock.unlock();
		register const int status = select(devices+1, &watchedFiles, NULL, NULL, &timeout);
		syncLock.lock();
		if (status < 0) throw std::runtime_error("An unexpected error occurred while monitoring /dev/input for changes.");

		if (status == 0) { //nothing to report
			if (resync) {
				alive = false;
				for (auto pedal : deviceThreads) {
					pedal.second->join();
					delete pedal.second;
				}
				for (auto pedal : deviceConnected) onDeviceDisconnect(pedal.first);
				deviceThreads.clear();
				deviceConnected.clear();
				alive = true;

				DIR *devInput = opendir("/dev/input");
				dirent *fileInfo;

				if (devInput == NULL) throw std::runtime_error("opendir(\"/dev/input\") returned an error!");

				// Don't try to free fileInfo -- opendir, readdir, and closedir handle that
				while ((fileInfo = readdir(devInput)) != NULL) deviceChange(fileInfo->d_name, IN_CREATE, true);
				closedir(devInput);

				resync = false;
			}
			continue;
		}

		if (resync) continue;

		// We can now perform a read that will not block
		ssize_t bytesRead = read(devices, buff, sizeof(inotify_event)+256);
		if (bytesRead <= (ssize_t)sizeof(inotify_event)) throw std::runtime_error("Received incomplete or invalid event from inotify. Aborting.");

		std::memcpy(&ev, buff, sizeof(inotify_event));
		char *fname = &buff[sizeof(inotify_event)];
		fname[ev.len] = '\0';

		deviceChange(fname, ev.mask, false);
	}
	syncLock.unlock();
	// Loop exited. Cleanup time!

	FD_ZERO(&watchedFiles);
	inotify_rm_watch(devices, watchpost);
	close(devices);

	// end remaining threads
	assert(!alive);
	for (auto &p : deviceThreads) {
		p.second->join();
		delete p.second;
	}

	// I don't need to clear() deviceThreads or deviceConnected because this class is about to be deleted
}

/* Handle a device connection or disconnection
 * Should only be called from coordinatorLoop()
 * (event is either IN_CREATE or IN_DELETE)
 *
 * fromSync indicates wether or not this function was called as a result of a sync request
 */
void FootPedalCoordinator::deviceChange(char *fname, const uint32_t &event, bool fromSync) {
	// Ignore files that aren't called event#
	register const size_t len = std::strlen(fname);
	if (len < 6 || len > 14 || fname[0] != 'e' || fname[1] != 'v' || fname[2] != 'e' || fname[3] != 'n' || fname[4] != 't') return;
	bool isNumber = true;
	for (int i = 5; fname[i] != '\0'; i++) {
		if (fname[i] < '0' || fname[i] > '9') {
			isNumber = false;
			break;
		}
	}
	if (!isNumber) return;
	// Also, there shouldn't be leading zeroes
	if (fname[5] == '0' && fname[6] != '\0') return;

	int fpid = std::atoi(&fname[5]); // file is /dev/input/event[fpid]

	if (event == IN_CREATE && deviceConnected.count(fpid)) {
		// device is already connected. We are out of sync
		std::printf("[Warning] Detected device connection on /dev/input/js%d, but we thought a device was already connected there. Resyncing with /dev/input.", fpid);
		resync = true;
		return;
	}

	char fullname[12 + std::strlen(fname)];
	std::strcpy(fullname, "/dev/input/");
	std::strcpy(&fullname[11], fname);

	if (mode == CONFIGURATION) {
		if (event == IN_CREATE) {
			// device connected

			char *deviceName = getDeviceName(fullname);
			if (deviceName == NULL) return;

			try {
				PedalInfo info = getPedalInfo(fullname, !fromSync, deviceName); // info now handles deviceName. Do not delete[] unless this throws an execption
				onDeviceConnect(info, fpid);
				dcLock.lock();
				deviceConnected[fpid] = true;
				dcLock.unlock();
				if (!info.isProtected) deviceThreads[fpid] = new std::thread(&FootPedalCoordinator::footPedalConfLoop, this, fpid, open(fullname, O_RDONLY), info);
			} catch (std::system_error &e) {
				delete[] deviceName;
			}
		} else { // event == IN_DELETE
			// device disconnected
			onDeviceDisconnect(fpid);
			goto doDisconnect;
		}
	} else { // mode == DICTATION
		if (event == IN_CREATE) {
			// A device has been connected

			char *deviceName = getDeviceName(fullname);
			if (deviceName == NULL) return;

			// check if the device has been configured
			try {
				PedalInfo info = getPedalInfo(fullname, !fromSync, deviceName); // info now handles deviceName. Do not delete[] unless this throws an execption

				if (info.isProtected) {
					bool found = false;
					for (FootPedalConfiguration *conf : configs) {
						if (std::strcmp(conf->info.name, deviceName) == 0) {
							found = true;
							break;
						}
					}

					if (found) {
						char username[256];

						// get the username of the current user
						std::FILE *who = popen("whoami", "r");
						if (std::fgets(username, 256, who)) {
							// get rid of the newline at the end of the username
							for (int i = 0; username[i] != '\0'; i++) {
								if (username[i] == '\n') {
									username[i] = '\0';
									break;
								}
							}

							// add the current user to the list of users allowed to read from the device
							char *cmd = new char[453];
							std::snprintf(cmd, 453, "gksudo -m 'A configured device has been detected, but it is protected. Enter your password to allow OpenScribe to read from the connected input device.' 'setfacl -m u:%s:r %s'", username, fullname);

							if (std::system(cmd) == 0) {
								// success
								pclose(who);
								delete[] cmd;
								info.name = NULL; //don't free deviceName yet
								info = getPedalInfo(fullname, false, deviceName); // try openning the device again
								if (info.isProtected) return;
							} else {
								delete[] cmd;
								return;
							}
						} else return;
					} else return;
				}

				for (FootPedalConfiguration* conf : configs) {
					if (conf->info == info) {
						// found a foot pedal matching the connected device

						dcLock.lock();
						deviceConnected[fpid] = true;
						dcLock.unlock();

						deviceThreads[fpid] = new std::thread(&FootPedalCoordinator::footPedalLoop, this, fpid, open(fullname, O_RDONLY), conf);

						break;
					}
				}
			} catch (std::system_error &e) {
				delete[] deviceName;
			}

		} else { //event == IN_DELETE
			// A device has been disconnected
doDisconnect:
			if (deviceThreads.count(fpid) > 0) {
				// The thread should be closing on its own anyways because the file closed,
				// but I'll use deviceConnected to terminate the thread just to be safe

				std::thread *T = deviceThreads[fpid];

				dcLock.lock();
				deviceConnected[fpid] = false;
				dcLock.unlock();

				T->join();
				delete T;
				deviceThreads.erase(fpid);
			}

			dcLock.lock();
			deviceConnected.erase(fpid);
			dcLock.unlock();
		}
	}
}


bool FootPedalCoordinator::start(void (*pedalDictationEventHandler)(Action), void (*connectionHandler)(const PedalInfo &, int), void (*disconnectionHandler)(int), void (*pedalConfigEventHandler)(const PedalEvent &, int)) {
	alive = true;
	resync = true;
	mode = DICTATION;

	eventHandler = pedalDictationEventHandler;
	onPedalEvent = pedalConfigEventHandler;
	onDeviceConnect = connectionHandler;
	onDeviceDisconnect = disconnectionHandler;

	bool ret = true;
	try {
		configs = loadFootpedalConfiguration();
	} catch (const std::ios_base::failure &ex) {
		for (FootPedalConfiguration *c : configs) delete c;
		configs.clear();
		ret = false;
	}

	loopThread = new std::thread(&FootPedalCoordinator::coordinatorLoop, this);
	return ret;
}

void FootPedalCoordinator::stop() {
	alive = false;
	loopThread->join();
	delete loopThread;
	loopThread = NULL;
	for (FootPedalConfiguration *conf : configs) delete conf;
}

void FootPedalCoordinator::dictationMode(const std::vector<FootPedalConfiguration*> &newConfigs) {
	assert(loopThread != NULL);
	syncLock.lock();

	// If a resync is already pending, wait for it to finish to ensure we are actually in the current mode
	while (resync) {
		syncLock.unlock();
		std::this_thread::yield();
		syncLock.lock();
	}

	// If we are already in dication mode, do nothing
	if (mode == DICTATION) {
		syncLock.unlock();
		return;
	}

	/* Update configuration
	 * This is safe to do because we know that we are in CONFIGURATION mode,
	 * and since we waited for pending resync request to complete, there
	 * are no footpedal threads running in DICTATION mode. (Threads in
	 * CONFIGUTATION mode do not access the configuration vector)
	 */
	for (FootPedalConfiguration *conf : configs) delete conf;
	configs.clear();
	configs = newConfigs;
	// The fact that I am simply copying the pointers in conf over
	// instead of copying their data is intentional

	mode = DICTATION;
	resync = true;
	syncLock.unlock();
}

void FootPedalCoordinator::configurationMode() {
	assert(loopThread != NULL);
	syncLock.lock();

	// If a resync is already pending, wait for it to finish to ensure we are actually in the current mode
	while (resync) {
		syncLock.unlock();
		std::this_thread::yield();
		syncLock.lock();
	}

	if (mode == CONFIGURATION) {
		syncLock.unlock();
		return;
	}

	mode = CONFIGURATION;
	resync = true;
	syncLock.unlock();
}

void FootPedalCoordinator::syncDevices() {
	assert(loopThread != NULL);
	syncLock.lock();
	resync = true;
	syncLock.unlock();
}

INLINE void FPC_WRITE(const void *buffer, size_t size, size_t count, FILE *stream) {
	if (std::fwrite(buffer, size, count, stream) < count) {
		std::fclose(stream);
		throw std::ios_base::failure("Error saving footpedal configuration file. (Write failed)");
	}
}

void saveFootpedalConfiguration(const std::vector<FootPedalConfiguration*> &configs) {
	touchOptionsFolder();
	std::FILE *out = std::fopen((std::string(getenv("HOME")) + "/.config/OpenScribe/pedalConf").c_str(), "w");
	if (out == NULL) throw std::ios_base::failure("Error: Could not save footpedal configurations to ~/.config/OpenScribe/pedalConf");

	char header[] = "OSFCv02";
	FPC_WRITE(header, 1, 7, out);

	unsigned short num = (unsigned short)configs.size();
	FPC_WRITE(&num, sizeof(unsigned short), 1, out);
	for (FootPedalConfiguration *conf : configs) {
		const unsigned char stringSize = (unsigned char)std::strlen(conf->info.name);
		FPC_WRITE(&stringSize, 1, 1, out);
		FPC_WRITE(conf->info.name, 1, stringSize, out);
		unsigned short numberOfButtons = conf->info.getNumButtons();
		unsigned short numberOfAxes = conf->info.getNumAxes();
		FPC_WRITE(&numberOfButtons, sizeof(unsigned short), 1, out);
		FPC_WRITE(&numberOfAxes, sizeof(unsigned short), 1, out);

		for (unsigned short j = 0; j < numberOfButtons; j++) {
#ifndef NDEBUG
			bool found = false;
#endif
			for (auto mapping : conf->info.buttons) {
				if (mapping.second == j) {
					FPC_WRITE(&mapping.first, sizeof(unsigned short), 1, out);
#ifndef NDEBUG
					found = true;
#endif
					break;
				}
			}
#ifndef NDEBUG
			assert(found);
#endif
		}

		for (unsigned short j = 0; j < numberOfAxes; j++) {
#ifndef NDEBUG
			bool found = false;
#endif
			for (auto mapping : conf->info.axes) {
				if (mapping.second == j) {
					FPC_WRITE(&mapping.first, sizeof(unsigned short), 1, out);
#ifndef NDEBUG
					found = true;
#endif
					break;
				}
			}
#ifndef NDEBUG
			assert(found);
#endif
		}

		for (unsigned short j = 0; j < numberOfButtons; j++) {
			FPC_WRITE(&conf->primaryButtonActions[j].type, 1, 1, out);
			if (conf->primaryButtonActions[j].type == Action::SKIP || conf->primaryButtonActions[j].type == Action::CHANGE_SLOW_SPEED) {
				FPC_WRITE(&conf->primaryButtonActions[j].amount, 1, 1, out);
			}

			FPC_WRITE(&conf->secondaryButtonActions[j].type, 1, 1, out);
			if (conf->secondaryButtonActions[j].type == Action::SKIP || conf->secondaryButtonActions[j].type == Action::CHANGE_SLOW_SPEED) {
				FPC_WRITE(&conf->secondaryButtonActions[j].amount, 1, 1, out);
			}
		}

		for (unsigned short j = 0; j < numberOfAxes; j++) {
			const unsigned char ii = conf->isInverted[j] ? 1 : 0;
			FPC_WRITE(&ii, 1, 1, out);

			FPC_WRITE(&conf->deadzone[j], sizeof(int), 1, out);

			FPC_WRITE(&conf->primaryAxisActions[j].type, 1, 1, out);
			if (conf->primaryAxisActions[j].type == Action::SKIP || conf->primaryAxisActions[j].type == Action::CHANGE_SLOW_SPEED) {
				FPC_WRITE(&conf->primaryAxisActions[j].amount, 1, 1, out);
			}

			FPC_WRITE(&conf->secondaryAxisActions[j].type, 1, 1, out);
			if (conf->secondaryAxisActions[j].type == Action::SKIP || conf->secondaryAxisActions[j].type == Action::CHANGE_SLOW_SPEED) {
				FPC_WRITE(&conf->secondaryAxisActions[j].amount, 1, 1, out);
			}
		}
	}
	std::fclose(out);
}

INLINE void FPC_READ(void *buffer, size_t size, size_t count, FILE *stream) {
	if (std::fread(buffer, size, count, stream) < count) {
		std::fclose(stream);
		throw std::ios_base::failure("Error loading footpedal configuration file.");
	}
}

std::vector<FootPedalConfiguration*> loadFootpedalConfiguration() {
	std::vector<FootPedalConfiguration*> ret;

	std::FILE *in = std::fopen((std::string(getenv("HOME")) + "/.config/OpenScribe/pedalConf").c_str(), "r");
	if (in == NULL) {
		if (errno == ENOENT) {
			//File does not exist yet
			return ret;
		} else throw std::ios_base::failure("Error loading footpedal configuration file.");
	}

	char header[7];
	FPC_READ(header, 1, 7, in);
	if (std::memcmp(header, "OSFCv02", 7) != 0) {
		throw std::ios_base::failure("Error loading footpedal configuration file.");
	}

	unsigned short size;
	FPC_READ(&size, sizeof(unsigned short), 1, in);
	if (size == 0) return ret;
	ret.reserve(size);
	for (unsigned short i = 0; i < size; i++) {
		FootPedalConfiguration *conf = new FootPedalConfiguration();

		unsigned char stringSize;
		FPC_READ(&stringSize, 1, 1, in);
		conf->info.name = new char[stringSize+1];
		FPC_READ(conf->info.name, 1, stringSize, in);
		conf->info.name[stringSize] = '\0';

		unsigned short numberOfButtons, numberOfAxes;

		FPC_READ(&numberOfButtons, sizeof(unsigned short), 1, in);
		FPC_READ(&numberOfAxes, sizeof(unsigned short), 1, in);

		for (unsigned short j = 0; j < numberOfButtons; j++) {
			unsigned short code;
			FPC_READ(&code, sizeof(unsigned short), 1, in);
			if (code >= KEY_MAX) throw std::ios_base::failure("Error loading footpedal configuration file.");
			conf->info.buttons[code] = j;
		}

		for (unsigned short j = 0; j < numberOfAxes; j++) {
			unsigned short code;
			FPC_READ(&code, sizeof(unsigned short), 1, in);
			if (code >= KEY_MAX) throw std::ios_base::failure("Error loading footpedal configuration file.");
			conf->info.axes[code] = j;
		}

		conf->primaryButtonActions = new Action[numberOfButtons];
		conf->secondaryButtonActions = new Action[numberOfButtons];
		for (unsigned short j = 0; j < numberOfButtons; j++) {
			FPC_READ(&conf->primaryButtonActions[j].type, 1, 1, in);
			if (conf->primaryButtonActions[j].type == Action::SKIP || conf->primaryButtonActions[j].type == Action::CHANGE_SLOW_SPEED) {
				FPC_READ(&conf->primaryButtonActions[j].amount, 1, 1, in);
			} else {
				conf->primaryButtonActions[j].amount = 0;
			}

			FPC_READ(&conf->secondaryButtonActions[j].type, 1, 1, in);
			if (conf->secondaryButtonActions[j].type == Action::SKIP || conf->secondaryButtonActions[j].type == Action::CHANGE_SLOW_SPEED) {
				FPC_READ(&conf->secondaryButtonActions[j].amount, 1, 1, in);
			} else {
				conf->secondaryButtonActions[j].amount = 0;
			}
		}

		conf->primaryAxisActions = new Action[numberOfAxes];
		conf->secondaryAxisActions = new Action[numberOfAxes];
		conf->isInverted = new bool[numberOfAxes];
		conf->deadzone = new int[numberOfAxes];
		for (unsigned short j = 0; j < numberOfAxes; j++) {
			unsigned char ii;
			FPC_READ(&ii, 1, 1, in);
			conf->isInverted[j] = (ii == 1);

			FPC_READ(&conf->deadzone[j], sizeof(int), 1, in);

			FPC_READ(&conf->primaryAxisActions[j].type, 1, 1, in);
			if (conf->primaryAxisActions[j].type == Action::SKIP || conf->primaryAxisActions[j].type == Action::CHANGE_SLOW_SPEED) {
				FPC_READ(&conf->primaryAxisActions[j].amount, 1, 1, in);
			} else {
				conf->primaryAxisActions[j].amount = 0;
			}

			FPC_READ(&conf->secondaryAxisActions[j].type, 1, 1, in);
			if (conf->secondaryAxisActions[j].type == Action::SKIP || conf->secondaryAxisActions[j].type == Action::CHANGE_SLOW_SPEED) {
				FPC_READ(&conf->secondaryAxisActions[j].amount, 1, 1, in);
			} else {
				conf->secondaryAxisActions[j].amount = 0;
			}
		}
		ret.push_back(conf);
	}

	std::fclose(in);
	return ret;
}

unsigned char getFootpedalConfigurationFileVersion() {
	std::FILE *in = std::fopen((std::string(getenv("HOME")) + "/.config/OpenScribe/pedalConf").c_str(), "r");
	if (in == NULL) return 0;

	char header[7];
	if (std::fread(header, 1, 7, in) < 7) {
		std::fclose(in);
		return 1;
	}

	std::fclose(in);
	if (std::memcmp(header, "OSFCv", 5) != 0) return 1; // version 1 didn't include version information in the file
	if (header[5] < '0' || header[5] > '9' || header[6] < '0' || header[6] > '9') return 1;
	return (10 * (header[5] - '0') + header[6] - '0');
}
