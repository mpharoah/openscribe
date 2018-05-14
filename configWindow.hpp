#ifndef CONFIGWINDOW_HPP_
#define CONFIGWINDOW_HPP_

#include <gtkmm/window.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treemodelfilter.h>
#include <gtkmm/hvbox.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/button.h>
#include <gtkmm/combobox.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/frame.h>
#include <gtkmm/hvscale.h>
#include <gtkmm/image.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/iconinfo.h>
#include <gtkmm/messagedialog.h>
#include <glibmm/dispatcher.h>

#include <queue>
#include <mutex>
#include <vector>
#include <map>
#include <atomic>

#include "attributes.hpp"
#include "windowList.hpp"
#include "footPedal.hpp"
#include "simpleBar.hpp"
#include "actions.hpp"

#include <cassert>


class ActionSelector : public Gtk::ComboBox {
  private:
	Glib::RefPtr<Gtk::ListStore> comboModel;
	Gtk::TreeModel::ColumnRecord entryType;

	Gtk::TreeModelColumn<int> entryData;
	Gtk::TreeModelColumn<Glib::ustring> entryText;

  public:
	ActionSelector();
	INLINE ~ActionSelector() {};

	INLINE int getActionType() { return(get_active()->get_value(entryData)); }
	void makePrimary();

	static const int NOOP;
	static const int PLAY;
	static const int SLOW;
	static const int FFWD;
	static const int RWD;
	static const int SKIP_AHEAD;
	static const int SKIP_BACK;
	static const int RESTART;
	static const int FASTER;
	static const int SLOWER;
	static const int MOD;
};

class ToggleSelector : public Gtk::ComboBox {
  private:
	Glib::RefPtr<Gtk::ListStore> comboModel;
	Gtk::TreeModel::ColumnRecord entryType;

	Gtk::TreeModelColumn<bool> entryData;
	Gtk::TreeModelColumn<Glib::ustring> entryText;

  public:
	ToggleSelector();
	INLINE ~ToggleSelector() {};

	INLINE bool isToggleSelected() { return(get_active()->get_value(entryData)); }
};

class ConfigRowBase {
  protected:
	Gtk::Grid *layout;
	int row;

	Gtk::Label secondsLabel1, secondsLabel2, percentLabel1, percentLabel2;
	ActionSelector primaryActionBox, secondaryActionBox;
	ToggleSelector primaryToggleBox, secondaryToggleBox;
	Gtk::SpinButton primarySeconds, secondarySeconds;
	Gtk::SpinButton primaryPercent, secondaryPercent;
	Gtk::HBox primaryEmpty, secondaryEmpty;
	Gtk::HBox primarySecondsBox, secondarySecondsBox;
	Gtk::HBox primaryPercentBox, secondaryPercentBox;

	enum { EXT_NONE, EXT_TOGGLE, EXT_SECONDS, EXT_PERCENT } p_ext, s_ext;

	Action getAction(bool primary);
	void   setAction(const Action &cmd, bool primary);

	void onPrimaryActionChange();
	void onSecondaryActionChange();
	void onActionChange(bool primary);

	ConfigRowBase(Gtk::Grid *parent, int i);
	INLINE ~ConfigRowBase() {}

  public:
	Action getPrimaryAction() { return getAction(true); }
	Action getSecondaryAction() { return getAction(false); }

	static const int \
	NAME_INDEX, NAME_WIDTH, \
	DOWN_INDEX, DOWN_WIDTH, \
	P_ACTION_INDEX, S_ACTION_INDEX, \
	P_EXT_INDEX, S_EXT_INDEX, \
	ACTION_WIDTH, EXT_WIDTH;
};

class ButtonConfigRow : public ConfigRowBase {
  private:
	Gtk::Label name, isDown;
	Gtk::HBox nameBox, spacer;

	Glib::Dispatcher updatePedalDown;
	void onPedalStatusChange();
	std::atomic<bool> pedalDown;

	void connectSignals();

  public:
	ButtonConfigRow(Gtk::Grid *parent, int i);
	ButtonConfigRow(Gtk::Grid *parent, int i, const Action &action1, const Action &action2);
	~ButtonConfigRow();

	INLINE void updatePedalStatus(bool down) {
		pedalDown = down;
		updatePedalDown.emit();
	}
};


