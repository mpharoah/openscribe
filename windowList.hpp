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

//Global variables for most windows used by OpenScribe

#ifndef WINDOWLIST_HPP_
#define WINDOWLIST_HPP_

#include <gtkmm/window.h>

namespace WindowList {
	extern Gtk::Window *main;
	extern Gtk::Window *options;
	extern Gtk::Window *config;
	extern Gtk::Window *help;
}


#endif /* WINDOWLIST_HPP_ */
