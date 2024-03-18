/*
 * Copyright 2024, Emir SARI <emir_sari@icloud.com>.
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SettingsWindow.h"

#include <Application.h>
#include <Catalog.h>
#include <LayoutBuilder.h>

#include "Database.h"
#include "EscapeCancelFilter.h"

#include "Settings.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SettingsWindow"

SettingsWindow::SettingsWindow(Settings* settings)
	: BWindow(
		  BRect(0, 0, 300, 300), B_TRANSLATE("CapitalBe settings"), B_TITLED_WINDOW, B_NOT_ZOOMABLE,
		  B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS |
			  B_SUPPORTS_LAYOUT
	  )
{
	fSettings = settings;
	fStartSettings = new Settings();
	*fStartSettings = *fSettings;
}

SettingsWindow::~SettingsWindow()
{
	if (fStartSettings)
		delete fStartSettings;
}

void
SettingsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case Actions::USE_SYSTEM_DEFAULTS: {
	} break;
	default: {
		BWindow::MessageReceived(message);
	} break;
	}
}

void
SettingsWindow::Quit()
{
	be_app->PostMessage(APP_SETTINGS_QUITTING);

	BWindow::Quit();
}
