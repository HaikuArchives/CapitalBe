/*
 * Copyright 2024, Emir SARI <emir_sari@icloud.com>.
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#ifndef SETTINGS_H
#define SETTINGS_H


#include <File.h>
#include <Message.h>
#include <Path.h>

#include <memory>

class Settings {
  public:
	BRect fWindowRect;
	BMessage fFindWindowState;
	BPath fSettingsPath;

	Settings& operator=(Settings& settings);

	bool fUseSystemDefaults;
	void Load(const char* filename);
	void Save(const char* filename);

  private:
	std::shared_ptr<BFile> _OpenFile(const char* filename, uint32 openMode);
};


#endif // SETTINGS_H
