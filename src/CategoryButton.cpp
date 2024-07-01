#include "CategoryButton.h"
#include "Database.h"
#include "MainWindow.h"
#include "SplitView.h"

#include <Bitmap.h>
#include <Catalog.h>
#include <MenuItem.h>
#include <Picture.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


enum {
	M_SHOW_POPUP,
	M_CLOSE_POPUP,
	M_CATEGORY_CHOSEN
};


CategoryButton::CategoryButton(CategoryBox* categorybox)
	: BButton("calenderbutton", "", new BMessage(M_SHOW_POPUP)),
	  fCategoryBox(categorybox)
{
	float height;
	fCategoryBox->GetPreferredSize(NULL, &height);
	BSize size(height - 2, height);
	SetExplicitSize(size);

	SetIcon(DrawIcon());
}


void
CategoryButton::AttachedToWindow(void)
{
	SetTarget(this);
}


void
CategoryButton::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_SHOW_POPUP:
		{
			ShowPopUpMenu();
			break;
		}
		case M_CLOSE_POPUP:
		{
			fShowingPopUpMenu = false;
			break;
		}
		case M_CATEGORY_CHOSEN:
		{
			BString category;
			msg->FindString("category", &category);
			fCategoryBox->SetText(category);
			bool success = fCategoryBox->Validate();

			if (success) {
				BMessenger* msgr(fCategoryBox->GetFilter()->GetMessenger());
				BMessage notice(M_SPLIT_CATEGORY_CHANGED);
				msgr->SendMessage(&notice);
			}
			break;
		}
		default:
		{
			BButton::MessageReceived(msg);
		}
	}
}


BBitmap*
CategoryButton::DrawIcon()
{
	font_height fh;
	GetFontHeight(&fh);
	float fontHeight = floorf(fh.ascent + fh.descent + fh.leading);
	BRect rect(0, 0, fontHeight - 2, fontHeight);

	BView* view = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	BBitmap* bitmap = new BBitmap(rect, B_RGBA32, true);
	bitmap->Lock();
	bitmap->AddChild(view);

	view->SetDrawingMode(B_OP_COPY);
	view->SetHighUIColor(B_PANEL_BACKGROUND_COLOR);
	view->SetLowUIColor(B_PANEL_BACKGROUND_COLOR);

	view->FillRect(rect);
	view->SetHighUIColor(B_CONTROL_TEXT_COLOR);
	view->SetPenSize(1.0);
	view->StrokeRect(rect);
	view->FillTriangle(rect.LeftBottom(), rect.RightTop(), rect.LeftTop());

	BFont font;
	view->GetFont(&font);
	font.SetFace(B_BOLD_FACE);
	font.SetFlags(B_DISABLE_ANTIALIASING);
	view->SetFont(&font, B_FONT_FACE | B_FONT_FLAGS);

	view->DrawString("̵", BPoint(ceilf((rect.Width() * 0.75) - view->StringWidth("̵") / 2 - 1),
							 ceilf(rect.Height() * 0.75 + fontHeight / 5)));

	view->SetHighUIColor(B_PANEL_BACKGROUND_COLOR);
	view->DrawString("+", BPoint(ceilf((rect.Width() / 4) - view->StringWidth("+") / 2 + 1),
							  ceilf(rect.Height() / 5 + fontHeight / 3)));

	view->RemoveSelf();
	bitmap->Unlock();
	delete view;

	return bitmap;
}


void
CategoryButton::ShowPopUpMenu()
{
	if (fShowingPopUpMenu)
		return;

	CategoryPopUp* menu = new CategoryPopUp("PopUpMenu", this);
	BMenu* incomeMenu = new BMenu(B_TRANSLATE_CONTEXT("Income", "CommonTerms"));
	BMenu* spendingMenu = new BMenu(B_TRANSLATE_CONTEXT("Spending", "CommonTerms"));
	BMenuItem* editCategories = new BMenuItem(
		B_TRANSLATE("Edit categories" B_UTF8_ELLIPSIS), new BMessage(M_SHOW_CATEGORY_WINDOW));

	CppSQLite3Query query = gDatabase.DBQuery(
		"SELECT * FROM categorylist ORDER BY name ASC", "CategoryView::CategoryView");
	while (!query.eof()) {
		int expense = query.getIntField(1);
		BString name = DeescapeIllegalCharacters(query.getStringField(0));
		if (IsInternalCategory(name.String())) {
			query.nextRow();
			continue;
		}
		BMessage* msg = new BMessage(M_CATEGORY_CHOSEN);
		msg->AddString("category", name);

		if (expense == SPENDING)
			spendingMenu->AddItem(new BMenuItem(name, msg));
		else if (expense == INCOME)
			incomeMenu->AddItem(new BMenuItem(name, msg));

		query.nextRow();
	}

	menu->AddItem(incomeMenu);
	menu->AddItem(spendingMenu);
	menu->AddItem(new BSeparatorItem());
	menu->AddItem(editCategories);

	incomeMenu->SetTargetForItems(this);
	spendingMenu->SetTargetForItems(this);
	editCategories->SetTarget(Window());

	BPoint where = Bounds().LeftTop();
	where.x += 10;
	where.y += 10;
	ConvertToScreen(&where);
	menu->Go(where, true, true, true);

	fShowingPopUpMenu = true;
}


CategoryPopUp::CategoryPopUp(const char* name, BMessenger target)
	: BPopUpMenu(name, false, false),
	  fTarget(target)
{
	SetAsyncAutoDestruct(true);
}


CategoryPopUp::~CategoryPopUp()
{
	fTarget.SendMessage(M_CLOSE_POPUP);
}