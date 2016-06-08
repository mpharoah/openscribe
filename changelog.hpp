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
#include <gtkmm/textview.h>
#include <gtkmm/button.h>
#include <gtkmm/hvbox.h>
#include <gtkmm/scrolledwindow.h>

#include "version.hpp"
#include "windowList.hpp"

#define CHANGELOG_SECTION(version) \
	message->insert_with_tag(message->end(), Glib::ustring("\n" version "\n"), bold);

#define CHANGELOG_ENTRY(line) \
	message->insert(message->end(), "\t•\t"); \
	message->insert_with_tag(message->end(), Glib::ustring(line "\n"), indent);

class ChangelogWindow : public Gtk::Window {
  private:
	Gtk::ScrolledWindow scrollArea;
	Gtk::TextView messageView;
	Glib::RefPtr<Gtk::TextBuffer> message;
	Gtk::Button okayButton;

	Gtk::VBox rootLayout;
	Gtk::HBox buttonLayout;

  public:
	INLINE ChangelogWindow(const Version &since) : Gtk::Window() {
		set_title("OpenScribe Changelog");
		resize(1, 500);
		set_border_width(3);

		okayButton.set_label(Glib::ustring("Okay"));
		okayButton.signal_clicked().connect(sigc::mem_fun(*this, &ChangelogWindow::hide));

		scrollArea.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

		messageView.set_editable(false);
		messageView.set_cursor_visible(false);
		messageView.set_wrap_mode(Gtk::WRAP_NONE);
		message = Gtk::TextBuffer::create();
		message->set_text(Glib::ustring("You have updated to a new version of OpenScribe. Here are the changes since you last\nused OpenScribe:\n"));

		auto bold = message->create_tag();
		bold->property_weight_set() = true;
		bold->property_weight() = PANGO_WEIGHT_BOLD;

		auto indent = message->create_tag();
		indent->property_indent_set() = true;
		indent->property_indent() = 47;

		////////////////////////////////////////////////////////////////////////

		// put other versions here, with newer versions at the top

		/* Version 1.1-0ubuntu2 */
		if (since < Version(1,1,2)) {
			CHANGELOG_SECTION("OpenScribe 1.1-2")

			CHANGELOG_ENTRY("Fixed dependency issue that prevented OpenScribe from being installed on\n"
							"systems with a newer version of udev")
			CHANGELOG_ENTRY("Changed default audio chunk size from 15ms to 25ms to prevent audio crackling\n"
							"on slower systems")
			CHANGELOG_ENTRY("Changed error message when unable to load an audio file to be more informative")
		}

		/* Version 1.1-0ubuntu1 */
		if (since < Version(1,1,1)) {
			CHANGELOG_SECTION("OpenScribe 1.1")

			CHANGELOG_ENTRY("All footpedals should now be supported. If your footpedal does not work with\n"
							"OpenScribe, please e-mail the developer with a bug report.")
			CHANGELOG_ENTRY("Updated help section")
		}

		/* Version 1.0-0ubuntu7 */
		if (since < Version(1,0,7)) {
			CHANGELOG_SECTION("OpenScribe 1.0-7")

			CHANGELOG_ENTRY("Fixed an issue where OpenScribe would not compile when using gcc version 4.7 or\n"
							"later.")
			CHANGELOG_ENTRY("OpenScribe is now published for Trusty and Utopic in addition to Precise.")
			CHANGELOG_ENTRY("If you are using Precise (Ubuntu 12.04), this update will not affect you.")
		}

		/* Version 1.0-0ubuntu6 */
		if (since < Version(1,0,6)) {
			CHANGELOG_SECTION("OpenScribe 1.0-6")

			CHANGELOG_ENTRY("Fixed a potential stack overflow when slowing down audio.")
		}

		/* Version 1.0-0ubuntu5 (only present in PPA build) */
		if (since < Version(1,0,5)) {
			CHANGELOG_SECTION("OpenScribe 1.0-5")

			CHANGELOG_ENTRY("Fixed a minor memory leak when closing the configuration window while multiple\n"
							"devices are connected.")
			CHANGELOG_ENTRY("Added Revert, Cancel, Apply, and Okay buttons to configuration window.")
			CHANGELOG_ENTRY("Closing the configuration window now asks if you want to save changes instead\n"
							"of always saving them. (No message is shown if you have not made any changes.)")
			CHANGELOG_ENTRY("Name of audio file currently playing is ellipsized if it is too long. (This prevents\n"
							"the main window from being resized and adding ugly empty space.)")
		}

		/* Version 1.0-0ubuntu4 (only present in PPA build) */
		if (since < Version(1,0,4)) {
			CHANGELOG_SECTION("OpenScribe 1.0-4")

			CHANGELOG_ENTRY("Fixed a bug where applying changes to options while an audio file is loaded\n"
							"would result in a freeze.")
		}

		/* Version 1.0-0ubuntu3 (package fix-- only present in PPA build) */
		if (since < Version(1,0,3)) {
			CHANGELOG_SECTION("OpenScribe 1.0-3")

			CHANGELOG_ENTRY("Fixed missing .desktop file in Debian package. OpenScribe should now be added\n"
							"to your application menu, and your file browser should now add OpenScribe to\n"
							"the list of programs that can open audio files.")
		}

		/* Version 1.0 (Updating from QTranscribe */

		if (since < Version(1,0,0)) {
			CHANGELOG_SECTION("OpenScribe 1.0 (upgrading from qtranscribe 1.0-15)")

			CHANGELOG_ENTRY("Switched GUI from QT to GTK+ 3.0")
			CHANGELOG_ENTRY("Support for footpedals whose pedals are axes rather than buttons")
			CHANGELOG_ENTRY("Support for saving the configuration of more than one footpedal")
			CHANGELOG_ENTRY("OpenScribe allows both a footpedal and a hand control (or multiple footpedals)\n"
							"to be used at the same time")
			CHANGELOG_ENTRY("Can now increase and decrease the slow speed with a footpedal")
			CHANGELOG_ENTRY("Can now assign two actions to each footpedal. If a pedal's action is set to\n"
							"\"Modifier Pedal\", and it is depressed, other pedals will perform their second\n"
							"action")
			CHANGELOG_ENTRY("Added help section")
			CHANGELOG_ENTRY("Fixed memory leak caused by QT")
			CHANGELOG_ENTRY("Fixed rare crash when skipping back slowed audio")
			CHANGELOG_ENTRY("Fixed bug where, sometimes after opening a new dictation while another was\n"
							"already open, playback would skip to the beginning of the file every time the\n"
							"audio was paused.")
			CHANGELOG_ENTRY("Fixed bug where audio position slider would not always update correctly")
			CHANGELOG_ENTRY("Fast forward and rewind sound effect volume decreased")
			CHANGELOG_ENTRY("New application icon")
			CHANGELOG_ENTRY("Added in-application changelog")
			CHANGELOG_ENTRY("Miscellaneous bugfixes and optimizations")
		}

		////////////////////////////////////////////////////////////////////////

		Pango::TabArray TA(2, true);
		TA.set_tab(0, Pango::TAB_LEFT, 23);
		TA.set_tab(1, Pango::TAB_LEFT, 47);

		auto tabs = message->create_tag();
		tabs->property_tabs_set() = true;
		tabs->property_tabs() = TA;

		message->apply_tag(tabs, message->begin(), message->end());

		messageView.set_buffer(message);
		add(rootLayout);
			rootLayout.pack_start(scrollArea, true, true);
				scrollArea.add(messageView);
			rootLayout.pack_start(buttonLayout, false, false);
				buttonLayout.pack_end(okayButton, false, false);
		show_all_children();
	}
	virtual ~ChangelogWindow() {}


};


#endif /* CHANGELOG_HPP_ */
