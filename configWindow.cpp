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

#include "configWindow.hpp"

#include <gtkmm/messagedialog.h>
#include <cstdio>

#include "helpWindow.hpp"

/*
 * Action Selector / Toggle Selector
 */
#define ADD_ACTIONBOX_ENTRY(_action_, _name_) { \
	auto newEntry = comboModel->append(); \
	newEntry->set_value(entryData, _action_); \
	newEntry->set_value(entryText, Glib::ustring(_name_)); \
}

ActionSelector::ActionSelector() : Gtk::ComboBox() {
	entryType.add(entryData);
	entryType.add(entryText);

	comboModel = Gtk::ListStore::create(entryType);

	ADD_ACTIONBOX_ENTRY(NOOP, "Do Nothing");
	ADD_ACTIONBOX_ENTRY(PLAY, "Play");
	ADD_ACTIONBOX_ENTRY(SLOW, "Slow");
	ADD_ACTIONBOX_ENTRY(FFWD, "Fast Forward");
	ADD_ACTIONBOX_ENTRY(RWD, "Rewind");
	ADD_ACTIONBOX_ENTRY(SKIP_AHEAD, "Skip Forward");
	ADD_ACTIONBOX_ENTRY(SKIP_BACK, "Skip Backwards");
	ADD_ACTIONBOX_ENTRY(RESTART, "Skip to Beginning");
	ADD_ACTIONBOX_ENTRY(FASTER, "Increase Slow Speed");
	ADD_ACTIONBOX_ENTRY(SLOWER, "Decrease Slow Speed");

	set_model(comboModel);
	pack_start(entryText);

	set_active(0);
}

void ActionSelector::makePrimary() {
	ADD_ACTIONBOX_ENTRY(MOD, "Modifier Pedal");
}

const int ActionSelector::NOOP = 0;
const int ActionSelector::PLAY = 1;
const int ActionSelector::SLOW = 2;
const int ActionSelector::FFWD = 3;
const int ActionSelector::RWD = 4;
const int ActionSelector::SKIP_AHEAD = 5;
const int ActionSelector::SKIP_BACK = 6;
const int ActionSelector::RESTART = 7;
const int ActionSelector::FASTER = 8;
const int ActionSelector::SLOWER = 9;
const int ActionSelector::MOD = 10;

ToggleSelector::ToggleSelector() : Gtk::ComboBox() {
	entryType.add(entryData);
	entryType.add(entryText);

	comboModel = Gtk::ListStore::create(entryType);

	auto holdEntry = comboModel->append();
	holdEntry->set_value(entryData, false);
	holdEntry->set_value(entryText, Glib::ustring("Hold"));

	auto toggleEntry = comboModel->append();
	toggleEntry->set_value(entryData, true);
	toggleEntry->set_value(entryText, Glib::ustring("Toggle"));

	set_model(comboModel);
	pack_start(entryText);

	set_active(0);
}

/*
 * ConfigRowBase and ButtonConfigRow
 */

const int ConfigRowBase::NAME_WIDTH =	7;
const int ConfigRowBase::DOWN_WIDTH =	7;
const int ConfigRowBase::ACTION_WIDTH =	18;
const int ConfigRowBase::EXT_WIDTH =	15;

/* Don't touch these ones */
const int ConfigRowBase::NAME_INDEX = 0;
const int ConfigRowBase::DOWN_INDEX =		ConfigRowBase::NAME_INDEX +		ConfigRowBase::NAME_WIDTH;
const int ConfigRowBase::P_ACTION_INDEX =	ConfigRowBase::DOWN_INDEX +		ConfigRowBase::DOWN_WIDTH;
const int ConfigRowBase::P_EXT_INDEX =		ConfigRowBase::P_ACTION_INDEX +	ConfigRowBase::ACTION_WIDTH;
const int ConfigRowBase::S_ACTION_INDEX =	ConfigRowBase::P_EXT_INDEX +	ConfigRowBase::EXT_WIDTH;
const int ConfigRowBase::S_EXT_INDEX = 		ConfigRowBase::S_ACTION_INDEX +	ConfigRowBase::ACTION_WIDTH;

