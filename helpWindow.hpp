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

#ifndef HELPWINDOW_HPP_
#define HELPWINDOW_HPP_

#include <gtkmm/window.h>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textview.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/iconinfo.h>
#include <gtkmm/texttag.h>
#include <gtkmm/treepath.h>
#include <gdkmm/pixbuf.h>

#include <cassert>

#include "attributes.hpp"
#include "version.hpp"

class HelpWindow : public Gtk::Window {
  private:
	Gtk::Paned rootLayout;
	Gtk::TreeView explorer;
	Gtk::ScrolledWindow helpView;
	Gtk::TextView help;

	Glib::RefPtr<Gtk::TextBuffer> about;
	Glib::RefPtr<Gtk::TextBuffer> usage;
	Glib::RefPtr<Gtk::TextBuffer> configuration;

	Glib::RefPtr<Gtk::TreeStore> contents;
	Gtk::TreeModel::ColumnRecord cr;
	Gtk::TreeModelColumn<Glib::ustring> stringColumn;
	Gtk::TreeModelColumn<Glib::RefPtr<Gtk::TextBuffer> > chapterColumn;
	Gtk::TreeModelColumn<Glib::RefPtr<Gtk::TextBuffer::Mark> > sectionColumn;

	Glib::RefPtr<Gtk::TextBuffer::Mark> mark_about;
	Glib::RefPtr<Gtk::TextBuffer::Mark> mark_basicUsage, mark_buttons;
	Glib::RefPtr<Gtk::TextBuffer::Mark> mark_supportedDevices, mark_modifierPedal, mark_actions, mark_axes;

	void onSectionSelected() {
		auto selection = explorer.get_selection();
		if (selection->count_selected_rows() > 0) {
			assert(selection->count_selected_rows() == 1);
			help.set_buffer(selection->get_selected()->get_value(chapterColumn));
			help.scroll_to(selection->get_selected()->get_value(sectionColumn), 0.1, 0.0, 0.0);
		}
	}

  public:
	HelpWindow() {
		set_title("OpenScribe Help");
		resize(800, 600);

		Gtk::IconInfo helpIconInfo = Gtk::IconTheme::get_default()->lookup_icon("help", 16, Gtk::ICON_LOOKUP_FORCE_SIZE);
		if (!helpIconInfo) helpIconInfo = Gtk::IconTheme::get_default()->lookup_icon("help-browser", 16, Gtk::ICON_LOOKUP_FORCE_SIZE);
		if (helpIconInfo) {
			set_icon(helpIconInfo.load_icon());
		} else {
			set_icon_from_file(INSTALL_DIR "/OpenScribe/icons/fallback/help.svg");
		}

		initMarks();
		setupAboutText();
		setupUsageText();
		setupConfigurationText();
		setupHelpContents();

		rootLayout.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
		helpView.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_ALWAYS);
		help.set_editable(false);
		help.set_cursor_visible(false);
		help.set_wrap_mode(Gtk::WRAP_WORD);
		helpView.set_min_content_width(320);
		helpView.set_min_content_height(240);

		add(rootLayout);
			rootLayout.pack1(explorer, Gtk::FILL);
			rootLayout.pack2(helpView, Gtk::EXPAND);
				helpView.add(help);

