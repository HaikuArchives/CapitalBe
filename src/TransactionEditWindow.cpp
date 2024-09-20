/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	bitigchi (Emir Sari)
 *	dospuntos (Johan Wagenheim)
 *	humdinger (Joachim Seemer)
 *	raefaldhia (Raefaldhi Amartya Junior)
 */
#include "TransactionEditWindow.h"

#include <Catalog.h>
#include <LayoutBuilder.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TransactionEdit"


#define M_TOGGLE_SPLIT 'tspl'


TransactionEditWindow::TransactionEditWindow(const TransactionData& trans)
	:
	BWindow(BRect(), B_TRANSLATE("Edit transaction"), B_DOCUMENT_WINDOW_LOOK,
		B_FLOATING_APP_WINDOW_FEEL, B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE)
{
	fSplitView = new SplitView("splitview", trans, B_WILL_DRAW);
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0).SetInsets(0).Add(fSplitView).End();

	AddShortcut('S', B_COMMAND_KEY, new BMessage(M_TOGGLE_SPLIT));
	AddShortcut('A', B_COMMAND_KEY, new BMessage(M_ADD_SPLIT));
	AddShortcut('R', B_COMMAND_KEY, new BMessage(M_REMOVE_SPLIT));
	AddShortcut('Q', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddShortcut(B_UP_ARROW, B_COMMAND_KEY, new BMessage(M_PREVIOUS_SPLIT));
	AddShortcut(B_DOWN_ARROW, B_COMMAND_KEY, new BMessage(M_NEXT_SPLIT));
}


void
TransactionEditWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_TOGGLE_SPLIT:
		{
			fSplitView->ToggleSplit();
			break;
		}
		case M_ADD_SPLIT:
		case M_REMOVE_SPLIT:
		case M_NEXT_SPLIT:
		case M_PREVIOUS_SPLIT:
		{
			fSplitView->MessageReceived(msg);
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}
