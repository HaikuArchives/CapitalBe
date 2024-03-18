/*
 * Copyright 2024, Emir SARI <emir_sari@icloud.com>.
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "Settings.h"

#include <Alert.h>
#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Settings"

std::shared_ptr<BFile>
Settings::_OpenFile(const char* filename, uint32 openMode)
{
	if (openMode == B_WRITE_ONLY)
		openMode |= B_CREATE_FILE | B_ERASE_FILE;
	const int index = (openMode == B_READ_ONLY) ? 1 : 2;
	auto file = std::make_shared<BFile>(filename, openMode);
	status_t result = file->InitCheck();
	if (result != B_OK) {
		BString error = B_TRANSLATE("Unknown error.");
		switch (result) {
		case B_BAD_VALUE:
			error = B_TRANSLATE("Encountered an error while opening the configuration file.");
			break;
		case B_PERMISSION_DENIED:
			error = B_TRANSLATE("Could not open the configuration file. "
								"Please check the file permissions.");
			break;
		case B_NO_MEMORY:
			error = B_TRANSLATE("Could not allocate enough memory. "
								"Free some memory and try again.");
			break;
		default:
			// If opened for reading and not found, don't throw "Unknown error."
			if (index == 1)
				return std::shared_ptr<BFile>();
		}
		BAlert* alert = new BAlert(
			B_TRANSLATE("Error"), error, B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL,
			B_EVEN_SPACING, B_WARNING_ALERT
		);
		alert->SetShortcut(0, B_ESCAPE);
		return std::shared_ptr<BFile>();
	}
	return file;
}

void
Settings::Load(const char* filename)
{
	auto file = _OpenFile(filename, B_READ_ONLY);
	BMessage storage;

	if (file)
		storage.Unflatten(file.get());

	fWindowRect = storage.GetRect("windowRect", BRect(50, 50, 500, 500));
	fUseSystemDefaults = storage.GetBool("useSystemDefaults", true);

	if (storage.FindMessage("findWindowState", &fFindWindowState) != B_OK)
		fFindWindowState = BMessage();
}

void
Settings::Save(const char* filename)
{
	auto file = _OpenFile(filename, B_WRITE_ONLY);
	BMessage storage;

	storage.AddRect("windowRect", fWindowRect);
	storage.AddBool("useSystemDefaults", fUseSystemDefaults);

	if (file) {
		storage.Flatten(file.get());
	}
}

Settings&
Settings::operator=(Settings& settings)
{
	fWindowRect = settings.fWindowRect;
	fUseSystemDefaults = settings.fUseSystemDefaults;

	return *this;
}
