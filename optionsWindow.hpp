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

#ifndef OPTIONSWINDOW_HPP_
#define OPTIONSWINDOW_HPP_

#include <gtkmm/window.h>
#include <gtkmm/label.h>
#include <gtkmm/hvscale.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/hvseparator.h>
#include <gtkmm/frame.h>
#include <gtkmm/hvbox.h>
#include <gtkmm/grid.h>

#include "attributes.hpp"
#include "config.hpp"

class OptionsWindow : public Gtk::Window {
  private:
	Gtk::VBox rootLayout;
	Gtk::Grid SBOPLayout;
	Gtk::VBox FFARLayout, SSLayout, AOLayout;
	Gtk::HBox buttonLayout;

	Gtk::HBox spinnerLayout, soundEffectsLayout;
	Gtk::Grid buttonSublayout;

	Gtk::HBox spacer1, spacer2, spacer3, spacer4, spacer5;
	Gtk::HSeparator sep1, sep2, sep3, sep4;
	Gtk::HBox indent;

	Gtk::Frame SBOPFrame, FFARFrame, SSFrame, AOFrame;

	Gtk::SpinButton skipBackSpinner;
	Gtk::CheckButton soundEffectsCheckbox, skipBackCheckbox;
	Gtk::HScale rwdSlider, ffwdSlider, slowSlider;
	Gtk::HScale chunkSizeSlider, preloadSlider, historySlider;
	Gtk::Button cancel, apply, okay;

	Gtk::Label seconds;
	Gtk::Label rwdLabel, ffwdLabel, slowLabel;
	Gtk::Label advOptLabel, advOptInfoLabel;
	Gtk::Label chunkSizeLabel, preloadLabel, historyLabel;

	void applyOptions() const;
	void applyAndClose() { applyOptions(); hide(); }

	Glib::ustring formatTimes(double value) const {			return Glib::ustring::compose("x%1", 1 << (int)value); }
	Glib::ustring formatMilliseconds(double value) const {	return Glib::ustring::compose("%1 milliseconds", (int)value); }
	Glib::ustring formatPercent(double value) const {		return Glib::ustring::compose("%1%%", (int)(100.0*value)); }
	Glib::ustring formatSeconds(double value) const {
		if (value == 1.0) return Glib::ustring("1 second");
		return Glib::ustring::compose("%1 seconds", (int)value);
	}

	void onSkipCheckClicked() {
		skipBackSpinner.set_sensitive(skipBackCheckbox.get_active());
	}

  protected:
	virtual void on_show();