ConfigRowBase::ConfigRowBase(Gtk::Grid *parent, int i) {
	layout = parent;
	row = i;

	primaryActionBox.makePrimary();

	secondsLabel1.set_label("seconds");
	secondsLabel2.set_label("seconds");
	percentLabel1.set_label("percent");
	percentLabel2.set_label("percent");

	primarySeconds.set_digits(1);
	primarySeconds.set_increments(0.1, 1.0);
	primarySeconds.set_range(0.1, 12.7);
	primarySeconds.set_snap_to_ticks(true);

	secondarySeconds.set_digits(1);
	secondarySeconds.set_increments(0.1, 1.0);
	secondarySeconds.set_range(0.1, 12.7);
	secondarySeconds.set_snap_to_ticks(true);

	primaryPercent.set_digits(0);
	primaryPercent.set_increments(1.0, 10.0);
	primaryPercent.set_range(1.0, 80.0);
	primaryPercent.set_snap_to_ticks(true);

	secondaryPercent.set_digits(0);
	secondaryPercent.set_increments(1.0, 10.0);
	secondaryPercent.set_range(1.0, 80.0);
	secondaryPercent.set_snap_to_ticks(true);

	primarySecondsBox.pack_start(primarySeconds, true, true);
	primarySecondsBox.pack_start(secondsLabel1, false, false);

	secondarySecondsBox.pack_start(secondarySeconds, true, true);
	secondarySecondsBox.pack_start(secondsLabel2, false, false);

	primaryPercentBox.pack_start(primaryPercent, true, true);
	primaryPercentBox.pack_start(percentLabel1, false, false);

	secondaryPercentBox.pack_start(secondaryPercent, true, true);
	secondaryPercentBox.pack_start(percentLabel2, false, false);

	parent->attach(primaryActionBox, P_ACTION_INDEX, i, ACTION_WIDTH, 1);
	parent->attach(primaryEmpty, P_EXT_INDEX, i, EXT_WIDTH, 1);
	parent->attach(secondaryActionBox, S_ACTION_INDEX, i, ACTION_WIDTH, 1);
	parent->attach(secondaryEmpty, S_EXT_INDEX, i, EXT_WIDTH, 1);

	primarySeconds.set_value(1.0);
	secondarySeconds.set_value(1.0);
	primaryPercent.set_value(10.0);
	secondaryPercent.set_value(10.0);

	p_ext = s_ext = EXT_NONE;

	parent->show_all_children();
}

void ButtonConfigRow::connectSignals() {
	primaryActionBox.signal_changed().connect(sigc::mem_fun(*this, &ButtonConfigRow::onPrimaryActionChange));
	secondaryActionBox.signal_changed().connect(sigc::mem_fun(*this, &ButtonConfigRow::onSecondaryActionChange));
	updatePedalDown.connect(sigc::mem_fun(*this, &ButtonConfigRow::onPedalStatusChange));
}

inline void ConfigRowBase::onActionChange(bool primary) {
	decltype(p_ext) &ext_var = primary ? p_ext : s_ext;
	ActionSelector &actionBox = primary ? primaryActionBox : secondaryActionBox;
	Gtk::HBox &w_empty = primary ? primaryEmpty : secondaryEmpty;
	ToggleSelector &w_toggle = primary ? primaryToggleBox : secondaryToggleBox;
	Gtk::HBox &w_seconds = primary ? primarySecondsBox : secondarySecondsBox;
	Gtk::HBox &w_percent = primary ? primaryPercentBox : secondaryPercentBox;
	const int EXT_INDEX = primary ? P_EXT_INDEX : S_EXT_INDEX;

	switch (ext_var) {
		case EXT_NONE: layout->remove(w_empty); break;
		case EXT_TOGGLE: layout->remove(w_toggle); break;
		case EXT_SECONDS: layout->remove(w_seconds); break;
		case EXT_PERCENT: layout->remove(w_percent); break;
	}

	switch (actionBox.getActionType()) {
		case ActionSelector::NOOP:
		case ActionSelector::RESTART:
			layout->attach(w_empty, EXT_INDEX, row, EXT_WIDTH, 1);
			ext_var = EXT_NONE;
			break;
		case ActionSelector::PLAY:
		case ActionSelector::SLOW:
		case ActionSelector::FFWD:
		case ActionSelector::RWD:
		case ActionSelector::MOD:
			layout->attach(w_toggle, EXT_INDEX, row, EXT_WIDTH, 1);
			ext_var = EXT_TOGGLE;
			break;
		case ActionSelector::SKIP_AHEAD:
		case ActionSelector::SKIP_BACK:
			layout->attach(w_seconds, EXT_INDEX, row, EXT_WIDTH, 1);
			ext_var = EXT_SECONDS;
			break;
		case ActionSelector::FASTER:
		case ActionSelector::SLOWER:
			layout->attach(w_percent, EXT_INDEX, row, EXT_WIDTH, 1);
			ext_var = EXT_PERCENT;
			break;
	}

	if (primary) {
		if (actionBox.getActionType() == ActionSelector::MOD) {
			secondaryActionBox.set_active(0);
			if (s_ext != EXT_NONE) {
				switch (s_ext) {
					case EXT_TOGGLE: layout->remove(secondaryToggleBox); break;
					case EXT_SECONDS: layout->remove(secondarySecondsBox); break;
					case EXT_PERCENT: layout->remove(secondaryPercentBox); break;
					default: break; // suppress compiler warning about missing EXT_NONE
				}
				layout->attach(secondaryEmpty, S_EXT_INDEX, row, EXT_WIDTH, 1);
				s_ext = EXT_NONE;
			}
			secondaryActionBox.set_sensitive(false);
		} else {
			secondaryActionBox.set_sensitive(true);
		}
	}

	layout->show_all_children();
}

