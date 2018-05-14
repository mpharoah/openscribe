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
	options.latency = (unsigned)latencySlider.get_value();
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
	latencySlider.set_value((double)options.latency);
	historySlider.set_value((double)options.historySize);
	preloadSlider.set_value((double)options.preloadSize);
	Gtk::Window::on_show();
}
