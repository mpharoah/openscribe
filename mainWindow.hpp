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

#ifndef MAINWINDOW_HPP_
#define MAINWINDOW_HPP_

#include <gtkmm/window.h>
#include <gtkmm/button.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/scale.h>
#include <gtkmm/label.h>
#include <gtkmm/hvbox.h>
#include <gtkmm/separatortoolitem.h>
#include <gtkmm/image.h>
#include <gtkmm/expander.h>
#include <gtkmm/hvseparator.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/iconinfo.h>
#include <glibmm/main.h>
#include <glibmm/dispatcher.h>
#include <cstdlib>
#include <mutex>
#include <atomic>
#include <queue>

#include "windowList.hpp"
#include "attributes.hpp"
#include "dictation.hpp"
#include "footPedal.hpp"
#include "config.hpp"
#include "actions.hpp"

class MainWindow : public Gtk::Window {
  private:
	Dictation *player;
	FootPedalCoordinator *pedals;

	Gtk::Button playButton, restartButton, skipBack10Button, skipBack5Button, rewindButton, fastForwardButton, helpButton, openButton, optionsButton, pedalConfigButton;
	Gtk::ToggleButton slowButton, showSlowSpeedButton;
	Gtk::Label audioFileLabel, positionLabel, lengthLabel;
	Gtk::HScale slider;

	Gtk::HBox rootLayout;
	Gtk::VBox mainLayout;
	Gtk::HBox topLayout, middleLayout, bottomLayout;

	Gtk::HBox slowSpeedPopout;
	Gtk::Label slowSpeedLabel;
	Gtk::VScale slowSpeedSlider;
	Gtk::VSeparator popoutSeparator;

	Gtk::SeparatorToolItem sepA, sepB, sepC;

	void playButtonPressed();
	void restartButtonPressed();
	void skipBack10ButtonPressed();
	void skipBack5ButtonPressed();
	void slowButtonPressed();
	void rewindButtonPressed();
	void rewindButtonReleased();
	void fastForwardButtonPressed();
	void fastForwardButtonReleased();

	void helpButtonPressed();
	void openButtonPressed();
	void optionsButtonPressed();
	void pedalConfigButtonPressed();
	void showSlowSpeedButtonPressed();

	void onPedalEvent(Action cmd);

	bool sliderPressed(GdkEventButton *ev);
	bool sliderMoved(Gtk::ScrollType type, double value);
	bool sliderReleased(GdkEventButton *ev);

	bool onSlowSpeedSliderMoved(Gtk::ScrollType type, double value);

	bool updatePosition();
	void updateNameAndDurationLabels();

	bool adjustingSlider;
	bool wasPlaying;

	Gtk::Image playIcon, pauseIcon;
	Gtk::Image restartIcon, skipBack5Icon, skipBack10Icon, slowIcon, rwdIcon, ffwdIcon;
	Gtk::Image helpIcon, openIcon, optionsIcon, pedalIcon, slowSpeedIcon;

	Options options;

	sigc::connection timer;

	/* Things used for updating the UI after receiving footpedal events */
	std::mutex actionLock;
	std::queue<decltype(Action::type)> actionQueue;
	Glib::Dispatcher updateUI;
	void onUpdateRequest();

	/* As above, but for slowSpeed specifically */
	std::atomic<unsigned char> newSlowSpeed;
	Glib::Dispatcher refreshSlowSpeed;
	void onSlowSpeedChanged();

	Glib::ustring slowSliderFormatLabel(double value) const { return Glib::ustring::compose("%1%%", (int)(100.0*value)); }

	Glib::Dispatcher errorDispatcher;
	static void onStreamErrorAdaptor(__attribute__((unused)) int err) { ((MainWindow*)WindowList::main)->errorDispatcher.emit(); }
	void onStreamError() const;

