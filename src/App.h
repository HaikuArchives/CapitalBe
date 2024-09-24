/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	humdinger (Joachim Seemer)
 */
#ifndef APP_H
#define APP_H

#include <Application.h>
#include <Message.h>

#include "MainWindow.h"

#define M_QUIT_NOW 'qnow'

class App : public BApplication {
public:
	App();
	~App();

	virtual void AboutRequested();
	void RefsReceived(BMessage* msg);
	void MessageReceived(BMessage* msg);
	void ReadyToRun();

private:
	void _InstallMimeType();
	void _ShowAlert(BString text);
	void _ShowMainWindow(BPath path);

	MainWindow* fMainWindow;
};

#endif	// APP_H
