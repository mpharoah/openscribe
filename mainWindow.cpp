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

#include "mainWindow.hpp"

#include <gtkmm/filechooserdialog.h>
#include <gtkmm/messagedialog.h>
#include <cassert>

#include "footPedal.hpp"

bool MainWindow::updatePosition() {
	unsigned seconds = player->getPositionMilliseconds() / 1000u;
	char time[9]; std::snprintf(time, 9, "%2u:%02u", seconds / 60u, seconds % 60u);
	positionLabel.set_text(time);

	if (!adjustingSlider) slider.set_value(player->getPositionPercentage());

	// update the play button if we reach the end of the file
	if (playButton.get_image() == &pauseIcon && player->isPaused()) {
		playButton.set_image(playIcon);
	}

	return true;
}

void MainWindow::updateNameAndDurationLabels() {
	char *fileName; player->getFilename(&fileName);
	if (fileName != NULL) {
		int start = 0, end = -1;
		while (fileName[++end] != '\0') {
			if (fileName[end] == '/') start = end+1;
		}

		char time[9];
		register const unsigned seconds = player->getLengthMilliseconds() / 1000u;
		std::snprintf(time, 9, "%2u:%02u", seconds / 60u, seconds % 60u);

		audioFileLabel.set_text(&fileName[start]);
		lengthLabel.set_text(time);
		delete[] fileName;
	} else {
		audioFileLabel.set_text("No audio file loaded.");
		lengthLabel.set_text(" 0:00");
	}
}

void MainWindow::playButtonPressed() {
	if (player->isPaused()) {
		player->skipBack((int)options.skipBackOnPlay);
		player->play();
		playButton.set_image(pauseIcon);
	} else {
		player->pause();
		playButton.set_image(playIcon);
	}
}
void MainWindow::restartButtonPressed() {
	player->setPositionMilliseconds(0);
}
void MainWindow::skipBack10ButtonPressed() {
	player->skipBack(10000);
}
void MainWindow::skipBack5ButtonPressed() {
	player->skipBack(5000);
}
void MainWindow::slowButtonPressed() {
	player->toggleSlow();
}
void MainWindow::rewindButtonPressed() {
	player->startRewind();
}
void MainWindow::rewindButtonReleased() {
	player->stopRewind();
}
void MainWindow::fastForwardButtonPressed() {
	player->startFastForward();
}
void MainWindow::fastForwardButtonReleased() {
	player->stopFastForward();
}

void MainWindow::helpButtonPressed() {
	WindowList::help->show();
}
void MainWindow::openButtonPressed() {
	Gtk::FileChooserDialog fchooser("Select an audio file", Gtk::FILE_CHOOSER_ACTION_OPEN);
	fchooser.set_transient_for(*this);

	Glib::RefPtr<Gtk::FileFilter> filter = Gtk::FileFilter::create();
	filter->set_name("Supported Audio Files");
	filter->add_mime_type("application/ogg");
	filter->add_mime_type("audio/aiff");
	filter->add_mime_type("audio/basic");
	filter->add_mime_type("audio/flac");
	filter->add_mime_type("audio/ogg");
	filter->add_mime_type("audio/x-aiff");
	filter->add_mime_type("audio/x-mp3");
	filter->add_mime_type("audio/x-wav");
	filter->add_pattern("*.ogg");
	filter->add_pattern("*.flac");
	filter->add_pattern("*.wav");
	filter->add_pattern("*.snd");
	filter->add_pattern("*.au");
	filter->add_pattern("*.aiff");
	filter->add_pattern("*.aif");
	filter->add_pattern("*.aifc");
	filter->add_pattern("*.mp3");
	filter->add_pattern("*.mp2");
	filter->add_pattern("*.iff");
	filter->add_pattern("*.8svx");
	filter->add_pattern("*.pvf");
	filter->add_pattern("*.caf");
	filter->add_pattern("*.voc");
	filter->add_pattern("*.vox");
	filter->add_pattern("*.cda");
	filter->add_pattern("*.avr");
	filter->add_pattern("*.cvs");
	filter->add_pattern("*.vms");
	filter->add_pattern("*.gsm");
	filter->add_pattern("*.htk");
	filter->add_pattern("*.hcom");
	fchooser.add_filter(filter);

	Glib::RefPtr<Gtk::FileFilter> all = Gtk::FileFilter::create();
	all->set_name("All Files");
	all->add_pattern("*");
	fchooser.add_filter(all);

	fchooser.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	fchooser.add_button("_Open", Gtk::RESPONSE_OK);

	if (fchooser.run() == Gtk::RESPONSE_OK) {
		player->closeFile();
		try {
			player->openFile(fchooser.get_filename().c_str(), options);
		} catch (const std::exception &ex) {
			player->closeFile();
			Gtk::MessageDialog(*this, Glib::ustring(ex.what()), false, Gtk::MESSAGE_ERROR).run();
		}
		updateNameAndDurationLabels();
	}
	playButton.set_image(playIcon);
}
FLATTEN void MainWindow::optionsButtonPressed() {
	WindowList::options->show();
}
FLATTEN void MainWindow::pedalConfigButtonPressed() {
	WindowList::config->show();
}
void MainWindow::showSlowSpeedButtonPressed() {
	if (slowSpeedPopout.get_visible()) {
		slowSpeedPopout.hide();
	} else {
		slowSpeedPopout.show_all();
	}
	resize(1,1);
}