USERET Action ConfigRowBase::getAction(bool primary) {
	ActionSelector &w_action = primary ? primaryActionBox : secondaryActionBox;
	ToggleSelector &w_toggle = primary ? primaryToggleBox : secondaryToggleBox;
	Gtk::SpinButton &w_seconds = primary ? primarySeconds : secondarySeconds;
	Gtk::SpinButton &w_percent = primary ? primaryPercent : secondaryPercent;

	Action ret;
	ret.amount = 0;
	switch (w_action.getActionType()) {
		case ActionSelector::NOOP:
			ret.type = Action::NOOP;
			break;
		case ActionSelector::RESTART:
			ret.type = Action::RESTART;
			break;
		case ActionSelector::PLAY:
			ret.type = w_toggle.isToggleSelected() ? Action::TOGGLE_PLAY : Action::PLAY;
			break;
		case ActionSelector::SLOW:
			ret.type = w_toggle.isToggleSelected() ? Action::TOGGLE_SLOW : Action::SLOW;
			break;
		case ActionSelector::FFWD:
			ret.type = w_toggle.isToggleSelected() ? Action::TOGGLE_FAST_FORWARD : Action::FAST_FORWARD;
			break;
		case ActionSelector::RWD:
			ret.type = w_toggle.isToggleSelected() ? Action::TOGGLE_REWIND : Action::REWIND;
			break;
		case ActionSelector::MOD:
			ret.type = w_toggle.isToggleSelected() ? Action::TOGGLE_MODIFIER : Action::MODIFIER;
			break;
		case ActionSelector::SKIP_AHEAD:
			ret.type = Action::SKIP;
			ret.deciseconds = (signed char)(10.0 * w_seconds.get_value());
			break;
		case ActionSelector::SKIP_BACK:
			ret.type = Action::SKIP;
			ret.deciseconds = (signed char)(-10.0 * w_seconds.get_value());
			break;
		case ActionSelector::FASTER:
			ret.type = Action::CHANGE_SLOW_SPEED;
			ret.percent = (signed char)w_percent.get_value();
			break;
		case ActionSelector::SLOWER:
			ret.type = Action::CHANGE_SLOW_SPEED;
			ret.percent = (signed char)-w_percent.get_value();
			break;
	}
	return ret;
}

FLATTEN void ConfigRowBase::onPrimaryActionChange() {
	onActionChange(true);
}

FLATTEN void ConfigRowBase::onSecondaryActionChange() {
	onActionChange(false);
}

ButtonConfigRow::ButtonConfigRow(Gtk::Grid *parent, int i) : ConfigRowBase(parent, i) {
	char nameText[19];
	std::sprintf(nameText, "Button %d", i);
	name.set_label(Glib::ustring(nameText));
	nameBox.pack_start(name, false, false);
	nameBox.pack_start(spacer, true, true);
	parent->attach(nameBox, NAME_INDEX, i, NAME_WIDTH, 1);

	isDown.set_markup("<span color='#800000'>NO</span>");
	parent->attach(isDown, DOWN_INDEX, i, DOWN_WIDTH, 1);

	parent->show_all_children();
	connectSignals();
}

ButtonConfigRow::ButtonConfigRow(Gtk::Grid *parent, int i, const Action &action1, const Action &action2) : ConfigRowBase(parent, i) {
	char nameText[19];
	std::sprintf(nameText, "Button %d", i);
	name.set_label(Glib::ustring(nameText));
	nameBox.pack_start(name, false, false);
	nameBox.pack_start(spacer, true, true);
	parent->attach(nameBox, NAME_INDEX, i, NAME_WIDTH, 1);

	isDown.set_markup("<span color='#800000'>NO</span>");
	parent->attach(isDown, DOWN_INDEX, i, DOWN_WIDTH, 1);

	setAction(action1, true);
	setAction(action2, false);

	parent->show_all_children();
	connectSignals();
}

void ButtonConfigRow::onPedalStatusChange() {
	if (pedalDown) {
		isDown.set_markup("<span color='#008000'>YES</span>");
	} else {
		isDown.set_markup("<span color='#800000'>NO</span>");
	}
}