		help.set_buffer(about);
		show_all_children();
	}
	~HelpWindow() {}

	inline void showAbout() { help.set_buffer(about); explorer.get_selection()->select(Gtk::TreePath("0")); }
	inline void showUsage() { help.set_buffer(usage); explorer.get_selection()->select(Gtk::TreePath("1")); }
	inline void showConfiguration() { help.set_buffer(configuration); explorer.get_selection()->select(Gtk::TreePath("2")); }

  private:
	INLINE void initMarks() {
		mark_about = Gtk::TextBuffer::Mark::create(true);
		mark_basicUsage = Gtk::TextBuffer::Mark::create(true);
		mark_buttons = Gtk::TextBuffer::Mark::create(true);
		mark_supportedDevices = Gtk::TextBuffer::Mark::create(true);
		mark_modifierPedal = Gtk::TextBuffer::Mark::create(true);
		mark_actions = Gtk::TextBuffer::Mark::create(true);
		mark_axes = Gtk::TextBuffer::Mark::create(true);
	}

	INLINE void setupHelpContents() {
		cr.add(chapterColumn);
		cr.add(sectionColumn);
		cr.add(stringColumn);
		contents = Gtk::TreeStore::create(cr);

		auto _about = contents->append();
		_about->set_value(stringColumn, Glib::ustring("About"));
		_about->set_value(chapterColumn, about);
		_about->set_value(sectionColumn, mark_about);

		auto _usage = contents->append();
		_usage->set_value(stringColumn, Glib::ustring("Basic Usage"));
		_usage->set_value(chapterColumn, usage);
		_usage->set_value(sectionColumn, mark_basicUsage);

			auto _basicUsage = contents->append(_usage->children());
			_basicUsage->set_value(stringColumn, Glib::ustring("Using OpenScribe"));
			_basicUsage->set_value(chapterColumn, usage);
			_basicUsage->set_value(sectionColumn, mark_basicUsage);

			auto _buttons = contents->append(_usage->children());
			_buttons->set_value(stringColumn, Glib::ustring("Buttons"));
			_buttons->set_value(chapterColumn, usage);
			_buttons->set_value(sectionColumn, mark_buttons);

		auto _configuration = contents->append();
		_configuration->set_value(stringColumn, Glib::ustring("Footpedal Configuration"));
		_configuration->set_value(chapterColumn, configuration);
		_configuration->set_value(sectionColumn, mark_supportedDevices);

			auto _supportedDevices = contents->append(_configuration->children());
			_supportedDevices->set_value(stringColumn, Glib::ustring("Supported Devices"));
			_supportedDevices->set_value(chapterColumn, configuration);
			_supportedDevices->set_value(sectionColumn, mark_supportedDevices);

			auto _modifierPedal = contents->append(_configuration->children());
			_modifierPedal->set_value(stringColumn, Glib::ustring("Primary and Secondary Actions"));
			_modifierPedal->set_value(chapterColumn, configuration);
			_modifierPedal->set_value(sectionColumn, mark_modifierPedal);

			auto _actions = contents->append(_configuration->children());
			_actions->set_value(stringColumn, Glib::ustring("List of Actions"));
			_actions->set_value(chapterColumn, configuration);
			_actions->set_value(sectionColumn, mark_actions);

			auto _axes = contents->append(_configuration->children());
			_axes->set_value(stringColumn, Glib::ustring("Configuring Axes"));
			_axes->set_value(chapterColumn, configuration);
			_axes->set_value(sectionColumn, mark_axes);

		explorer.get_selection()->signal_changed().connect(sigc::mem_fun(*this, &HelpWindow::onSectionSelected));
		explorer.set_headers_visible(false);
		explorer.set_rubber_banding(false);
		explorer.get_selection()->set_mode(Gtk::SelectionMode::SELECTION_SINGLE);
		explorer.append_column(Glib::ustring("Contents"), stringColumn);
		explorer.set_model(contents);

	}

	INLINE void setupAboutText() {
		about = Gtk::TextBuffer::create();
		auto centre = about->create_tag();
		centre->property_justification_set() = true;
		centre->property_justification() = Gtk::JUSTIFY_CENTER;

		about->add_mark(mark_about, about->begin());
		about->insert(about->end(), "\n\n");
		about->insert_pixbuf(about->end(), Gdk::Pixbuf::create_from_file(INSTALL_DIR "/OpenScribe/icons/OpenScribe.svg", 128, 128));
		about->insert(about->end(), "\nOpenScribe ");
		about->insert(about->end(), CURRENT_VERSION);
		about->insert(about->end(), "\n\nCopyright © 2012–2014 Matt Pharoah\n\n");
		about->apply_tag(centre, about->begin(), about->end());
		about->insert(about->end(), "\n\n"
				"Openscribe is free software: you can redistribute it and/or modify "
				"it under the terms of the GNU General Public License as published by "
				"the Free Software Foundation, either version 3 of the License, or "
				"(at your option) any later version."
				"\n\n"
				"This program is distributed in the hope that it will be useful, "
				"but WITHOUT ANY WARRANTY; without even the implied warranty of "
				"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
				"GNU General Public License for more details."
				"\n\n"
				"You should have received a copy of the GNU General Public License "
				"along with this program.  If not, see <http://www.gnu.org/licenses/>.");
	}

	INLINE void setupUsageText() {
		usage = Gtk::TextBuffer::create();

		auto sectionHeader = usage->create_tag();
		sectionHeader->property_weight_set() = true;
		sectionHeader->property_scale_set() = true;
		sectionHeader->property_weight() = Pango::WEIGHT_BOLD;
		sectionHeader->property_scale() = Pango::SCALE_LARGE;

		usage->add_mark(mark_basicUsage, usage->begin());
		usage->insert_with_tag(usage->end(), "Using OpenScribe\n", sectionHeader);
		usage->insert(usage->end(), "OpenScribe is a program designed for transcribing audio files using a USB footpedal or hand control. For information on configuring your footpedal (or other input device), click on the Footpedal Configuration section in the Help explorer on the left.\n"
		"\nWhile OpenScribe is intended to be used with a footpedal, you may also control audio playback with the mouse by clicking on the larger buttons on the main window. One of the key features of OpenScribe is the ability to slow down audio without lowering its pitch- a feature designed to help you decipher unclear words in the dictation. You can adjust the speed at which audio is played by clicking on the ");
		usage->insert_pixbuf(usage->end(), Gdk::Pixbuf::create_from_file(INSTALL_DIR "/OpenScribe/icons/tortoise.svg"));
		usage->insert(usage->end(), " button to reveal the slow speed slider, then adjusting the speed. Note that this slider adjusts the speed at which audio is played when it is slowed, but to actually play the audio at this slowed speed, you must either click the SLOW button, or slow the dictation using your footpedal. OpenScribe uses the open-source Sonic library, developed by Bill Cox, for slowing audio.\n"
		"\nWhen OpenScribe starts, it requests to be kept in front of other windows so that it remains visible while you type in your word processor. If you do not want this behaviour, your window manager should allow you to turn this off. In Compiz, this is accomplished by right clicking on the window titlebar and unchecking 'Always On Top.'\n"
		"\nIf you encounter any bugs in this program, you may report them to the developer at < mtpharoah@gmail.com >.");

		usage->add_mark(mark_buttons, usage->end());
		usage->insert_with_tag(usage->end(), "\n\nButtons\n", sectionHeader);
		Gtk::IconInfo helpIconInfo = Gtk::IconTheme::get_default()->lookup_icon("help", 16, Gtk::ICON_LOOKUP_FORCE_SIZE);
		if (!helpIconInfo) helpIconInfo = Gtk::IconTheme::get_default()->lookup_icon("help-browser", 16, Gtk::ICON_LOOKUP_FORCE_SIZE);
		if (helpIconInfo) {
			usage->insert_pixbuf(usage->end(), helpIconInfo.load_icon());
		} else {
			usage->insert_pixbuf(usage->end(), Gdk::Pixbuf::create_from_file(INSTALL_DIR "/OpenScribe/icons/fallback/help.svg"));
		}
		usage->insert(usage->end(), "\tOpen help\n");
		Gtk::IconInfo openIconInfo = Gtk::IconTheme::get_default()->lookup_icon("document-open", 16, Gtk::ICON_LOOKUP_FORCE_SIZE);
		if (openIconInfo) {
			usage->insert_pixbuf(usage->end(), openIconInfo.load_icon());
		} else {
			usage->insert_pixbuf(usage->end(), Gdk::Pixbuf::create_from_file(INSTALL_DIR "/OpenScribe/icons/fallback/open.svg"));
		}
		usage->insert(usage->end(), "\tOpen audio file\n");
		usage->insert_pixbuf(usage->end(), Gdk::Pixbuf::create_from_file(INSTALL_DIR "/OpenScribe/icons/options.svg"));
		usage->insert(usage->end(), "\tOptions/Preferences\n");
		usage->insert_pixbuf(usage->end(), Gdk::Pixbuf::create_from_file(INSTALL_DIR "/OpenScribe/icons/pedal.svg"));
		usage->insert(usage->end(), "\tConfigure footpedals\n");
		usage->insert_pixbuf(usage->end(), Gdk::Pixbuf::create_from_file(INSTALL_DIR "/OpenScribe/icons/tortoise.svg"));
		usage->insert(usage->end(), "\tShow/Hide slow speed slider");
	}

	INLINE void setupConfigurationText() {
		configuration = Gtk::TextBuffer::create();

		auto sectionHeader = configuration->create_tag();
		sectionHeader->property_weight_set() = true;
		sectionHeader->property_scale_set() = true;
		sectionHeader->property_weight() = Pango::WEIGHT_BOLD;
		sectionHeader->property_scale() = Pango::SCALE_LARGE;

		auto bold = configuration->create_tag();
		bold->property_weight_set() = true;
		bold->property_weight() = Pango::WEIGHT_BOLD;

		auto italics = configuration->create_tag();
		italics->property_style() = Pango::STYLE_ITALIC;
		italics->property_style_set() = true;

		configuration->add_mark(mark_supportedDevices, configuration->begin());
		configuration->insert_with_tag(configuration->end(), "Supported Devices\n", sectionHeader);
		configuration->insert(configuration->end(), "OpenScribe supports all USB input devices with at least one button or axis.\n\n"
		"Most footpedal devices should work without any type of configuration required aside from assigning actions to each pedal; however, some footpedal manufacturers incorrectly "
		"configure their devices, causing users to not have permission to read from them. In this case, you must be an administrator (if you are the only user account on your "
		"computer, you will be an administrator) to use the device, and you will be asked to enter your password in order to temporarily change the permissions of the device so that "
		"you can use it. You should only do this for footpedal or hand control devices- USB keyboards and mice should not have their permissions changed, as this is a security issue.\n\n"
		"If your input device uses an adaptor, OpenScribe may think that the device has more buttons or axes than it actually does. This is not a software bug, but rather a side effect of "
		"the adaptor's implementation.");

		configuration->add_mark(mark_modifierPedal, configuration->end());
		configuration->insert_with_tag(configuration->end(), "\n\nPrimary and Secondary Actions\n", sectionHeader);
		configuration->insert(configuration->end(), "For a basic footpedal configuration where each pedal always performs the same action, you do not need to worry about secondary actions, and instead, simply set the primary actions of each pedal.\n"
		"\nHowever, OpenScribe allows you to assign two actions to all but one pedal, increasing the number of actions you can perform using your USB input device. To make use of this feature, you must assign one pedal to be a ");
		configuration->insert_with_tag(configuration->end(), "modifier pedal", italics);
		configuration->insert(configuration->end(), " by setting its primary action to 'Modifier Pedal.' Now, when you hold down this pedal (assuming you set its mode to HOLD), pressing or releasing other pedals will perform their secondary action. When the modifier pedal is ");
		configuration->insert_with_tag(configuration->end(), "not", italics);
		configuration->insert(configuration->end(), " held down, pedals perform their primary action. This effectively allows you to turn a 3-pedal device into a 4-pedal device, a 4-pedal device into a 6-pedal device, and so on. You may assign more than one pedal to be a modifier pedal, but there is little reason to do so.");

		configuration->add_mark(mark_actions, configuration->end());
		configuration->insert_with_tag(configuration->end(), "\n\nList of Actions\n", sectionHeader);
		configuration->insert(configuration->end(), "NOTE: Some actions (Play, Slow, Fast Forward, Rewind, and Modifier) can be set to either HOLD mode or TOGGLE mode. HOLD mode is selected by default. When a pedal is set to HOLD, pressing the pedal will perform the action, and releasing the pedal will undo the action."
		"(For example, in the case of the Play action, pressing the pedal down plays the audio, and releasing the pedal pauses it again.) If you instead choose TOGGLE mode, the action will be performed when you press the pedal, and only be undone when you press it a second time. The effect of each action is listed below:\n\n");
		configuration->insert_with_tag(configuration->end(), "\nDo Nothing\n", bold);
			configuration->insert(configuration->end(), "Pressing or releasing this pedal has no effect.\n");
		configuration->insert_with_tag(configuration->end(), "\nPlay\n", bold);
			configuration->insert(configuration->end(), "Play or pause the audio.\n"
			"If you have enabled the 'Skip back when resuming playback' feature in the options window, playback will skip backwards when audio starts playing. This affects both HOLD and TOGGLE modes.\n");
		configuration->insert_with_tag(configuration->end(), "\nSlow\n", bold);
			configuration->insert(configuration->end(), "Slows down audio playback.\n"
			"If the mode is set to HOLD, pressing and releasing this button will also play and pause the audio, skipping back when resuming playback if that feature is enabled.\n"
			"The amount that audio is slowed can be changed using the slow speed slider. (Click the ");
			configuration->insert_pixbuf(configuration->end(), Gdk::Pixbuf::create_from_file(INSTALL_DIR "/OpenScribe/icons/tortoise.svg"));
			configuration->insert(configuration->end(), " button on the main OpenScribe window to show/hide this slider.)\n");
		configuration->insert_with_tag(configuration->end(), "\nFast Forward / Rewind\n", bold);
			configuration->insert(configuration->end(), "Fast forward or rewind the audio.\n"
			"The speed at which the audio is fast forwarded or rewinded can be changed in the options window.\n");
		configuration->insert_with_tag(configuration->end(), "\nSkip Forward / Skip Backwards\n", bold);
			configuration->insert(configuration->end(), "Skip forward/backward the specified number of seconds.\n");
		configuration->insert_with_tag(configuration->end(), "\nSkip to Beginning\n", bold);
			configuration->insert(configuration->end(), "Seek back to the beginning of the audio file.\n");
		configuration->insert_with_tag(configuration->end(), "\nIncrease / Decrease Slow Speed\n", bold);
			configuration->insert(configuration->end(), "Increase or decrease the speed at which audio is played when it is slowed.\n"
			"If audio is currently slowed, the playback speed is changed immediately.");

		configuration->add_mark(mark_axes, configuration->end());
		configuration->insert_with_tag(configuration->end(), "\n\nConfiguring Axes\n", sectionHeader);
		configuration->insert(configuration->end(), "Most footpedals implement their pedals as buttons, meaning the pedal is either pressed or it is not. Some pedals, however, may implement each pedal as an axis, meaning it tells the computer how far down the pedal is being pressed. For devices like this, you must configure each pedal so that OpenScribe knows when a footpedal should be considered to be pressed. To configure a pedal, find the pedal you wish to configure by pressing that pedal up and down. The bar beside the axis corresponding to the pedal you want to configure should change as you press and release the pedal."
		"Once you have determined which pedal you wish to configure, click the axis name (eg. 'Axis 5'), and follow the instructions in the window that pops up. If you configured a pedal incorrectly and need to reconfigure it, you can click on its name to try again.");
	}
};


#endif /* HELPWINDOW_HPP_ */
