#include "TransactionEditWindow.h"
#include <Catalog.h>
#include <LayoutBuilder.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TransactionEditWindow"


#define M_TOGGLE_SPLIT 'tspl'

TransactionEditWindow::TransactionEditWindow(const BRect& frame, const TransactionData& trans)
	: BWindow(frame, B_TRANSLATE("Edit transaction"), B_DOCUMENT_WINDOW_LOOK,
		  B_FLOATING_APP_WINDOW_FEEL, B_AUTO_UPDATE_SIZE_LIMITS)
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
