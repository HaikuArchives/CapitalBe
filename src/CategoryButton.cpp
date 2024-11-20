/*
 * Copyright 2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	humdinger (Joachim Seemer)
 */
#include "CategoryButton.h"
#include "CategoryWindow.h"
#include "Database.h"
#include "SplitView.h"

#include <Bitmap.h>
#include <Catalog.h>
#include <MenuItem.h>
#include <Picture.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CategoryButton"

// clang-format off
enum {
	M_SHOW_POPUP,
	M_CLOSE_POPUP,
	M_CATEGORY_CHOSEN,
	M_OPEN_CATEGORY_WINDOW,
};
// clang-format on

CategoryButton::CategoryButton(CategoryBox* categorybox)
	: BButton("calenderbutton", "", new BMessage(M_SHOW_POPUP)),
	  fCategoryBox(categorybox),
	  fShowingPopUpMenu(false)
{
	float height;
	fCategoryBox->GetPreferredSize(NULL, &height);
	BSize size(height - 2, height);
	SetExplicitSize(size);

	SetIcon(_DrawIcon());
}


void
CategoryButton::AttachedToWindow()
{
	SetTarget(this);
}


void
CategoryButton::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_SHOW_POPUP:
		{
			_ShowPopUpMenu();
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
		case M_OPEN_CATEGORY_WINDOW:
		{
			CategoryWindow* catwin = new CategoryWindow(BRect(100, 100, 600, 425));
			catwin->CenterIn(Frame());
			catwin->Show();
			break;
		}
		default:
		{
			BButton::MessageReceived(msg);
		}
	}
}


BBitmap*
CategoryButton::_DrawIcon()
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
CategoryButton::_ShowPopUpMenu()
{
	if (fShowingPopUpMenu)
		return;

	CategoryPopUp* menu = new CategoryPopUp("PopUpMenu", this);
	BMenu* expensesMenu = new BMenu(B_TRANSLATE_CONTEXT("Expenses", "CommonTerms"));
	BMenu* incomeMenu = new BMenu(B_TRANSLATE_CONTEXT("Income", "CommonTerms"));
	BMenuItem* editCategories = new BMenuItem(
		B_TRANSLATE("Edit categories" B_UTF8_ELLIPSIS), new BMessage(M_OPEN_CATEGORY_WINDOW));

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

		if (expense == EXPENSES)
			expensesMenu->AddItem(new BMenuItem(name, msg));
		else if (expense == INCOME)
			incomeMenu->AddItem(new BMenuItem(name, msg));

		query.nextRow();
	}

	menu->AddItem(expensesMenu);
	menu->AddItem(incomeMenu);
	menu->AddItem(new BSeparatorItem());
	menu->AddItem(editCategories);

	incomeMenu->SetTargetForItems(this);
	expensesMenu->SetTargetForItems(this);
	editCategories->SetTarget(this);

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
