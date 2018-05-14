#include <gtkmm/application.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/window.h>

#include <fstream>
#include <vector>

#include "windowList.hpp"
#include "dictation.hpp"
#include "mainWindow.hpp"
#include "configWindow.hpp"
#include "optionsWindow.hpp"
#include "helpWindow.hpp"
#include "config.hpp"
#include "footPedal.hpp"

/* xxx remember to update changelog.hpp and version.hpp xxx */
#include "changelog.hpp"

int main(int argc, char *argv[]) {
	Glib::RefPtr<Gtk::Application> program = Gtk::Application::create("gtk.OpenScribe", Gio::APPLICATION_HANDLES_OPEN);

	Version lastUsed = getLastVersionUsed();
	if (lastUsed == Version(0) && QTranscribeInstalled()) {
		// If the user has qtranscribe installed, and this is their first time running OpenScribe, copy over their settings
		lastUsed = Version(0,0,15);
		saveOptions(loadOptions(lastUsed)); // update options from QTranscribe
	}
	saveVersionFile(CURRENT_VERSION);

	Dictation dict;
	Options opt = loadOptions();

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			try {
				dict.openFile(argv[i], opt);
			} catch (const std::runtime_error &ex) {
				Gtk::MessageDialog(Glib::ustring(ex.what()), false, Gtk::MESSAGE_ERROR).run();
				dict.closeFile();
			} catch (const std::invalid_argument &ex) {
				dict.closeFile();
			}
			break;
		}
	}

	std::vector<Glib::RefPtr<Gdk::Pixbuf> > icons;
	icons.reserve(5);
	icons.push_back(Gdk::Pixbuf::create_from_file(INSTALL_DIR "/OpenScribe/icons/OpenScribe_minimal.svg", 16, 16));
	icons.push_back(Gdk::Pixbuf::create_from_file(INSTALL_DIR "/OpenScribe/icons/OpenScribe.svg", 32, 32));
	icons.push_back(Gdk::Pixbuf::create_from_file(INSTALL_DIR "/OpenScribe/icons/OpenScribe.svg", 48, 48));
	icons.push_back(Gdk::Pixbuf::create_from_file(INSTALL_DIR "/OpenScribe/icons/OpenScribe.svg", 64, 64));
	icons.push_back(Gdk::Pixbuf::create_from_file(INSTALL_DIR "/OpenScribe/icons/OpenScribe.svg", 128, 128));
	Gtk::Window::set_default_icon_list(icons);

	FootPedalCoordinator FPC;

	WindowList::main = new MainWindow(&dict, opt, &FPC);
	WindowList::options = new OptionsWindow();
	WindowList::config = new ConfigWindow(&FPC);
	WindowList::help = new HelpWindow();

	ChangelogWindow changelog(lastUsed);
	if (lastUsed != CURRENT_VERSION && lastUsed != Version(0)) {
		changelog.show();
		Gtk::Application::create("gtk.OpenScribe", Gio::APPLICATION_HANDLES_OPEN)->run(changelog);
	}

	if (FPC.start(	MainWindow::onPedalEventAdaptor, ConfigWindow::onDeviceConnectAdaptor,
					ConfigWindow::onDeviceDisconnectAdaptor, ConfigWindow::onDeviceEventAdaptor) == false) {
		// Error loading footpedal configuration, but this is not a fatal error. (FPC still starts)
		Gtk::MessageDialog(Glib::ustring("Error loading footpedal configuration. You will need to reconfigure your footpedal(s)."), false, Gtk::MESSAGE_ERROR).run();
	}
	WindowList::main->show();
	if (lastUsed < Version(1,0,0)) {
		//First time running OpenScribe
		if (Gtk::MessageDialog(Glib::ustring("This is your first time using OpenScribe. Would you like to configure your footpedal(s) and/or hand control(s) now?"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true).run() == Gtk::RESPONSE_YES) {
			WindowList::config->show();
		}
	} else if (lastUsed < Version(1,1,1) && getFootpedalConfigurationFileVersion() < 2) {
		// Footpedal configuration file format changed in OpenScribe 1.1
		saveFootpedalConfiguration(std::vector<FootPedalConfiguration*>()); //clear the old file and replace it with a new (empty) one
		if (Gtk::MessageDialog(Glib::ustring("The OpenScribe 1.1 update changed the way USB input devices are read, so you will need to reconfigure your footpedal(s). Would you like to do this now?"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true).run() == Gtk::RESPONSE_YES) {
			WindowList::config->show();
		}
	}
	int exitStatus = program->run(*WindowList::main);
	FPC.stop();

	delete WindowList::help;
	delete WindowList::config;
	delete WindowList::options;
	delete WindowList::main;

	return exitStatus;
}