class AxisConfigRow : public ConfigRowBase {
  private:
	Gtk::Button name;
	Gtk::Label configLabel;
	Gtk::HBox configLabelBox, spacer;
	Gtk::Label isDownLabel;
	SimpleBar isDownBar;

	Glib::Dispatcher updatePedalDown;
	void onPedalStatusChange();
	std::atomic<bool> pedalDown;

	std::mutex configureLock;
	float deadzone;
	bool isInverted;

	int axisNum;

	void connectSignals();
	void onNameClick();

  public:
	static const int CONFIG_INDEX, CONFIG_WIDTH;

	INLINE bool isConfigured() { return(deadzone >= 0); }
	INLINE bool getIsInverted() { return isInverted; }
	INLINE float getDeadzone() { return deadzone; }

	INLINE void updatePedalStatus(float val) {
		configureLock.lock();
		if (deadzone < 0) {
			configureLock.unlock();
			isDownBar.set_value_async(isInverted ? (1.0 - val) : val);
		} else {
			register const bool down = ((val > deadzone) != isInverted);
			configureLock.unlock();
			if (pedalDown != down) {
				pedalDown = down;
				updatePedalDown.emit();
			}
		}

	}
	void configure(float _deadzone, bool _isInverted);

	AxisConfigRow(Gtk::Grid *parent, int i, int j);
	AxisConfigRow(Gtk::Grid *parent, int i, int j, const Action &action1, const Action &action2, float _deadzone, bool _isInverted);
	~AxisConfigRow();
};

class AxisConfigWindow : public Gtk::Window {
	/*
	 * Step 0: Confirm/Identify Pedal
	 * Step 1: Get minimum value
	 * Step 2: Get maximum value
	 * Step 3: Confirm min and max values
	 * Step 4: Set deadzone
	 */
  private:
	Gtk::VBox rootLayout;
	Gtk::VBox barAndSliderLayout;
	Gtk::HBox middleLayout, bottomLayout;
	Gtk::VBox vspacer1, vspacer2;
	Gtk::HBox hspacer1, hspacer2, hspacer3;

	SimpleBar bar;
	Gtk::HScale deadzoneSlider;
	Gtk::Button cancel, previous, next;
	Gtk::Label message;

	std::mutex stepLock;

	unsigned char step;
	float min_value, max_value, deadzone;
	float axisPos;
	bool finished;

	void loadStep();
	void nextStep();
	void prevStep();

	bool onSliderMoved(__attribute__((unused)) Gtk::ScrollType type, double value) {
		deadzone = value;
		bar.set_threshold(value);
		return true;
	}

  protected:
	virtual void on_hide();

  public:
	// start() should only be called from the graphics thread
	INLINE void start() {
		finished = false;
		set_transient_for(*WindowList::config);
		step = 0;
		axisPos = 0.f;
		loadStep();
		show();
	}

	// joystick event calls this function (safe to call from any thread)
	INLINE void axisMoved(float value);

	INLINE AxisConfigWindow() {
		set_title("Axis Configuration");
		set_border_width(8);
		set_modal(true);
		resize(600, 300);
		finished = false;

		add(rootLayout);
			rootLayout.pack_start(message, false, false);
			rootLayout.pack_start(vspacer1, true, true);
			rootLayout.pack_start(middleLayout, false, false);
				middleLayout.pack_start(hspacer1, true, true);
				middleLayout.pack_start(barAndSliderLayout, false, false);
					barAndSliderLayout.pack_start(bar, false, false);
					barAndSliderLayout.pack_start(deadzoneSlider, false, false);
				middleLayout.pack_start(hspacer2, true, true);
			rootLayout.pack_start(vspacer2, true, true);
			rootLayout.pack_start(bottomLayout, false, false);
				bottomLayout.pack_start(hspacer3, true, true);
				bottomLayout.pack_start(cancel, false, false);
				bottomLayout.pack_start(previous, false, false);
				bottomLayout.pack_start(next, false, false);

		bar.set_minimum_size(200, 30);
		message.set_line_wrap(true);
		message.set_ellipsize(Pango::ELLIPSIZE_NONE);
		cancel.set_label("Cancel");
		previous.set_label("Back");

		deadzoneSlider.set_range(0.0, 1.0);
		deadzoneSlider.set_increments(0.05, 0.2);
		deadzoneSlider.set_value(0.2);
		deadzoneSlider.set_draw_value(false);

		deadzoneSlider.signal_change_value().connect(sigc::mem_fun(*this, &AxisConfigWindow::onSliderMoved));
		cancel.signal_clicked().connect(sigc::mem_fun(*this, &AxisConfigWindow::hide));
		previous.signal_clicked().connect(sigc::mem_fun(*this, &AxisConfigWindow::prevStep));
		next.signal_clicked().connect(sigc::mem_fun(*this, &AxisConfigWindow::nextStep));

		show_all_children();

		step = 0;
		min_value = 0.f;
		max_value = 1.f;
		axisPos = 0.f;
		loadStep();
	}
	INLINE ~AxisConfigWindow() {}
};