void ConfigRowBase::setAction(const Action &cmd, bool primary) {
	ActionSelector &actionBox = primary ? primaryActionBox : secondaryActionBox;
	ToggleSelector &toggleBox = primary ? primaryToggleBox : secondaryToggleBox;
	Gtk::SpinButton &seconds = primary ? primarySeconds : secondarySeconds;
	Gtk::SpinButton &percent = primary ? primaryPercent : secondaryPercent;

	switch (cmd.type) {
		case Action::NOOP:
			break;

		case Action::RESTART:
			actionBox.set_active(ActionSelector::RESTART);
			break;

		case Action::TOGGLE_PLAY:
			toggleBox.set_active(1);
		case Action::PLAY:
			actionBox.set_active(ActionSelector::PLAY);
			break;
		case Action::TOGGLE_SLOW:
			toggleBox.set_active(1);
		case Action::SLOW:
			actionBox.set_active(ActionSelector::SLOW);
			break;
		case Action::TOGGLE_FAST_FORWARD:
			toggleBox.set_active(1);
		case Action::FAST_FORWARD:
			actionBox.set_active(ActionSelector::FFWD);
			break;
		case Action::TOGGLE_REWIND:
			toggleBox.set_active(1);
		case Action::REWIND:
			actionBox.set_active(ActionSelector::RWD);
			break;
		case Action::TOGGLE_MODIFIER:
			toggleBox.set_active(1);
		case Action::MODIFIER:
			actionBox.set_active(ActionSelector::MOD);
			break;

		case Action::SKIP:
			if (cmd.deciseconds >= 0) {
				actionBox.set_active(ActionSelector::SKIP_AHEAD);
				seconds.set_value(0.1 * (double)cmd.deciseconds);
			} else {
				actionBox.set_active(ActionSelector::SKIP_BACK);
				seconds.set_value(-0.1 * (double)cmd.deciseconds);
			}
			break;

		case Action::CHANGE_SLOW_SPEED:
			if (cmd.percent >= 0) {
				actionBox.set_active(ActionSelector::FASTER);
				percent.set_value((double)cmd.percent);
			} else {
				actionBox.set_active(ActionSelector::SLOWER);
				percent.set_value(-(double)cmd.percent);
			}
			break;

		default: break;

	}
	onActionChange(primary);
}

ButtonConfigRow::~ButtonConfigRow() {
	layout->remove(nameBox);
	layout->remove(isDown);
	layout->remove(primaryActionBox);
	layout->remove(secondaryActionBox);

	switch (p_ext) {
		case EXT_NONE: layout->remove(primaryEmpty); break;
		case EXT_TOGGLE: layout->remove(primaryToggleBox); break;
		case EXT_SECONDS: layout->remove(primarySecondsBox); break;
		case EXT_PERCENT: layout->remove(primaryPercentBox); break;
	}

	switch (s_ext) {
		case EXT_NONE: layout->remove(secondaryEmpty); break;
		case EXT_TOGGLE: layout->remove(secondaryToggleBox); break;
		case EXT_SECONDS: layout->remove(secondarySecondsBox); break;
		case EXT_PERCENT: layout->remove(secondaryPercentBox); break;
	}
}

/*
 * AxisConfigRow
 */
const int AxisConfigRow::CONFIG_INDEX = ConfigRowBase::P_ACTION_INDEX;
const int AxisConfigRow::CONFIG_WIDTH = 2*(ConfigRowBase::ACTION_WIDTH + ConfigRowBase::EXT_WIDTH);

void AxisConfigRow::connectSignals() {
	primaryActionBox.signal_changed().connect(sigc::mem_fun(*this, &AxisConfigRow::onPrimaryActionChange));
	secondaryActionBox.signal_changed().connect(sigc::mem_fun(*this, &AxisConfigRow::onSecondaryActionChange));
	name.signal_clicked().connect(sigc::mem_fun(*this, &AxisConfigRow::onNameClick));
	updatePedalDown.connect(sigc::mem_fun(*this, &AxisConfigRow::onPedalStatusChange));
}

