/*
 * Copyright 2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	humdinger (Joachim Seemer)
 */
#ifndef HELP_H
#define HELP_H

#include <Button.h>

#define M_HELP 'help'


class HelpButton : public BButton {
public:
	HelpButton(const char* helpfilename, const char* anchor);
	~HelpButton();

	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* msg);

private:
	BString fHelpfile;
	BString fAnchor;
};

void openDocumentation(const char* helpfilename, const char* anchor);
BBitmap* getHelpIcon();

#endif	// HELP_H
