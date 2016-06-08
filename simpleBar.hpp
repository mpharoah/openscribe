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


/*
 * A widget similar to a Gtk::ProgressBar, but that uses a very simple
 * theme-independent drawing function and that can change colour when
 * its value is set above a given threshold
 */

#ifndef SIMPLEBAR_HPP_
#define SIMPLEBAR_HPP_

#include <gtkmm/widget.h>
#include <glibmm/dispatcher.h>
#include <gdkmm/general.h>
#include <gdkmm/color.h>

#include <atomic>
#include <limits>
#include <cstring>

#include "attributes.hpp"

class SimpleBar : public Gtk::Widget {
  private:
	Glib::RefPtr<Gdk::Window> window;

	double value;
	double threshold;

	Gdk::Color belowColour;
	Gdk::Color aboveColour;

	int width;
	int height;

	Glib::Dispatcher dispatch;
	std::atomic<unsigned> queued_value;

	FLATTEN void set_queued_value() { value = (double)queued_value / (double)std::numeric_limits<unsigned>::max(); queue_draw(); }

  protected:
	virtual void get_preferred_width_vfunc(int& minimum_width, int& natural_width) const {
		minimum_width = width;
		natural_width = (width > 150) ? width : 150;
	}
	virtual void get_preferred_height_vfunc(int& minimum_height, int& natural_height) const {
		minimum_height = height;
		natural_height = (height > 25) ? height : 25;
	}
	virtual void get_preferred_width_for_height_vfunc(int, int& minimum_width, int& natural_width) const {
		// no preferred aspect ratio
		minimum_width = width;
		natural_width = (width > 150) ? width : 150;
	}
	virtual void get_preferred_height_for_width_vfunc(int, int& minimum_height, int& natural_height) const {
		// no preferred aspect ratio
		minimum_height = height;
		natural_height = (height > 25) ? height : 25;
	}
	virtual void on_size_allocate(Gtk::Allocation& allocation) {
		set_allocation(allocation);
		if (window) {
			window->move_resize(allocation.get_x(), allocation.get_y(), allocation.get_width(), allocation.get_height());
		}
	}
	virtual void on_realize() {
		set_realized();

		if (!window) {
			GdkWindowAttr attr;
			std::memset(&attr, 0, sizeof(GdkWindowAttr));

			Gtk::Allocation alloc = get_allocation();

			attr.x = alloc.get_x();
			attr.y = alloc.get_y();
			attr.width = alloc.get_width();
			attr.height = alloc.get_height();

			attr.event_mask = get_events() | Gdk::EXPOSURE_MASK;
			attr.window_type = GDK_WINDOW_CHILD;
			attr.wclass = GDK_INPUT_OUTPUT;

			window = Gdk::Window::create(get_parent_window(), &attr, GDK_WA_X | GDK_WA_Y);
			set_window(window);
			window->set_user_data(gobj());
		}
	}
	virtual void on_unrealize() {
		window.reset();
		Gtk::Widget::on_unrealize();
	}

	virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& pen) {
		const double w = (double)get_allocation().get_width();
		const double h = (double)get_allocation().get_height();

		const double x = 0.5 + (w - 1.0) * value;

		const Gdk::Color &colour = (value > threshold) ? aboveColour : belowColour;
		pen->set_source_rgb(colour.get_red_p(), colour.get_green_p(), colour.get_blue_p());

		pen->move_to(0.5, 0.5);
		pen->line_to(x, 0.5);
		pen->line_to(x, h - 0.5);
		pen->line_to(0.5, h - 0.5);
		pen->fill();

		pen->set_source_rgb(0.0, 0.0, 0.0);
		pen->set_line_width(1.0);

		pen->move_to(0.5, 0.5);
		pen->line_to(w - 0.5, 0.5);
		pen->line_to(w - 0.5, h - 0.5);
		pen->line_to(0.5, h - 0.5);
		pen->close_path();
		pen->stroke();

		return true;
	}



  public:
	SimpleBar() : Glib::ObjectBase("SimpleBar"), Gtk::Widget(), value(0), threshold(0), width(3), height(3) {
		set_has_window(true);

		aboveColour.set_rgb_p(0.0, 1.0, 0.0);
		belowColour.set_rgb_p(1.0, 0.0, 0.0);

		dispatch.connect(sigc::mem_fun(*this, &SimpleBar::set_queued_value));
	}
	INLINE ~SimpleBar() {}

	INLINE void set_minimum_width(int w) { width = (w > 3) ? w : 3; }
	INLINE void set_minimum_height(int h) { height = (h > 3) ? h : 3; }
	INLINE void set_minimum_size(int w, int h) { width = (w > 3) ? w : 3;  height = (h > 3) ? h : 3; }

	INLINE void set_colour_below_threshold(const Gdk::Color &c) { belowColour = c; }
	INLINE void set_colour_above_threshold(const Gdk::Color &c) { aboveColour = c; }
	INLINE void set_colours(const Gdk::Color &below, const Gdk::Color &above) { belowColour = below; aboveColour = above; }

	INLINE void set_value(double v) { value = (v > 0.0) ? ((v < 1.0) ? v : 1.0) : 0.0; queue_draw(); }
	INLINE void set_value_async(double v) {
		if (v <= 0.0) {
			queued_value = 0;
		} else if (v >= 1.0) {
			queued_value = std::numeric_limits<unsigned>::max();
		} else {
			queued_value = (unsigned)(v * (double)std::numeric_limits<unsigned>::max());
		}
		dispatch.emit();
	}
	USERET INLINE double get_value() const { return value; }

	INLINE void set_threshold(double t) { threshold = (t > 0.0) ? ((t < 1.0) ? t : 1.0) : 0.0; queue_draw(); }
	USERET INLINE double get_threshold() const { return threshold; }

	USERET INLINE bool get_above_threshold() const { return(value > threshold); }

};


#endif /* SIMPLEBAR_HPP_ */