  public:
	UNROLL OptionsWindow() : Gtk::Window() {
		set_title("OpenScribe Options");
		set_border_width(5);
		resize(435, 1);

		skipBackCheckbox.set_label("Skip back when resuming playback");
		seconds.set_label("seconds");
		soundEffectsCheckbox.set_label("Rewind and fast forward sound effects");
		rwdLabel.set_markup("<b>Rewind Speed</b>");
		ffwdLabel.set_markup("<b>Fast Forward Speed</b>");
		slowLabel.set_markup("<b>Default Slow Speed</b>");
		advOptLabel.set_markup("<b><big>Advanced Options</big></b>");
		advOptInfoLabel.set_markup("<i>You do not need to adjust these settings unless you experience audio stuttering, audio latency, or delayed foot pedal response. Mouse over each slider for information.</i>");
		chunkSizeLabel.set_markup("<b>Audio Chunk Size</b>");
		historyLabel.set_markup("<b>Audio History Size</b>");
		preloadLabel.set_markup("<b>Audio Preload Size</b>");
		cancel.set_label("Cancel");
		apply.set_label("Apply");
		okay.set_label("Okay");
		SBOPFrame.set_label("Skip on Playback");
		FFARFrame.set_label("Fast Forward and Rewind");
		SSFrame.set_label("Slow Speed");

		advOptInfoLabel.set_line_wrap(true);
		advOptInfoLabel.set_single_line_mode(false);
		buttonSublayout.set_column_homogeneous(true);
		rwdSlider.set_range(0.0, 6.0);
		rwdSlider.set_draw_value(true);
		rwdSlider.set_value_pos(Gtk::POS_TOP);
		rwdSlider.set_round_digits(0);
		for (int i = 0; i <= 6; i++) rwdSlider.add_mark((double)i, Gtk::POS_TOP, Glib::ustring());
		ffwdSlider.set_range(1.0, 6.0);
		ffwdSlider.set_draw_value(true);
		ffwdSlider.set_value_pos(Gtk::POS_TOP);
		ffwdSlider.set_round_digits(0);
		for (int i = 0; i <= 6; i++) ffwdSlider.add_mark((double)i, Gtk::POS_TOP, Glib::ustring());
		slowSlider.set_range(0.2, 1.0);
		slowSlider.set_draw_value(true);
		slowSlider.set_value_pos(Gtk::POS_TOP);
		slowSlider.set_round_digits(2);
		chunkSizeSlider.set_range(10.0, 60.0);
		chunkSizeSlider.set_draw_value(true);
		chunkSizeSlider.set_value_pos(Gtk::POS_TOP);
		chunkSizeSlider.set_round_digits(0);
		historySlider.set_range(1.0, 13.0);
		historySlider.set_draw_value(true);
		historySlider.set_value_pos(Gtk::POS_TOP);
		historySlider.set_round_digits(0);
		for (int i = 1; i <= 13; i++) historySlider.add_mark((double)i, Gtk::POS_TOP, Glib::ustring());
		preloadSlider.set_range(1.0, 13.0);
		preloadSlider.set_draw_value(true);
		preloadSlider.set_value_pos(Gtk::POS_TOP);
		preloadSlider.set_round_digits(0);
		for (int i = 1; i <= 13; i++) preloadSlider.add_mark((double)i, Gtk::POS_TOP, Glib::ustring());
		skipBackSpinner.set_range(0.0, 10.0);
		skipBackSpinner.set_digits(2);
		skipBackSpinner.set_numeric(true);
		skipBackSpinner.set_increments(0.1, 1.0);

		rootLayout.set_spacing(5);
		soundEffectsLayout.set_margin_bottom(5);
		advOptLabel.set_margin_bottom(4);
		sep1.set_margin_top(8);
		sep1.set_margin_bottom(8);
		sep2.set_margin_top(8);
		sep2.set_margin_bottom(8);
		sep3.set_margin_top(8);
		sep3.set_margin_bottom(8);
		sep4.set_margin_top(8);
		sep4.set_margin_bottom(8);
		indent.set_size_request(32, 1);

		chunkSizeSlider.set_tooltip_text("Set how much audio is sent to the PulseAudio sound server at once. Also sets the PulseAudio buffer size to double this amount. A lower value means lower latency and better responsiveness, but setting it too low may cause stuttering on slow computers. Default value is 15ms.");
		historySlider.set_tooltip_text("Sets the maximum length of audio that is kept in memory after it has played. Skipping back further than this means that the audio will need to be decoded from the file again, which causes a slight pause. It is highly recommended to set this to at least 3 to 5 seconds. For a typical audio file, each second of history saved increases memory usage by about 1/3rd of a megabyte (exact value depends on sample rate and number of channels). Default value is 6 seconds.");
		preloadSlider.set_tooltip_text("Since decoding audio from a file takes time, OpenScribe decodes audio from the file ahead of the current position so that the data will be decoded and ready to play by the time the audio is needed. This slider sets how far ahead of the current position OpenScribe should go when preparing audio for playback. The actual amount of audio in memory that is ahead of the current position can be larger than this value if the user skips back (since the audio history we skipped past is now in the future). There is little benefit to making this a large value unless you are running another process in the background with irregular CPU usage. Default value is 2 seconds.");

		add(rootLayout);
			rootLayout.pack_start(SBOPFrame, false, false);
				SBOPFrame.add(SBOPLayout);
					SBOPLayout.attach(skipBackCheckbox, 0, 0, 1, 1);
					SBOPLayout.attach(spacer1, 1, 0, 1, 1);
					SBOPLayout.attach(spinnerLayout, 0, 1, 2, 1);
						spinnerLayout.pack_start(indent, false, false);
						spinnerLayout.pack_start(skipBackSpinner, false, false);
						spinnerLayout.pack_start(seconds, false, false);
						spinnerLayout.pack_start(spacer2, true, true);
			rootLayout.pack_start(FFARFrame, false, false);
				FFARFrame.add(FFARLayout);
					FFARLayout.pack_start(soundEffectsLayout);
						soundEffectsLayout.pack_start(soundEffectsCheckbox, false, false);
						soundEffectsLayout.pack_start(spacer3, true, true);
					FFARLayout.pack_start(rwdLabel);
					FFARLayout.pack_start(rwdSlider);
					FFARLayout.pack_start(sep1);
					FFARLayout.pack_start(ffwdLabel);
					FFARLayout.pack_start(ffwdSlider);
			rootLayout.pack_start(SSFrame, false, false);
				SSFrame.add(SSLayout);
					SSLayout.pack_start(slowLabel);
					SSLayout.pack_start(slowSlider);
			rootLayout.pack_start(AOFrame, false, false);
				AOFrame.add(AOLayout);
					AOLayout.pack_start(advOptLabel);
					AOLayout.pack_start(advOptInfoLabel);
					AOLayout.pack_start(sep2);
					AOLayout.pack_start(chunkSizeLabel);
					AOLayout.pack_start(chunkSizeSlider);
					AOLayout.pack_start(sep3);
					AOLayout.pack_start(historyLabel);
					AOLayout.pack_start(historySlider);
					AOLayout.pack_start(sep4);
					AOLayout.pack_start(preloadLabel);
					AOLayout.pack_start(preloadSlider);
			rootLayout.pack_start(spacer4, true, true);
			rootLayout.pack_start(buttonLayout, false, false);
				buttonLayout.pack_start(spacer5, true, true);
				buttonLayout.pack_start(buttonSublayout, false, false);
					buttonSublayout.attach(cancel, 0, 0, 1, 1);
					buttonSublayout.attach(apply,  1, 0, 1, 1);
					buttonSublayout.attach(okay,   2, 0, 1, 1);

		spacer1.property_expand() = true;
		show_all_children();

		skipBackCheckbox.signal_clicked().connect(sigc::mem_fun(*this, &OptionsWindow::onSkipCheckClicked));
		cancel.signal_clicked().connect(sigc::mem_fun(*this, &OptionsWindow::hide));
		apply.signal_clicked().connect(sigc::mem_fun(*this, &OptionsWindow::applyOptions));
		okay.signal_clicked().connect(sigc::mem_fun(*this, &OptionsWindow::applyAndClose));

		rwdSlider.signal_format_value().connect(sigc::mem_fun(*this, &OptionsWindow::formatTimes));
		ffwdSlider.signal_format_value().connect(sigc::mem_fun(*this, &OptionsWindow::formatTimes));
		slowSlider.signal_format_value().connect(sigc::mem_fun(*this, &OptionsWindow::formatPercent));
		chunkSizeSlider.signal_format_value().connect(sigc::mem_fun(*this, &OptionsWindow::formatMilliseconds));
		preloadSlider.signal_format_value().connect(sigc::mem_fun(*this, &OptionsWindow::formatSeconds));
		historySlider.signal_format_value().connect(sigc::mem_fun(*this, &OptionsWindow::formatSeconds));
	}
	INLINE ~OptionsWindow() {}
};

#endif /* OPTIONSWINDOW_HPP_ */