class ConfigWindow : public Gtk::Window {
  private:
	struct UpdateDeviceEvent {
		PedalInfo device;
		int port;
		bool connect; // true = connection, false = disconnection
	};

	FootPedalCoordinator *pedals;
	std::vector<FootPedalConfiguration*> configs;
	std::vector<FootPedalConfiguration*> prevConfigs;
	std::map<int, PedalInfo> pedalInfo;
	int currentDevice;
	int axisBeingConfigured;
	AxisConfigWindow acw;
	bool windowOpen;

	void onDeviceConnect(const PedalInfo &info, int port);
	void onDeviceDisconnect(int port);
	void onDeviceEvent(const PedalEvent &ev, int port);

	void onDeviceListChange();
	void onDeviceSelected();

	void onHelpButtonPressed() const;

	void saveCurrentDeviceSettings();

	void onRevertPressed();
	void onCancelPressed();
	void onApplyPressed();
	void onOkayPressed();

	void onPermissionChangeButtonPressed();

	Gtk::TreeView devicesView;
	Glib::RefPtr<Gtk::ListStore> devicesModel;
	Glib::RefPtr<Gtk::TreeModelFilter> devicesFilter;

	Gtk::TreeModel::ColumnRecord devicesColumns;
	Gtk::TreeModelColumn<Glib::ustring> stringColumnType;
	Gtk::TreeModelColumn<int> intColumnType;
	Gtk::TreeModelColumn<bool> boolColumnType;

	Glib::Dispatcher updateDeviceList;
	std::queue<UpdateDeviceEvent> UDLQueue;
	std::mutex UDLLock;
	std::mutex eventLock;

	Gtk::Grid rootLayout, confLayout;
	Gtk::Frame topFrame, bottomFrame;
	Gtk::VBox topLayout, bottomLayout;
	Gtk::CheckButton filterCheckbox;
	Gtk::ScrolledWindow scrollArea;
	Gtk::VBox protectedDeviceLayout;
	Gtk::Label PDLabel;
	Gtk::Button PDButton;
	Gtk::HBox PDButtonBox;
	Gtk::HBox PDSpacer1, PDSpacer2;
	Gtk::VBox PDSpacer3;
	Gtk::HBox buttonLayout, buttonSublayout, buttonSpacer;
	Gtk::HBox CRAOLayout, CRAOSublayout, CRAOSpacer;
	Gtk::Label selectDeviceLabel;
	Gtk::Button refreshButton, helpButton;
	Gtk::Button cancelButton, revertButton, applyButton, okayButton;

	Gtk::Label nameHeader, isDownHeader, primaryActionHeader, secondaryActionHeader;
	Gtk::HBox headerSpacer1, headerSpacer2, headerSpacer3, headerSpacer4;
	Gtk::HBox headerBox1, headerBox2, headerBox3, headerBox4;

	std::vector<ButtonConfigRow*> buttonRows;
	std::vector<AxisConfigRow*> axisRows;

	Gtk::Image helpIcon, refreshIcon;

	USERET INLINE bool configsChanged() const {
		if (configs.size() != prevConfigs.size()) return true;

		for (size_t i = 0; i < configs.size(); i++) {
			const FootPedalConfiguration &other = *(configs[i]);
			register bool matchFound = false;
			for (size_t j = 0; j < prevConfigs.size(); j++) {
				if (other == *(prevConfigs[j])) {
					matchFound = true;
					break;
				}
			}
			if (!matchFound) return true;
		}

		return false;
	}