FLATTEN void AxisConfigRow::onNameClick() {
	configureLock.lock();
	if (deadzone < 0) {
		// configure
		((ConfigWindow*)WindowList::config)->openAxisConfigurationWindow(axisNum);
	} else {
		constexpr int RECONFIGURE = 1;
		constexpr int UNCONFIGURE = 2;

		Gtk::MessageDialog question(*WindowList::config, "This axis has already been configured. Do you wish to reconfigure it, or do you want to forget its configuration (unconfigure)?", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
		question.add_button("Cancel", Gtk::RESPONSE_CANCEL);
		question.add_button("Reconfigure", RECONFIGURE);
		question.add_button("Unconfigure", UNCONFIGURE);

		int answer = question.run();

		if (answer == RECONFIGURE) {
			// reconfigure
			((ConfigWindow*)WindowList::config)->openAxisConfigurationWindow(axisNum);
		} else if (answer == UNCONFIGURE) {
			// unconfigure
			configureLock.unlock();
			configure(-1.0f, false);
			return;
		}
	}
	configureLock.unlock();
}

AxisConfigRow::AxisConfigRow(Gtk::Grid *parent, int i, int j) : ConfigRowBase(parent, i) {
	configLabel.set_label("Click the axis name to configure this axis.");
	configLabelBox.pack_start(configLabel, false, false);
	configLabelBox.pack_start(spacer, true, true);

	char nameText[17];
	std::sprintf(nameText, "Axis %d", j);
	name.set_label(Glib::ustring(nameText));
	parent->attach(name, NAME_INDEX, i, NAME_WIDTH, 1);

	parent->attach(isDownBar, DOWN_INDEX, i, DOWN_WIDTH, 1);

	parent->remove(primaryActionBox);
	parent->remove(primaryEmpty);
	parent->remove(secondaryActionBox);
	parent->remove(secondaryEmpty);
	parent->attach(configLabelBox, CONFIG_INDEX, i, CONFIG_WIDTH, 1);

	parent->show_all_children();

	deadzone = -1.0f;
	isInverted = false;
	axisNum = j-1;

	connectSignals();
}

AxisConfigRow::AxisConfigRow(Gtk::Grid *parent, int i, int j, const Action &action1, const Action &action2, float _deadzone, bool _isInverted) : ConfigRowBase(parent, i) {
	deadzone = _deadzone;
	isInverted = _isInverted;
	axisNum = j-1;

	configLabel.set_label("Click the axis name to configure this axis.");
	configLabelBox.pack_start(configLabel, false, false);
	configLabelBox.pack_start(spacer, true, true);

	char nameText[17];
	std::sprintf(nameText, "Axis %d", j);
	name.set_label(Glib::ustring(nameText));
	parent->attach(name, NAME_INDEX, i, NAME_WIDTH, 1);

	if (deadzone == -1.0f) { //unconfigured axis
		parent->attach(isDownBar, DOWN_INDEX, i, DOWN_WIDTH, 1);
		parent->remove(primaryActionBox);
		parent->remove(primaryEmpty);
		parent->remove(secondaryActionBox);
		parent->remove(secondaryEmpty);
		parent->attach(configLabelBox, CONFIG_INDEX, i, CONFIG_WIDTH, 1);
	} else {
		parent->attach(isDownLabel, DOWN_INDEX, i, DOWN_WIDTH, 1);
		isDownLabel.set_markup("<span color='#800000'>NO</span>");
		setAction(action1, true);
		setAction(action2, false);
	}

	parent->show_all_children();

	connectSignals();
}

void AxisConfigRow::onPedalStatusChange() {
	if (pedalDown) {
		isDownLabel.set_markup("<span color='#008000'>YES</span>");
	} else {
		isDownLabel.set_markup("<span color='#800000'>NO</span>");
	}
}

void AxisConfigRow::configure(float _deadzone, bool _isInverted) {
	configureLock.lock();
	if (deadzone < 0 && _deadzone >= 0) {
		/* Configure */
		layout->remove(isDownBar);
		layout->attach(isDownLabel, DOWN_INDEX, row, DOWN_WIDTH, 1);
		isDownLabel.set_markup("<span color='#800000'>NO</span>");

		layout->remove(configLabelBox);
		layout->attach(primaryActionBox, P_ACTION_INDEX, row, ACTION_WIDTH, 1);
		layout->attach(primaryEmpty, P_EXT_INDEX, row, EXT_WIDTH, 1);
		layout->attach(secondaryActionBox, S_ACTION_INDEX, row, ACTION_WIDTH, 1);
		layout->attach(secondaryEmpty, S_EXT_INDEX, row, EXT_WIDTH, 1);
	} else if (deadzone >= 0 && _deadzone < 0) {
		/* Unconfigure */
		layout->remove(isDownLabel);
		layout->attach(isDownBar, DOWN_INDEX, row, DOWN_WIDTH, 1);

		Action nothing;
		nothing.type = Action::NOOP;
		nothing.amount = 0;

		setAction(nothing, true);
		setAction(nothing, false);

		layout->remove(primaryActionBox);
		layout->remove(primaryEmpty);
		layout->remove(secondaryActionBox);
		layout->remove(secondaryEmpty);
		layout->attach(configLabelBox, CONFIG_INDEX, row, CONFIG_WIDTH, 1);
	}

	deadzone = _deadzone;
	isInverted = _isInverted;
	configureLock.unlock();

	layout->show_all_children();
}

AxisConfigRow::~AxisConfigRow() {
	layout->remove(name);

	if (deadzone < 0) {
		layout->remove(configLabelBox);
		layout->remove(isDownBar);
	} else {
		layout->remove(primaryActionBox);
		layout->remove(secondaryActionBox);
		layout->remove(isDownLabel);

		switch (p_ext) {
			case EXT_NONE: layout->remove(primaryEmpty); break;
			case EXT_TOGGLE: layout->remove(primaryToggleBox); break;
			case EXT_SECONDS: layout->remove(primarySecondsBox); break;
			case EXT_PERCENT: layout->remove(primaryPercentBox); break;
		}

		switch (s_ext) {
			case EXT_NONE: layout->remove(secondaryEmpty); break;
			case EXT_TOGGLE: layout->remove(secondaryToggleBox); break;
			case EXT_SECONDS: layout->remove(secondarySecondsBox); break;
			case EXT_PERCENT: layout->remove(secondaryPercentBox); break;
		}
	}
}

/*
 * ConfigWindow
 */

void ConfigWindow::onDeviceConnect(const PedalInfo &info, int port) {
	UDLLock.lock();
	UDLQueue.push(UpdateDeviceEvent{info, port, true});
	UDLLock.unlock();
	updateDeviceList.emit();
}

void ConfigWindow::onDeviceDisconnect(int port) {
	UDLLock.lock();
	UDLQueue.push(UpdateDeviceEvent{PedalInfo(), port, false});
	UDLLock.unlock();
	updateDeviceList.emit();
}

void ConfigWindow::onDeviceEvent(const PedalEvent &ev, int port) {
	eventLock.lock();
	if (port == currentDevice) {
		if (ev.type == PedalEvent::TYPE_BUTTON) {
			/* Button Press/Release */
			if (ev.index < buttonRows.size()) {
				buttonRows[ev.index]->updatePedalStatus(ev.isPressed > 0);
			}
		} else if (ev.type == PedalEvent::TYPE_AXIS) {
			/* Axis Event */
			if (ev.index < axisRows.size()) {
				axisRows[ev.index]->updatePedalStatus(ev.position);
			}

			if ((int)ev.index == axisBeingConfigured) acw.axisMoved(ev.position);
		}
	}
	eventLock.unlock();
}

void ConfigWindow::onDeviceListChange() {
	UDLLock.lock();
	while (!UDLQueue.empty()) {
		const UpdateDeviceEvent &ev = UDLQueue.front();

		if (ev.connect) {
			// Device connected
			assert(pedalInfo.count(ev.port) == 0);
			pedalInfo[ev.port] = ev.device;
			auto newEntry = devicesModel->append();
			newEntry->set_value(intColumnType, ev.port);
			newEntry->set_value(stringColumnType, Glib::ustring(ev.device.name));
			newEntry->set_value(boolColumnType, ev.device.isProtected);
		} else {
			// Device disconnected

			if (pedalInfo.count(ev.port)) {
				/*
				 * If the device we are currently configuring is disconnected,
				 * save the settings before doing anything else
				 */
				auto selection = devicesView.get_selection();
				if (selection->count_selected_rows() > 0 && selection->get_selected()->get_value(intColumnType) == currentDevice) {
					saveCurrentDeviceSettings();
				}

				/* Erase */
				pedalInfo.erase(ev.port);
				for (auto p = devicesModel->children().begin(); p != devicesModel->children().end(); p++) {
					if (p->get_value(intColumnType) == ev.port) {
						devicesModel->erase(p);
						break;
					}
				}
			}
		}

		UDLQueue.pop();
	}
	UDLLock.unlock();
}

void ConfigWindow::onDeviceSelected() {
	saveCurrentDeviceSettings();
	auto selection = devicesView.get_selection();
	eventLock.lock();
	if (selection->count_selected_rows() > 0) {
		for (ButtonConfigRow *row : buttonRows) delete row;
		for (AxisConfigRow *row : axisRows) delete row;
		buttonRows.clear();
		axisRows.clear();

		currentDevice = selection->get_selected()->get_value(intColumnType);

		assert(pedalInfo.count(currentDevice) == 1);
		const PedalInfo &info = pedalInfo[currentDevice];

		if (info.isProtected) {
			scrollArea.hide();
			selectDeviceLabel.hide();
			protectedDeviceLayout.show();
			protectedDeviceLayout.show_all_children();

			for (ButtonConfigRow *row : buttonRows) delete row;
			for (AxisConfigRow *row : axisRows) delete row;
			buttonRows.clear();
			axisRows.clear();
		} else {
			FootPedalConfiguration *conf = NULL;
			for (FootPedalConfiguration *c : configs) {
				if (info == c->info) {
					conf = c;
					break;
				}
			}

			if (conf != NULL) {
				for (unsigned short i = 0; i < info.getNumButtons(); i++) {
					buttonRows.push_back(new ButtonConfigRow(&confLayout, i+1, conf->primaryButtonActions[i], conf->secondaryButtonActions[i]));
				}
				for (unsigned short i = 0; i < info.getNumAxes(); i++) {
					double deadzone = -1.0;
					if (conf->deadzone[i] != 0x7fffffff) {
						deadzone = ((double)conf->deadzone[i] - (double)conf->info.axisMin[i]) / ((double)conf->info.axisMax[i] - (double)conf->info.axisMin[i]);
					}
					axisRows.push_back(new AxisConfigRow(&confLayout, info.getNumButtons()+i+1, i+1, conf->primaryAxisActions[i], conf->secondaryAxisActions[i], (float)deadzone, conf->isInverted[i]));
				}
			} else {
				for (unsigned short i = 0; i < info.getNumButtons(); i++) {
					buttonRows.push_back(new ButtonConfigRow(&confLayout, i+1));
				}
				for (unsigned short i = 0; i < info.getNumAxes(); i++) {
					axisRows.push_back(new AxisConfigRow(&confLayout, info.getNumButtons()+i+1, i+1));
				}
			}

			selectDeviceLabel.hide();
			protectedDeviceLayout.hide();
			scrollArea.show();
			confLayout.show();
			confLayout.show_all_children();
		}
	} else {
		currentDevice = -1;
		scrollArea.hide();
		selectDeviceLabel.show();
		protectedDeviceLayout.hide();
		for (ButtonConfigRow *row : buttonRows) delete row;
		for (AxisConfigRow *row : axisRows) delete row;
		buttonRows.clear();
		axisRows.clear();
	}
	eventLock.unlock();
}

void ConfigWindow::onHelpButtonPressed() const {
	WindowList::help->show();
	((HelpWindow*)WindowList::help)->showConfiguration();
}

void ConfigWindow::saveCurrentDeviceSettings() {
	if (currentDevice < 0 || pedalInfo.count(currentDevice) == 0 || !windowOpen) {
		return;
	}

	/* A device is configured if any axis is configured or any pedal has an action assigned to it */
	bool isConfigured = false;
	for (ButtonConfigRow *row : buttonRows) {
		if (row->getPrimaryAction().type != Action::NOOP || row->getSecondaryAction().type != Action::NOOP) {
			isConfigured = true;
			break;
		}
	}
	for (AxisConfigRow *row : axisRows) {
		if (row->isConfigured()) {
			isConfigured = true;
			break;
		}
	}

	/* We are going to build a new configuration from scratch, so delete the old one */

	for (auto c = configs.begin(); c != configs.end(); c++) {
		if ((*c)->info == pedalInfo[currentDevice]) {
			configs.erase(c);
			break;
		}
	}

	/* Create, update, or delete the device configuration */
	if (isConfigured) {
		FootPedalConfiguration *conf = new FootPedalConfiguration();
		configs.push_back(conf);

		conf->info = pedalInfo[currentDevice];

		conf->primaryButtonActions = new Action[conf->info.getNumButtons()];
		conf->secondaryButtonActions = new Action[conf->info.getNumButtons()];
		assert(conf->info.getNumButtons() == buttonRows.size());

		for (unsigned i = 0; i < conf->info.getNumButtons(); i++) {
			conf->primaryButtonActions[i] = buttonRows[i]->getPrimaryAction();
			conf->secondaryButtonActions[i] = buttonRows[i]->getSecondaryAction();
		}

		conf->isInverted = new bool[conf->info.getNumAxes()];
		conf->deadzone = new int[conf->info.getNumAxes()];
		conf->primaryAxisActions = new Action[conf->info.getNumAxes()];
		conf->secondaryAxisActions = new Action[conf->info.getNumAxes()];
		assert(conf->info.getNumAxes() == axisRows.size());

		for (unsigned i = 0; i < conf->info.getNumAxes(); i++) {
			if (axisRows[i]->getDeadzone() < 0.f) {
				conf->deadzone[i] = 0x7fffffff;
			} else {
				register const double deadzone = ((double)conf->info.axisMax[i] * (double)axisRows[i]->getDeadzone()) + ((double)conf->info.axisMin[i] * (1.0 - (double)axisRows[i]->getDeadzone()));
				if (deadzone <= (double)conf->info.axisMin[i]) {
					conf->deadzone[i] = conf->info.axisMin[i];
				} else if (deadzone >= (double)conf->info.axisMax[i] - 1.0) {
					conf->deadzone[i] = conf->info.axisMax[i] - 1;
				} else {
					conf->deadzone[i] = (int)deadzone;
				}
			}

			conf->isInverted[i] = axisRows[i]->getIsInverted();
			conf->primaryAxisActions[i] = axisRows[i]->getPrimaryAction();
			conf->secondaryAxisActions[i] = axisRows[i]->getSecondaryAction();
		}

	}

}

INLINE void ConfigWindow::openAxisConfigurationWindow(int axis) {
	eventLock.lock();
	axisBeingConfigured = axis;
	acw.start();
	eventLock.unlock();
}

void ConfigWindow::onAxisConfigured(float deadzone, bool isInverted) {
	eventLock.lock();
	if (axisBeingConfigured >= 0 && (size_t)axisBeingConfigured < axisRows.size()) {
		axisRows[axisBeingConfigured]->configure(deadzone, isInverted);
	}
	axisBeingConfigured = -1;
	eventLock.unlock();
}

void ConfigWindow::onAxisConfigurationCancelled() {
	eventLock.lock();
	axisBeingConfigured = -1;
	eventLock.unlock();
}

void ConfigWindow::onRevertPressed() {
	copyConfig(configs, prevConfigs); // configs = prevConfigs

	/* Update UI */
	windowOpen = false; // hack to prevent changes from being saved
	onDeviceSelected();
	windowOpen = true;
}
FLATTEN void ConfigWindow::onCancelPressed() {
	onRevertPressed();
	hide();
}
void ConfigWindow::onApplyPressed() {
	saveCurrentDeviceSettings();
	try {
		saveFootpedalConfiguration(configs);
		copyConfig(prevConfigs, configs); // prevConfigs = configs
	} catch (const std::ios_base::failure &ex) {
		Gtk::MessageDialog(*this, Glib::ustring(ex.what()), false, Gtk::MESSAGE_ERROR).run();
	}
}
void ConfigWindow::onOkayPressed() {
	saveCurrentDeviceSettings();
	try {
		saveFootpedalConfiguration(configs);
		copyConfig(prevConfigs, configs); // prevConfigs = configs
		hide();
	} catch (const std::ios_base::failure &ex) {
		Gtk::MessageDialog(*this, Glib::ustring(ex.what()), false, Gtk::MESSAGE_ERROR).run();
	}
}

void ConfigWindow::onPermissionChangeButtonPressed() {
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
		char *cmd = new char[307];
		std::snprintf(cmd, 307, "gksudo 'setfacl -m u:%s:r /dev/input/event%d'", username, currentDevice);

		if (std::system(cmd) == 0) {
			// success
			pclose(who);
			delete[] cmd;
			pedals->syncDevices();
			return;
		}
		delete[] cmd;
	}

	//failure
	pclose(who);
	Gtk::MessageDialog(Glib::ustring("Failed to set permissions on device."), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK).run();
}

