/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	humdinger (Joachim Seemer)
 */
#ifndef TRANSEDITWIN_H
#define TRANSEDITWIN_H

#include "SplitView.h"

#include <Message.h>
#include <Window.h>

class TransactionData;

class TransactionEditWindow : public BWindow {
public:
	TransactionEditWindow(const TransactionData& trans);
	void MessageReceived(BMessage* msg);

private:
	SplitView* fSplitView;
};

#endif // TRANSEDITWIN_H
