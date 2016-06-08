# Name of the .cpp file containing the main method
MAINFILE = main.cpp
# List of other .cpp files
CPPFILES = dictation.cpp audioFileReader.cpp mainWindow.cpp optionsWindow.cpp configWindow.cpp config.cpp version.cpp windowList.cpp footPedal.cpp
# List .hpp files with no associated .cpp file here
HFILES = attributes.hpp changelog.hpp actions.hpp simpleBar.hpp helpWindow.hpp

CC = g++
OFILES = $(MAINFILE:.cpp=.o) $(CPPFILES:.cpp=.o)
HFILES += $(CPPFILES:.cpp=.hpp)

# Include directories
INCLUDES = 
# Libraries to include
LIBS = `pkg-config --libs gtkmm-3.0` -lpulse -lsonic -lsox -ludev
# Miscellaneous flags
MISCFLAGS = `pkg-config --cflags gtkmm-3.0`
# Filename of compiled binary
TARGET = openscribe
# Normally, install to root. debuilder changes this when packaging
DESTDIR = /

# For manual builds from source, prefix should be /usr/local
# For building debian packages, prefix should be /usr
prefix=/usr

exec_prefix=$(prefix)
bindir=$(exec_prefix)/bin

datarootdir=$(prefix)/share
datadir=$(datarootdir)

# Installation Directory
INSTALL_DIR = $(datadir)/OpenScribe

# Standard used and warnings
CPPFLAGS = -std=c++0x -Wall -Wextra -DINSTALL_DIR='"$(datadir)"'

all: final

# Compiles the program with many optimizations, while still separating compilation units. Assertions are still performed (default)
optimized: CPPFLAGS += -O3 -fno-trapping-math
optimized: $(TARGET)

# Compiles the program with many optimizations, while still separating compilation units. Disables assertions
release: CPPFLAGS += -O3 -fno-signaling-nans -fno-trapping-math -DNDEBUG
release: $(TARGET)

# Compiles the program with debugging symbols and no optimization
debug: CPPFLAGS += -O0 -g3
debug: $(TARGET)

# Compiles the program with link time optimization. Takes much longer to build.
final: CPPFLAGS += -flto -fuse-linker-plugin -O3 -funroll-loops -fno-signaling-nans -fno-trapping-math -DIGNORE_UNROLL_ATTR -DNDEBUG
final: $(TARGET)

%.o: %.cpp $(HFILES) Makefile
	$(CC) -c $(CPPFLAGS) $(MISCFLAGS) $(INCLUDES) $< -o $@
	
$(TARGET): $(OFILES) $(HFILES)
	$(CC) -o $(TARGET) $(CPPFLAGS) $(MISCFLAGS) $(OFILES) $(HFILES) $(LIBS)
	
install: release
	install --mode=755 -d $(DESTDIR)$(INSTALL_DIR)
	install --mode=755 -d $(DESTDIR)$(INSTALL_DIR)/icons
	install --mode=644 -t $(DESTDIR)$(INSTALL_DIR)/icons icons/*.svg
	install --mode=755 -d $(DESTDIR)$(INSTALL_DIR)/icons/fallback
	install --mode=644 -t $(DESTDIR)$(INSTALL_DIR)/icons/fallback icons/fallback/*.svg
	install --mode=755 -d $(DESTDIR)$(bindir)
	install --mode=755 -t $(DESTDIR)$(bindir) $(TARGET)
	install --mode=755 -d $(DESTDIR)$(datadir)/pixmaps
	install --mode=644 -t $(DESTDIR)$(datadir)/pixmaps icons/menu/*.xpm
	
uninstall:
	rm -r $(DESTDIR)$(INSTALL_DIR)
	rm $(DESTDIR)$(bindir)/$(TARGET)
	
clean:
	rm -f $(OFILES)

distclean: clean
	rm -f $(TARGET)

