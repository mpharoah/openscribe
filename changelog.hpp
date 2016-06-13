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

#ifndef CHANGELOG_HPP_
#define CHANGELOG_HPP_

#include <gtkmm/window.h>
#include <gtkmm/button.h>
#include <gtkmm/hvbox.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/label.h>

#include "version.hpp"
#include "windowList.hpp"

class ChangelogWindow : public Gtk::Window {
  private:
	Gtk::ScrolledWindow scrollArea;
	Gtk::Button okayButton;

	Gtk::VBox rootLayout;
	Gtk::VBox changeList;
	Gtk::HBox buttonLayout;
	
	Gtk::Label updateMessage;
	
	std::vector<Gtk::Label*> labels;
	std::vector<Gtk::HBox*> changes;
	
	INLINE void addChangelogSection(char const *version) {
		Glib::ustring versionString("<b>OpenScribe ");
		versionString += version;
		versionString += "</b>";
		
		Gtk::Label *spacer = new Gtk::Label(Glib::ustring(" "));
		changeList.pack_start(*spacer, false, false);
		labels.push_back(spacer);
		
		Gtk::Label *versionLabel = new Gtk::Label();
		versionLabel->set_markup(versionString);
		versionLabel->set_alignment(Gtk::ALIGN_START);
		changeList.pack_start(*versionLabel, false, false);
		labels.push_back(versionLabel);
	}
	
	INLINE void addChangelogEntry(char const *change) {
		Gtk::HBox *entry = new Gtk::HBox();
		changeList.pack_start(*entry, false, false);
		changes.push_back(entry);
		
		Gtk::Label *bullet = new Gtk::Label(Glib::ustring("  •  "));
		bullet->set_valign(Gtk::ALIGN_START);
		entry->pack_start(*bullet, false, false);
		labels.push_back(bullet);
		
		Gtk::Label *changeLabel = new Gtk::Label(Glib::ustring(change));
		changeLabel->set_line_wrap(true);
		changeLabel->set_alignment(Gtk::ALIGN_START);
		entry->pack_start(*changeLabel, true, true);
		labels.push_back(changeLabel);
	}