bool MainWindow::sliderPressed(__attribute__((unused)) GdkEventButton *ev) {
	adjustingSlider = true;
	wasPlaying = !player->isPaused();
	player->pause();
	playButton.set_image(playIcon);
	return false;
}
bool MainWindow::sliderMoved(__attribute__((unused)) Gtk::ScrollType type, double value) {
	player->setPositionPercentage(value);
	return true;
}
bool MainWindow::sliderReleased(__attribute__((unused)) GdkEventButton *ev) {
	adjustingSlider = false;
	if (wasPlaying) player->play();
	return false;
}

void MainWindow::updateOptions(const Options &opt) {
	player->setOptions(opt);
	slowSpeedSlider.set_value(opt.slowSpeed);
	player->setSlowSpeed(opt.slowSpeed);
	options = opt;
}

void MainWindow::onPedalEvent(Action cmd) {
	/* Perform action */
	switch (cmd.type) {
		case Action::PLAY:			player->play(); player->skipBack((int)options.skipBackOnPlay); break;
		case Action::PAUSE:			player->pause();		break;
		case Action::TOGGLE_PLAY:	player->togglePlay(); if (!player->isPaused()) player->skipBack((int)options.skipBackOnPlay); break;
		case Action::SLOW:			player->slow(); player->play(); player->skipBack((int)options.skipBackOnPlay); break;
		case Action::UNSLOW:		player->unslow(); player->pause(); break;
		case Action::TOGGLE_SLOW:	player->toggleSlow(); break; //toggling SLOW should not play, pause, or skip back
		case Action::REWIND:		player->startRewind();	break;
		case Action::STOP_REWIND:	player->stopRewind();	break;
		case Action::TOGGLE_REWIND:	player->toggleRewind();	break;
		case Action::FAST_FORWARD:	player->startFastForward(); break;
		case Action::STOP_FAST_FORWARD: player->stopFastForward(); break;
		case Action::TOGGLE_FAST_FORWARD: player->toggleFastForward(); break;
		case Action::SKIP:			player->skipForward(100 * (int)cmd.deciseconds); return;
		case Action::RESTART:		player->setPositionMilliseconds(0); return;
		case Action::CHANGE_SLOW_SPEED:
			newSlowSpeed = (unsigned char) (100.f * player->increaseSlowSpeed(0.01f * (float)cmd.percent));
			refreshSlowSpeed.emit(); break;
		default: return;
	}

	/* Tell the UI to update accordingly */
	actionLock.lock();
	actionQueue.push(cmd.type);
	actionLock.unlock();
	updateUI.emit();
}

/*
 * Update the UI as appropriate when the user performs an action
 * with the footpedal. This function only updates the UI. The
 * actual actions are performed in the onPedalEvent function above
 */
void MainWindow::onUpdateRequest() {
	actionLock.lock();
	while (!actionQueue.empty()) {
		auto event = actionQueue.front();
		actionQueue.pop();

		switch (event) {
			case Action::PLAY:
			case Action::TOGGLE_PLAY:
			case Action::PAUSE:
				playButton.set_image(player->isPaused() ? playIcon : pauseIcon);
				break;

			case Action::SLOW:
			case Action::UNSLOW:
			case Action::TOGGLE_SLOW:
				if (player->isSlowed()) {
					slowButton.set_state_flags(Gtk::StateFlags::STATE_FLAG_ACTIVE);
				} else {
					slowButton.unset_state_flags(Gtk::StateFlags::STATE_FLAG_ACTIVE);
				}
				break;

			case Action::FAST_FORWARD:
			case Action::STOP_FAST_FORWARD:
			case Action::TOGGLE_FAST_FORWARD:
				if (player->isFastForwarding()) {
					fastForwardButton.set_state_flags(Gtk::StateFlags::STATE_FLAG_ACTIVE);
				} else {
					fastForwardButton.unset_state_flags(Gtk::StateFlags::STATE_FLAG_ACTIVE);
				}
				break;

			case Action::REWIND:
			case Action::STOP_REWIND:
			case Action::TOGGLE_REWIND:
				if (player->isRewinding()) {
					rewindButton.set_state_flags(Gtk::StateFlags::STATE_FLAG_ACTIVE);
				} else {
					rewindButton.unset_state_flags(Gtk::StateFlags::STATE_FLAG_ACTIVE);
				}
				break;
			default: break;
		}
	}
	actionLock.unlock();
}

void MainWindow::onStreamError() const {
	Gtk::MessageDialog(Glib::ustring("An error occurred while reading the audio file."), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, false).run();
}

// this function is called when the slow speed is changed by a footpedal (using the increase/decrease slow speed action)
void MainWindow::onSlowSpeedChanged() {
	register const double speed = 0.01 * (double)newSlowSpeed;
	options.slowSpeed = (float)speed;
	slowSpeedSlider.set_value(speed);
}

// ...and this one is called when the slow speed UI slider is moved
bool MainWindow::onSlowSpeedSliderMoved(__attribute__((unused)) Gtk::ScrollType type, double value) {
	options.slowSpeed = (float)value;
	player->setSlowSpeed((float)value);
	return true;
}