/*
 * AxisConfigWindow
 */


void AxisConfigWindow::on_hide() {
	if (finished) {
		stepLock.lock();
		((ConfigWindow*)WindowList::config)->onAxisConfigured((max_value * deadzone) + (min_value * (1.f - deadzone)), min_value > max_value);
		stepLock.unlock();
		finished = false;
	} else {
		((ConfigWindow*)WindowList::config)->onAxisConfigurationCancelled();
	}
	Gtk::Window::on_hide();
}

void AxisConfigWindow::loadStep() {
	assert(step <= 4);
	/*
	 * Step 0: Confirm/Identify Pedal
	 * Step 1: Get minimum value
	 * Step 2: Get maximum value
	 * Step 3: Confirm min and max values
	 * Step 4: Set deadzone
	 */
	previous.set_visible(step > 0);
	next.set_label((step < 4) ? "Continue" : "Finish");
	deadzoneSlider.set_value(0.2);
	if (step == 4) {
		deadzone = 0.2f;
		bar.set_threshold(0.2);
	} else {
		deadzone = 1.0;
	}
	if (step < 2) {
		min_value = 0.f;
		max_value = 1.f;
	}
	bar.set_visible(step > 2 || step == 0);
	bar.set_value((axisPos - min_value) / (max_value - min_value));
	deadzoneSlider.set_visible(step == 4);

	switch (step) {
		case 0: message.set_label("Identify the physical foot pedal being configured. When you press the pedal being configured, the bar below will change. Once you have identified the pedal, click Continue.\nNOTE: Some footpedals, especially ones that use adaptors, incorrectly report themselves as having more pedals than they actually do. If this is the case with your footpedal, some axes cannot be configured because they do not correspond to any physical pedal. If this is the case, click Cancel and chose another button or axis."); break;
		case 1: message.set_label("Release the pedal and click the Continue button once your foot is off the pedal."); break;
		case 2: message.set_label("Press the pedal all the way down. Keep holding down the pedal as you click Continue."); break;
		case 3: message.set_label("Pedal range configured. Please verify that the pedal has been configured correctly. The bar below should be empty when you are not touching the pedal, and it should be full when you hold the pedal all the way down. If the pedal is working correctly, click Continue. Otherwise, click Back to reconfigure."); break;
		case 4: message.set_label("You may now configure the pedal's sensitivity, which determines how far down you need to press the pedal before OpenScribe considers it to be pressed. The bar below will be red when the pedal is considered to be up (unpressed) and will turn green when the pedal is considered to be down (pressed). If you do not like the default sensitivity, adjust it using the slider below the bar (left = more sensitive, right = less sensitive). When you are finished, click the Finish button."); break;
		default: break; //suppress compiler warning
	}

	if (step == 3 && min_value == max_value) {
		message.set_markup("<span color='#ff0000'>Configuration failed! No difference in axis position between configured released and pressed states. Click the Back button to retry. Ensure that your foot is completely off of the pedal when you click Continue the first time, and hold the pedal down when you click Continue the second time.</span>");
		next.set_sensitive(false);
		bar.set_visible(false);
	} else {
		next.set_sensitive(true);
	}
}

void AxisConfigWindow::axisMoved(float value) {
	stepLock.lock();
	axisPos = value;
	bar.set_value_async((axisPos - min_value) / (max_value - min_value));
	stepLock.unlock();
}


void AxisConfigWindow::nextStep() {
	stepLock.lock();
	if (step == 1) {
		min_value = axisPos;
	} else if (step == 2) {
		max_value = axisPos;
	} else if (step == 4) {
		stepLock.unlock();
		finished = true;
		hide();
		return;
	}
	step++;
	loadStep();
	stepLock.unlock();
}

void AxisConfigWindow::prevStep() {
	stepLock.lock();
	if (step-- == 3) step--;
	loadStep();
	stepLock.unlock();
}