  public:
	INLINE ChangelogWindow(const Version &since) : Gtk::Window() {
		set_title("OpenScribe Changelog");
		resize(610, 500);
		set_border_width(3);

		okayButton.set_label(Glib::ustring("Okay"));
		okayButton.signal_clicked().connect(sigc::mem_fun(*this, &ChangelogWindow::hide));

		scrollArea.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
		
		updateMessage.set_text(Glib::ustring("You have updated to a new version of OpenScribe. Here are the changes since you last used OpenScribe:"));
		updateMessage.set_alignment(Gtk::ALIGN_START);
		updateMessage.set_line_wrap(true);
		changeList.pack_start(updateMessage, false, false);

		// put other versions here, with newer versions at the top

		/* Version 1.2.0 */
		if (since < Version(1,2,0)) {
			addChangelogSection("1.2.0");

			addChangelogEntry("Fixed audio crackling in Ubuntu 16.04");
			addChangelogEntry("Changed audio buffer settings to fix buffer underruns");
			addChangelogEntry("Fixed freeze when opening OpenScribe with a command line argument");
			addChangelogEntry("Fixed freeze when opening OpenScribe from the Open With dialog in a file browser");
			addChangelogEntry("Fixed changelog window to have proper word wrapping");
		}

		/* Version 1.1.2 */
		if (since < Version(1,1,2)) {
			addChangelogSection("1.1.2");

			addChangelogEntry("Fixed dependency issue that prevented OpenScribe from being installed on systems with a newer version of udev");
			addChangelogEntry("Changed default audio latency from 15ms to 25ms to prevent audio crackling on slower systems");
			addChangelogEntry("Changed error message when unable to load an audio file to be more informative");
		}

		/* Version 1.1.0 */
		if (since < Version(1,1,0)) {
			addChangelogSection("1.1.0");

			addChangelogEntry("All footpedals should now be supported. If your footpedal does not work with OpenScribe, please e-mail the developer with a bug report.");
			addChangelogEntry("Updated help section");
		}

		/* Version 1.0.7 */
		if (since < Version(1,0,7)) {
			addChangelogSection("1.0.7");

			addChangelogEntry("Fixed an issue where OpenScribe would not compile when using gcc version 4.7 or later.");
			addChangelogEntry("OpenScribe is now published for Trusty and Utopic in addition to Precise.");
			addChangelogEntry("If you are using Precise (Ubuntu 12.04), this update will not affect you.");
		}

		/* Version 1.0.6 */
		if (since < Version(1,0,6)) {
			addChangelogSection("1.0.6");

			addChangelogEntry("Fixed a potential stack overflow when slowing down audio.");
		}

		/* Version 1.0.5 */
		if (since < Version(1,0,5)) {
			addChangelogSection("1.0.5");

			addChangelogEntry("Fixed a minor memory leak when closing the configuration window while multiple devices are connected.");
			addChangelogEntry("Added Revert, Cancel, Apply, and Okay buttons to configuration window.");
			addChangelogEntry("Closing the configuration window now asks if you want to save changes instead of always saving them. (No message is shown if you have not made any changes.)");
			addChangelogEntry("Name of audio file currently playing is ellipsized if it is too long. (This prevents the main window from being resized and adding ugly empty space.)");
		}

		/* Version 1.0.4 */
		if (since < Version(1,0,4)) {
			addChangelogSection("1.0.4");

			addChangelogEntry("Fixed a bug where applying changes to options while an audio file is loaded would result in a freeze.");
		}

		/* Version 1.0.3 */
		if (since < Version(1,0,3)) {
			addChangelogSection("1.0.3");

			addChangelogEntry("Fixed missing .desktop file in Debian package. OpenScribe should now be added to your application menu, and your file browser should now add OpenScribe to the list of programs that can open audio files.");
		}

		/* Version 1.0 (Updating from QTranscribe) */

		if (since < Version(1,0,0)) {
			addChangelogSection("1.0 (upgrading from QTranscribe 1.0.15)");

			addChangelogEntry("Switched GUI from QT to GTK+ 3.0");
			addChangelogEntry("Support for footpedals whose pedals are axes rather than buttons");
			addChangelogEntry("Support for saving the configuration of more than one footpedal");
			addChangelogEntry("OpenScribe allows both a footpedal and a hand control (or multiple footpedals) to be used at the same time");
			addChangelogEntry("Can now increase and decrease the slow speed with a footpedal");
			addChangelogEntry("Can now assign two actions to each footpedal. If a pedal's action is set to \"Modifier Pedal\", and it is depressed, other pedals will perform their second action");
			addChangelogEntry("Added help section");
			addChangelogEntry("Fixed memory leak caused by QT");
			addChangelogEntry("Fixed rare crash when skipping back slowed audio");
			addChangelogEntry("Fixed bug where, sometimes after opening a new dictation while another was already open, playback would skip to the beginning of the file every time the audio was paused.");
			addChangelogEntry("Fixed bug where audio position slider would not always update correctly");
			addChangelogEntry("Fast forward and rewind sound effect volume decreased");
			addChangelogEntry("New application icon");
			addChangelogEntry("Added in-application changelog");
			addChangelogEntry("Miscellaneous bugfixes and optimizations");
		}

		add(rootLayout);
			rootLayout.pack_start(scrollArea, true, true);
				scrollArea.add(changeList);
			rootLayout.pack_start(buttonLayout, false, false);
				buttonLayout.pack_end(okayButton, false, false);
		show_all_children();
	}
	
	virtual ~ChangelogWindow() {
		for (Gtk::Label *label : labels) delete label;
		for (Gtk::HBox *change : changes) delete change;
	}


};


#endif /* CHANGELOG_HPP_ */