  public:
	MainWindow(Dictation *dict, const Options &opt, FootPedalCoordinator *fpc) : Gtk::Window(), player(dict), pedals(fpc), adjustingSlider(false), options(opt) {
		set_title("OpenScribe");
		set_border_width(3);
		set_resizable(false);
		set_keep_above(true);

		errorDispatcher.connect(sigc::mem_fun(*this, &MainWindow::onStreamError));
		dict->connectErrorHandler(onStreamErrorAdaptor);

		playIcon.set(INSTALL_DIR "/OpenScribe/icons/play.svg");
		pauseIcon.set(INSTALL_DIR "/OpenScribe/icons/pause.svg");
		restartIcon.set(INSTALL_DIR "/OpenScribe/icons/toStart.svg");
		skipBack10Icon.set(INSTALL_DIR "/OpenScribe/icons/back10.svg");
		skipBack5Icon.set(INSTALL_DIR "/OpenScribe/icons/back5.svg");
		slowIcon.set(INSTALL_DIR "/OpenScribe/icons/slow.svg");
		rwdIcon.set(INSTALL_DIR "/OpenScribe/icons/rewind.svg");
		ffwdIcon.set(INSTALL_DIR "/OpenScribe/icons/fastForward.svg");
		optionsIcon.set(INSTALL_DIR "/OpenScribe/icons/options.svg");
		pedalIcon.set(INSTALL_DIR "/OpenScribe/icons/pedal.svg");
		slowSpeedIcon.set(INSTALL_DIR "/OpenScribe/icons/tortoise.svg");

		Gtk::IconInfo openIconInfo = Gtk::IconTheme::get_default()->lookup_icon("document-open", 16, Gtk::ICON_LOOKUP_FORCE_SIZE);
		if (openIconInfo) {
			openIcon.set(openIconInfo.load_icon());
		} else {
			openIcon.set(INSTALL_DIR "/OpenScribe/icons/fallback/open.svg");
		}

		Gtk::IconInfo helpIconInfo = Gtk::IconTheme::get_default()->lookup_icon("help", 16, Gtk::ICON_LOOKUP_FORCE_SIZE);
		if (!helpIconInfo) helpIconInfo = Gtk::IconTheme::get_default()->lookup_icon("help-browser", 16, Gtk::ICON_LOOKUP_FORCE_SIZE);
		if (helpIconInfo) {
			helpIcon.set(helpIconInfo.load_icon());
		} else {
			helpIcon.set(INSTALL_DIR "/OpenScribe/icons/fallback/help.svg");
		}

		playButton.set_image(playIcon);
		restartButton.set_image(restartIcon);
		skipBack10Button.set_image(skipBack10Icon);
		skipBack5Button.set_image(skipBack5Icon);
		slowButton.set_image(slowIcon);
		rewindButton.set_image(rwdIcon);
		fastForwardButton.set_image(ffwdIcon);
		helpButton.set_image(helpIcon);
		openButton.set_image(openIcon);
		optionsButton.set_image(optionsIcon);
		pedalConfigButton.set_image(pedalIcon);
		showSlowSpeedButton.set_image(slowSpeedIcon);

		playButton.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::playButtonPressed));
		restartButton.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::restartButtonPressed));
		skipBack10Button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::skipBack10ButtonPressed));
		skipBack5Button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::skipBack5ButtonPressed));
		slowButton.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::slowButtonPressed));
		rewindButton.signal_pressed().connect(sigc::mem_fun(*this, &MainWindow::rewindButtonPressed));
		rewindButton.signal_released().connect(sigc::mem_fun(*this, &MainWindow::rewindButtonReleased));
		fastForwardButton.signal_pressed().connect(sigc::mem_fun(*this, &MainWindow::fastForwardButtonPressed));
		fastForwardButton.signal_released().connect(sigc::mem_fun(*this, &MainWindow::fastForwardButtonReleased));
		helpButton.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::helpButtonPressed));
		openButton.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::openButtonPressed));
		optionsButton.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::optionsButtonPressed));
		pedalConfigButton.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::pedalConfigButtonPressed));
		showSlowSpeedButton.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::showSlowSpeedButtonPressed));
		slider.signal_change_value().connect(sigc::mem_fun(*this, &MainWindow::sliderMoved));
		slider.signal_button_press_event().connect(sigc::mem_fun(*this, &MainWindow::sliderPressed), false);
		slider.signal_button_release_event().connect(sigc::mem_fun(*this, &MainWindow::sliderReleased), false);

		audioFileLabel.override_font(Pango::FontDescription("Ubuntu 11"));
		positionLabel.override_font(Pango::FontDescription("Ubuntu Mono 18"));
		lengthLabel.override_font(Pango::FontDescription("Ubuntu Mono 18"));

		positionLabel.set_text(" 0:00");
		updateNameAndDurationLabels();

		slider.set_range(0.0,1.0);
		slider.set_increments(0.01, 0.1);
		slider.set_value(0.0);
		slider.set_draw_value(false);

		popoutSeparator.set_margin_left(5);
		popoutSeparator.set_margin_right(5);

		slowSpeedLabel.set_angle(90);
		slowSpeedLabel.set_label("Slow Speed");

		slowSpeedSlider.set_range(0.2, 1.0);
		slowSpeedSlider.set_draw_value(true);
		slowSpeedSlider.set_value_pos(Gtk::POS_RIGHT);
		slowSpeedSlider.set_digits(2);
		slowSpeedSlider.set_round_digits(2);
		slowSpeedSlider.set_increments(0.1, 0.1);
		slowSpeedSlider.set_inverted(true);
		slowSpeedSlider.signal_format_value().connect(sigc::mem_fun(*this, &MainWindow::slowSliderFormatLabel));
		slowSpeedSlider.set_value(opt.slowSpeed);
		slowSpeedSlider.signal_change_value().connect(sigc::mem_fun(*this, &MainWindow::onSlowSpeedSliderMoved));

		add(rootLayout);
			rootLayout.pack_start(mainLayout, false, false);
				mainLayout.pack_start(topLayout, false, false);
					topLayout.pack_start(audioFileLabel, true, true);
					topLayout.pack_start(helpButton, false, false);
					topLayout.pack_start(openButton, false, false);
					topLayout.pack_start(optionsButton, false, false);
					topLayout.pack_start(pedalConfigButton, false, false);
					topLayout.pack_start(showSlowSpeedButton, false, false);
				mainLayout.pack_start(middleLayout, false, false);
					middleLayout.pack_start(playButton, false, false);
					middleLayout.pack_start(sepA, false, false);
					middleLayout.pack_start(restartButton, false, false);
					middleLayout.pack_start(skipBack10Button, false, false);
					middleLayout.pack_start(skipBack5Button, false, false);
					middleLayout.pack_start(sepB, false, false);
					middleLayout.pack_start(slowButton, false, false);
					middleLayout.pack_start(sepC, false, false);
					middleLayout.pack_start(rewindButton, false, false);
					middleLayout.pack_start(fastForwardButton, false, false);
				mainLayout.pack_start(bottomLayout, false, false);
					bottomLayout.pack_start(positionLabel, false, false);
					bottomLayout.pack_start(slider, true, true);
					bottomLayout.pack_start(lengthLabel, false, false);
			rootLayout.pack_start(slowSpeedPopout, false, false);
				slowSpeedPopout.pack_start(popoutSeparator, false, false);
				slowSpeedPopout.pack_start(slowSpeedLabel, false, false);
				slowSpeedPopout.pack_start(slowSpeedSlider, false, false);

		audioFileLabel.set_alignment(Gtk::ALIGN_START);
		audioFileLabel.set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
		audioFileLabel.set_max_width_chars(13); //the actual value given to set_max_width_chars doesn't matter if the label is ellipsized (it should ellipsize itself iff it would cause the parent widget to grow otherwise)
		timer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &MainWindow::updatePosition), 50);
		updateUI.connect(sigc::mem_fun(*this, &MainWindow::onUpdateRequest));
		refreshSlowSpeed.connect(sigc::mem_fun(*this, &MainWindow::onSlowSpeedChanged));
		show_all_children();
		slowSpeedPopout.hide();
	}

	virtual ~MainWindow() { timer.disconnect();	}

	void updateOptions(const Options &opt);
	INLINE const Options &getOptions() const { return options; }

	static void onPedalEventAdaptor(Action cmd) {
		((MainWindow*)WindowList::main)->onPedalEvent(cmd);
	}
};


#endif /* MAINWINDOW_HPP_ */