	INLINE static void copyConfig(std::vector<FootPedalConfiguration*> &dest, const std::vector<FootPedalConfiguration*> &src) {
		for (FootPedalConfiguration *conf : dest) delete conf;
		dest.clear();

		dest.reserve(src.size());
		for (FootPedalConfiguration *conf : src) dest.push_back(new FootPedalConfiguration(*conf));
	}

	bool protectedDeviceFilter(const Gtk::TreeModel::const_iterator &row) const {
		return (filterCheckbox.get_active() || !((*row)[boolColumnType]));
	}
	void onFilterCheckboxClicked() {
		devicesFilter->refilter();
	}

  protected:
	virtual void on_show() {
		assert(configs.empty());
		windowOpen = true;

		try {
			configs = loadFootpedalConfiguration();
		} catch (const std::ios_base::failure &ex) {
			for (FootPedalConfiguration *C : configs) delete C;
			configs.clear();
			try {
				saveFootpedalConfiguration(configs);
				Gtk::MessageDialog(*this, Glib::ustring("Error loading footpedal configuration file. You will need to reconfigure your footpedal(s)."), false, Gtk::MESSAGE_ERROR);
			} catch (const std::ios_base::failure &ex2) {
				Gtk::MessageDialog(*this, Glib::ustring("Could not load or save footpedal configuration to file. Do you have permission to access your .conf directory?"), false, Gtk::MESSAGE_ERROR).run();
			}
		}
		copyConfig(prevConfigs, configs); // prevConfigs = configs

		pedals->configurationMode();
		Gtk::Window::on_show();
	}

