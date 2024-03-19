/*
 * Copyright 2024, Emir SARI <emir_sari@icloud.com>.
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H


#include <Window.h>

#include "Settings.h"


const uint32 APP_SETTINGS_CHANGED = 'apch';
const uint32 APP_SETTINGS_QUITTING = 'APQU';

class SettingsWindow : public BWindow {
  public:
	SettingsWindow(Settings* settings);
	~SettingsWindow();

	void MessageReceived(BMessage* message);
	void Quit();

  private:
	enum Actions {
		USE_SYSTEM_DEFAULTS = 'usdf',
	};

	Settings* fSettings;
	Settings* fStartSettings;
};


#endif // SETTINGS_WINDOW_H
