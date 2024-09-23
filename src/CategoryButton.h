/*
 * Copyright 2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	humdinger (Joachim Seemer)
 */
#ifndef CATEGORYBUTTON_H
#define CATEGORYBUTTON_H

#include <Button.h>
#include <PopUpMenu.h>
#include <SupportDefs.h>

#include "CategoryBox.h"


class CategoryButton : public BButton {
public:
	CategoryButton(CategoryBox* box);

	void AttachedToWindow();
	virtual void MessageReceived(BMessage* message);

private:
	BBitmap* _DrawIcon();
	void _ShowPopUpMenu();

	CategoryBox* fCategoryBox;
	bool fShowingPopUpMenu;
};


class CategoryPopUp : public BPopUpMenu {
public:
	CategoryPopUp(const char* name, BMessenger target);
	virtual ~CategoryPopUp();

private:
	BMessenger fTarget;
};

#endif	// CATEGORYBUTTON_H
