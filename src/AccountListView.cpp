#include "AccountListView.h"
#include <Catalog.h>
#include <Font.h>
#include <ListView.h>
#include <String.h>
#include <View.h>

#include "Account.h"
#include "CBLocale.h"
#include "MainWindow.h"
#include "Preferences.h"
#include "RegisterView.h"
#include "TransactionLayout.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Account"


AccountListItem::AccountListItem(Account* acc)
	:
	BListItem()
{
	fAccount = acc;
	fEnabled = true;
}


void
AccountListItem::SetEnabled(bool enabled)
{
	fEnabled = enabled;
}


void
AccountListItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	// Draw item background
	owner->SetHighUIColor(
		IsSelected() ? B_LIST_SELECTED_BACKGROUND_COLOR : B_LIST_BACKGROUND_COLOR);
	owner->FillRect(frame);

	// Draw item border
	if (IsSelected()) {
		owner->SetHighUIColor(fAccount->IsClosed() ? B_FAILURE_COLOR : B_CONTROL_MARK_COLOR);
		owner->StrokeRect(frame);
	}

	// Draw account title
	owner->SetFont(be_bold_font);
	if (IsSelected()) {
		owner->SetHighUIColor(B_LIST_SELECTED_ITEM_TEXT_COLOR,
			fAccount->IsClosed()
				? GetMutedTint(ui_color(B_LIST_SELECTED_BACKGROUND_COLOR), CB_MUTED_TEXT)
				: B_NO_TINT);
	} else {
		owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR,
			fAccount->IsClosed() ? GetMutedTint(ui_color(B_LIST_BACKGROUND_COLOR), CB_MUTED_TEXT)
								 : B_NO_TINT);
	}

	// Vertically center the two lines of text
	font_height fh;
	owner->GetFontHeight(&fh);
	float fontHeight = floorf(fh.ascent + fh.descent + fh.leading);
	frame.top += (frame.Height() - fontHeight * 2 + floorf(fh.descent - 1)) / 2;

	BFont font;
	owner->DrawString(fAccount->Name(), BPoint(frame.left + 5, frame.top + (font.Size())));

	// Draw Balance (or "Closed")
	owner->SetFont(be_plain_font);
	if (fAccount->IsClosed()) {
		owner->DrawString(B_TRANSLATE("Closed"),
			BPoint(frame.left + 5, frame.top + (font.Size() * 2)));
	} else {
		BString text;
		fAccount->GetLocale().CurrencyToString(fAccount->Balance(), text);
		owner->DrawString(text.String(), BPoint(frame.left + 5, frame.top + (font.Size() * 2)));
	}
}


void
AccountListItem::Update(BView* owner, const BFont* finfo)
{
	BListItem::Update(owner, finfo);

	// We can afford to make this call because the row height is just a sane
	// value based on the height of be_plain_font, which we are also using here
	SetHeight(TRowHeight() * 2);
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


AccountList::AccountList()
	:
	BListView("AccountList"),
	fShowingPopUpMenu(false)
{
}


AccountList::~AccountList()
{
}


void
AccountList::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case M_CONTEXT_CLOSE:
		{
			fShowingPopUpMenu = false;
			break;
		}
		default:
		{
			BListView::MessageReceived(message);
			break;
		}
	}
}


void
AccountList::MouseDown(BPoint position)
{
	uint32 buttons = 0;
	if (Window() != NULL && Window()->CurrentMessage() != NULL)
		buttons = Window()->CurrentMessage()->FindInt32("buttons");

	if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
		Select(IndexOf(position));
		_ShowPopUpMenu(position);
		return;
	}

	BView::MouseDown(position);
}

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


void
AccountList::_ShowPopUpMenu(BPoint position)
{
	if (fShowingPopUpMenu || IsEmpty() || IndexOf(position) < 0)
		return;

	AccountContext* menu = new AccountContext("PopUpMenu", this);

	menu->AddItem(new BMenuItem(B_TRANSLATE("Reconcile" B_UTF8_ELLIPSIS),
		new BMessage(M_SHOW_RECONCILE_WINDOW), 'R'));
	menu->AddSeparatorItem();
	menu->AddItem(
		new BMenuItem(B_TRANSLATE("New" B_UTF8_ELLIPSIS), new BMessage(M_SHOW_NEW_ACCOUNT), 'N'));
	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Delete" B_UTF8_ELLIPSIS), new BMessage(M_DELETE_ACCOUNT)));

	AccountListItem* item = dynamic_cast<AccountListItem*>(ItemAt(IndexOf(position)));
	Account* acc = item->GetAccount();
	BString label = acc->IsClosed() ? B_TRANSLATE("Reopen") : B_TRANSLATE("Close");
	menu->AddItem(new BMenuItem(label, new BMessage(M_CLOSE_ACCOUNT)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Settings" B_UTF8_ELLIPSIS),
		new BMessage(M_SHOW_ACCOUNT_SETTINGS)));

	menu->SetTargetForItems(Window());
	menu->Go(ConvertToScreen(position), true, true, true);
	fShowingPopUpMenu = true;
}


AccountContext::~AccountContext()
{
	fTarget.SendMessage(M_CONTEXT_CLOSE);
}


AccountContext::AccountContext(const char* name, BMessenger target)
	:
	BPopUpMenu(name, false, false),
	fTarget(target)
{
	SetAsyncAutoDestruct(true);
}
