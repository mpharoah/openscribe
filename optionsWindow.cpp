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

#include "optionsWindow.hpp"

#include <gtkmm/messagedialog.h>

#include <cmath>
#include <ios>

#include "windowList.hpp"
#include "mainWindow.hpp"

void OptionsWindow::applyOptions() const {
	Options options;
	options.skipBackOnPlay = (unsigned)(1000.0 * (skipBackCheckbox.get_active() ? skipBackSpinner.get_value() : 0.0));
	options.playSoundEffects = soundEffectsCheckbox.get_active();
	options.rewindSpeed = (unsigned short)(1 << (int)rwdSlider.get_value());
	options.fastForwardSpeed = (unsigned short)(1 << (int)ffwdSlider.get_value());
	options.slowSpeed = (float)slowSlider.get_value();
	options.chunkSize = (unsigned)chunkSizeSlider.get_value();
	options.historySize = (unsigned)historySlider.get_value();
	options.preloadSize = (unsigned)preloadSlider.get_value();

	((MainWindow*)WindowList::main)->updateOptions(options);

	try {
		saveOptions(options);
	} catch (const std::ios_base::failure &ex) {
		Gtk::MessageDialog(Glib::ustring("Error: Could not save options to file. Do you have write permissions on your .conf directory?"), false, Gtk::MESSAGE_ERROR).run();
	}
}

void OptionsWindow::on_show() {
	const Options options = ((MainWindow*)WindowList::main)->getOptions();
	skipBackCheckbox.set_active(options.skipBackOnPlay != 0);
	skipBackSpinner.set_sensitive(options.skipBackOnPlay != 0);
	skipBackSpinner.set_value(0.001 * (double)options.skipBackOnPlay);
	soundEffectsCheckbox.set_active(options.playSoundEffects);
	rwdSlider.set_value(std::log2((double)options.rewindSpeed));
	ffwdSlider.set_value(std::log2((double)options.fastForwardSpeed));
	slowSlider.set_value(options.slowSpeed);
	chunkSizeSlider.set_value((double)options.chunkSize);
	historySlider.set_value((double)options.historySize);
	preloadSlider.set_value((double)options.preloadSize);
	Gtk::Window::on_show();
}