	virtual void on_hide() {
		saveCurrentDeviceSettings();
		if (configsChanged()) {
			if (Gtk::MessageDialog(Glib::ustring("You have modified the configuration of at least one device. Do you want to save these changes?"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true).run() == Gtk::RESPONSE_YES) {
				saveCurrentDeviceSettings();
				try {
					saveFootpedalConfiguration(configs);
				} catch (const std::ios_base::failure &ex) {
					Gtk::MessageDialog(*this, Glib::ustring(ex.what()), false, Gtk::MESSAGE_ERROR).run();
				}
			} else {
				copyConfig(configs, prevConfigs); // configs = prevConfigs
			}
		}

		pedals->dictationMode(configs);
		//The fact that I do not delete each entry in configs is intentional
		//These are copied to the FootPedalCoordinator object and later freed there

		configs.clear();
		windowOpen = false;

		for (FootPedalConfiguration *conf : prevConfigs) delete conf;
		prevConfigs.clear();

		devicesModel->clear();
		currentDevice = -1;

		Gtk::Window::on_hide();
	}

  public:
	INLINE ConfigWindow(FootPedalCoordinator *fpc) : Gtk::Window(), pedals(fpc) {
		set_title("Footpedal Configuration");
		currentDevice = -1;
		axisBeingConfigured = -1;
		windowOpen = false;

		/* Icons */
		Gtk::IconInfo helpIconInfo = Gtk::IconTheme::get_default()->lookup_icon("help", 16, Gtk::ICON_LOOKUP_FORCE_SIZE);
		if (!helpIconInfo) helpIconInfo = Gtk::IconTheme::get_default()->lookup_icon("help-browser", 16, Gtk::ICON_LOOKUP_FORCE_SIZE);
		if (helpIconInfo) {
			helpIcon.set(helpIconInfo.load_icon());
		} else {
			helpIcon.set(INSTALL_DIR "/OpenScribe/icons/fallback/help.svg");
		}


		Gtk::IconInfo refreshIconInfo = Gtk::IconTheme::get_default()->lookup_icon("view-refresh", 16, Gtk::ICON_LOOKUP_FORCE_SIZE);
		if (!refreshIconInfo) refreshIconInfo = Gtk::IconTheme::get_default()->lookup_icon("refresh", 16, Gtk::ICON_LOOKUP_FORCE_SIZE);
		if (refreshIconInfo) {
			refreshIcon.set(refreshIconInfo.load_icon());
		} else {
			refreshIcon.set(INSTALL_DIR "/OpenScribe/icons/fallback/refresh.svg");
		}

		/* Device List setup */
		devicesColumns.add(intColumnType);
		devicesColumns.add(stringColumnType);
		devicesColumns.add(boolColumnType);

		devicesModel = Gtk::ListStore::create(devicesColumns);
		devicesFilter = Gtk::TreeModelFilter::create(devicesModel);
		devicesView.set_model(devicesFilter);

		filterCheckbox.set_active(true);
		devicesFilter->set_visible_func(sigc::mem_fun(*this, &ConfigWindow::protectedDeviceFilter));

		devicesView.set_headers_clickable(false);
		devicesView.set_rubber_banding(false);
		devicesView.get_selection()->set_mode(Gtk::SelectionMode::SELECTION_SINGLE);

		Gtk::CellRendererText* cell = Gtk::manage(new Gtk::CellRendererText());
		cell->property_foreground_rgba() = Gdk::RGBA("red");
		devicesView.append_column(Glib::ustring("Connected Devices"), *cell);
		devicesView.get_column(0)->add_attribute(cell->property_text(), stringColumnType);
		devicesView.get_column(0)->add_attribute(cell->property_foreground_set(), boolColumnType);

		updateDeviceList.connect(sigc::mem_fun(*this, &ConfigWindow::onDeviceListChange));
		devicesView.get_selection()->signal_changed().connect(sigc::mem_fun(*this, &ConfigWindow::onDeviceSelected));

		/* Widget setup */
		selectDeviceLabel.set_label(Glib::ustring("Select a device to configure from the list above."));
		refreshButton.set_label(Glib::ustring("  Refresh"));
		refreshButton.set_image(refreshIcon);
		refreshButton.set_image_position(Gtk::POS_LEFT);
		helpButton.set_label(Glib::ustring("  Help"));
		helpButton.set_image(helpIcon);
		helpButton.set_image_position(Gtk::POS_LEFT);
		cancelButton.set_label("Cancel");
		revertButton.set_label("Revert");
		applyButton.set_label("Apply");
		okayButton.set_label("Okay");
		scrollArea.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

		PDLabel.set_line_wrap(true);
		PDLabel.set_line_wrap_mode(Pango::WRAP_WORD);
		PDLabel.set_justify(Gtk::JUSTIFY_FILL);
		PDLabel.set_label(
			"\nThis input device is protected.\n\n"
			"Certain input devices, such as a mouse or keyboard, are protected for security reasons to prevent user level applications from reading them directly. "
			"Devices such as joysticks, gamepads, and footpedals should not be protected as they are intended to be read directly by any user.\n\n"
			"However, some footpedal manufacturers incorrectly configure their devices, causing them to be protected when they should not be. "
			"In this case, you must give OpenScribe permission to read this device's input. If you are an administrator on your machine, you may click the button below to "
			"temporarily make this input device readable by your user account until you unplug the device or restart your system. You will be prompted to enter your password.\n"
		);
		PDButton.set_label("Unprotect Device");

		filterCheckbox.signal_clicked().connect(sigc::mem_fun(*this, &ConfigWindow::onFilterCheckboxClicked));
		refreshButton.signal_clicked().connect(sigc::mem_fun(*pedals, &FootPedalCoordinator::syncDevices));
		helpButton.signal_clicked().connect(sigc::mem_fun(*this, &ConfigWindow::onHelpButtonPressed));

		PDButton.signal_clicked().connect(sigc::mem_fun(*this, &ConfigWindow::onPermissionChangeButtonPressed));

		cancelButton.signal_clicked().connect(sigc::mem_fun(*this, &ConfigWindow::onCancelPressed));
		revertButton.signal_clicked().connect(sigc::mem_fun(*this, &ConfigWindow::onRevertPressed));
		applyButton.signal_clicked().connect(sigc::mem_fun(*this, &ConfigWindow::onApplyPressed));
		okayButton.signal_clicked().connect(sigc::mem_fun(*this, &ConfigWindow::onOkayPressed));

		/* Layout */
		resize(815, 550);
		set_border_width(5);
		rootLayout.set_column_homogeneous(true);
		rootLayout.set_row_homogeneous(true);
		rootLayout.set_row_spacing(10);
		confLayout.set_column_homogeneous(true);
		buttonSublayout.set_homogeneous(true);
		CRAOSublayout.set_homogeneous(true);
		CRAOSublayout.set_spacing(5);

		nameHeader.set_markup(Glib::ustring("<b>Pedal</b>"));
		isDownHeader.set_markup(Glib::ustring("<b>Pressed?</b>"));
		primaryActionHeader.set_markup(Glib::ustring("<b>Primary Action</b>"));
		secondaryActionHeader.set_markup(Glib::ustring("<b>Secondary Action</b>"));
		filterCheckbox.set_label("Show protected devices");

		headerBox1.pack_start(nameHeader, false, false);
		headerBox1.pack_start(headerSpacer1, true, true);
		confLayout.attach(headerBox1, ConfigRowBase::NAME_INDEX, 0, ConfigRowBase::NAME_WIDTH, 1);

		headerBox2.pack_start(isDownHeader, false, false);
		headerBox2.pack_start(headerSpacer2, true, true);
		confLayout.attach(headerBox2, ConfigRowBase::DOWN_INDEX, 0, ConfigRowBase::DOWN_WIDTH, 1);

		headerBox3.pack_start(primaryActionHeader, false, false);
		headerBox3.pack_start(headerSpacer3, true, true);
		confLayout.attach(headerBox3, ConfigRowBase::P_ACTION_INDEX, 0, ConfigRowBase::ACTION_WIDTH + ConfigRowBase::EXT_WIDTH, 1);

		headerBox4.pack_start(secondaryActionHeader, false, false);
		headerBox4.pack_start(headerSpacer4, true, true);
		confLayout.attach(headerBox4, ConfigRowBase::S_ACTION_INDEX, 0, ConfigRowBase::ACTION_WIDTH + ConfigRowBase::EXT_WIDTH, 1);

		add(rootLayout);
			rootLayout.attach(topFrame, 0, 0, 1, 2);
				topFrame.add(topLayout);
					topLayout.pack_start(devicesView, true, true);
					topLayout.pack_start(buttonLayout, false, false);
						buttonLayout.pack_start(filterCheckbox, false, false);
						buttonLayout.pack_start(buttonSpacer, true, true);
						buttonLayout.pack_start(buttonSublayout, false, false);
							buttonSublayout.pack_start(refreshButton);
							buttonSublayout.pack_start(helpButton);
			rootLayout.attach(bottomFrame, 0, 2, 1, 3);
				bottomFrame.add(bottomLayout);
					bottomLayout.pack_start(scrollArea, true, true);
						scrollArea.add(confLayout);
					bottomLayout.pack_start(selectDeviceLabel, true, true);
					bottomLayout.pack_start(protectedDeviceLayout, true, true);
						protectedDeviceLayout.pack_start(PDLabel, false, false);
						protectedDeviceLayout.pack_start(PDButtonBox, false, false);
							PDButtonBox.pack_start(PDSpacer1, true, true);
							PDButtonBox.pack_start(PDButton, false, false);
							PDButtonBox.pack_start(PDSpacer2, true, true);
						protectedDeviceLayout.pack_start(PDSpacer3, true, true);
					bottomLayout.pack_start(CRAOLayout, false, false);
						CRAOLayout.pack_start(CRAOSpacer, true, true);
						CRAOLayout.pack_start(CRAOSublayout, false, false);
							CRAOSublayout.pack_start(cancelButton);
							CRAOSublayout.pack_start(revertButton);
							CRAOSublayout.pack_start(applyButton);
							CRAOSublayout.pack_start(okayButton);
		show_all_children();
		scrollArea.hide();
		protectedDeviceLayout.hide();
	}

	~ConfigWindow() {
		for (ButtonConfigRow *row : buttonRows) delete row;
		for (AxisConfigRow *row : axisRows) delete row;
	}

	/* Event Handler Adaptors. It's a singleton class, so the implicit 'this' argument is known */
	static void onDeviceConnectAdaptor(const PedalInfo &info, int port) { ((ConfigWindow*)WindowList::config)->onDeviceConnect(info, port); }
	static void onDeviceDisconnectAdaptor(int port) { ((ConfigWindow*)WindowList::config)->onDeviceDisconnect(port); }
	static void onDeviceEventAdaptor(const PedalEvent &ev, int port) { ((ConfigWindow*)WindowList::config)->onDeviceEvent(ev, port); }

	// Called by AxisConfigRow to open the window for configuring an axis
	void openAxisConfigurationWindow(int axis);

	// AxisConfigWindow calls one of these function when it's done
	void onAxisConfigured(float deadzone, bool isInverted);
	void onAxisConfigurationCancelled();

};

#endif /* CONFIGWINDOW_HPP_ */
