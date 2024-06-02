#include "PrefWindow.h"

#include <Box.h>
#include <Catalog.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuItem.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PrefWindow"


// PrefWindow
#define M_EDIT_OPTIONS 'edop'

// PrefView
enum {
	M_NEW_CURRENCY_SYMBOL,
	M_NEW_CURRENCY_SEPARATOR,
	M_NEW_CURRENCY_DECIMAL,
	M_TOGGLE_PREFIX
};


PrefWindow::PrefWindow(const BRect& frame)
	: BWindow(frame, B_TRANSLATE("Settings"), B_FLOATING_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		  B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS |
			  B_CLOSE_ON_ESCAPE)
{
	BString temp;
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddShortcut('Q', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	fLabel = new BStringView("windowlabel", B_TRANSLATE("Default account settings"));
	BFont font(be_bold_font);
	font.SetSize(font.Size() * 1.2f);
	fLabel->SetFont(&font);

	fOK = new BButton("okbutton", B_TRANSLATE("OK"), new BMessage(M_EDIT_OPTIONS));

	BButton* cancel =
		new BButton("cancelbutton", B_TRANSLATE("Cancel"), new BMessage(B_QUIT_REQUESTED));

	SetDefaultButton(fOK);

	// clang-format off
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.AddGlue()
			.Add(fLabel)
			.AddGlue()
			.End()
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.AddGlue()
			.Add(cancel)
			.Add(fOK)
			.End()
		.End();
	// clang-format on
	ResizeToPreferred();
	CenterIn(frame);
}

void
PrefWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_EDIT_OPTIONS:
		{
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
		{
			BWindow::MessageReceived(msg);
			break;
		}
	}
}
