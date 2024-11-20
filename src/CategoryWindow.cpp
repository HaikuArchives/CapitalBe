/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	bitigchi (Emir Sari)
 *	humdinger (Joachim Seemer)
 *	dospuntos (Johan Wagenheim)
 *	raefaldhia
 */
#include "CategoryWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <ListItem.h>
#include <Message.h>
#include <Messenger.h>
#include <OutlineListView.h>
#include <RadioButton.h>
#include <Region.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextView.h>
#include <View.h>

#include "AutoTextControl.h"
#include "Database.h"
#include "Help.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CategoryWindow"

// clang-format off
enum {
	M_SHOW_ADD_WINDOW = 'shaw',
	M_SHOW_REMOVE_WINDOW,
	M_SHOW_EDIT_WINDOW,
	M_EDIT_CATEGORY,
	M_ADD_CATEGORY,
	M_REMOVE_CATEGORY,
	M_NAME_CHANGED,
	M_SELECT_ITEM
};
// clang-format on

class CategoryItem : public BStringItem {
public:
	CategoryItem(const BString& name);
	void DrawItem(BView* owner, BRect frame, bool complete = false);
};

class CategoryView : public BView {
public:
	CategoryView(const char* name, const int32& flags);
	void AttachedToWindow();
	void MessageReceived(BMessage* msg);

private:
	float RefreshCategoryList();

	BOutlineListView* fListView;

	BButton *fAddButton, *fRemoveButton, *fEditButton;

	CategoryItem *fIncomeItem, *fExpensesItem;
	float fBestWidth;
};

class CategoryInputWindow : public BWindow {
public:
	CategoryInputWindow(BView* target);
	void MessageReceived(BMessage* msg);

private:
	AutoTextControl* fNameBox;

	BButton* fOKButton;

	BRadioButton* fExpenses;
	BRadioButton* fIncome;
	BView* fTarget;
};

class CategoryEditWindow : public BWindow {
public:
	CategoryEditWindow(const char* oldname, BView* target);
	void MessageReceived(BMessage* msg);

private:
	AutoTextControl* fNameBox;

	BButton* fOKButton;
	BString fOldName;
	BView* fTarget;
};

class CategoryRemoveWindow : public BWindow {
public:
	CategoryRemoveWindow(const char* from, BView* target);
	void MessageReceived(BMessage* msg);
	void FrameResized(float w, float h);

private:
	BOutlineListView* fListView;

	CategoryItem *fIncomeItem, *fExpensesItem;

	BButton* fOKButton;
	BView* fTarget;
};


CategoryView::CategoryView(const char* name, const int32& flags)
	: BView(name, flags)
{
	// the buttons
	HelpButton* helpButton = new HelpButton("menus.html", "#categories");

	fEditButton = new BButton(
		"editbutton", B_TRANSLATE("Edit" B_UTF8_ELLIPSIS), new BMessage(M_SHOW_EDIT_WINDOW));
	fRemoveButton = new BButton(
		"removebutton", B_TRANSLATE("Remove" B_UTF8_ELLIPSIS), new BMessage(M_SHOW_REMOVE_WINDOW));

	fAddButton = new BButton(
		"addbutton", B_TRANSLATE("Add" B_UTF8_ELLIPSIS), new BMessage(M_SHOW_ADD_WINDOW));

	fEditButton->SetEnabled(false);
	fRemoveButton->SetEnabled(false);

	// the category list
	fListView = new BOutlineListView("categorylist", B_SINGLE_SELECTION_LIST,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE);
	BScrollView* scrollView = new BScrollView("scrollview", fListView, 0, false, true);
	fListView->SetSelectionMessage(new BMessage(M_SELECT_ITEM));

	fIncomeItem = new CategoryItem(B_TRANSLATE_CONTEXT("Income", "CommonTerms"));
	fExpensesItem = new CategoryItem(B_TRANSLATE_CONTEXT("Expenses", "CommonTerms"));

	RefreshCategoryList();

	// clang-format off
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(scrollView)
		.AddGroup(B_HORIZONTAL)
			.AddGlue(0)
			.Add(helpButton)
			.AddGlue(1)
			.Add(fEditButton)
			.Add(fRemoveButton)
			.Add(fAddButton)
			.AddGlue()
			.End()
		.End();
	// clang-format on
}


void
CategoryView::AttachedToWindow()
{
	fListView->SetTarget(this);
	fEditButton->SetTarget(this);
	fAddButton->SetTarget(this);
	fRemoveButton->SetTarget(this);

	fListView->MakeFocus(true);
}


void
CategoryView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_SELECT_ITEM:
		{
			int32 index = fListView->CurrentSelection();
			if (fListView->CurrentSelection() < 0)
				break;

			CategoryItem* item = (CategoryItem*)fListView->ItemAt(index);

			if (!item || item == fIncomeItem || item == fExpensesItem) {
				fEditButton->SetEnabled(false);
				fRemoveButton->SetEnabled(false);
			} else {
				fEditButton->SetEnabled(true);
				fRemoveButton->SetEnabled(true);
			}
			break;
		}
		case M_SHOW_ADD_WINDOW:
		{
			CategoryInputWindow* catwin = new CategoryInputWindow(this);
			catwin->CenterIn(Window()->Frame());
			catwin->Show();
			break;
		}
		case M_SHOW_REMOVE_WINDOW:
		{
			int32 index = fListView->CurrentSelection();
			CategoryItem* item = (CategoryItem*)fListView->ItemAt(index);

			if (!item || item == fIncomeItem || item == fExpensesItem)
				break;

			CategoryRemoveWindow* catwin = new CategoryRemoveWindow(item->Text(), this);
			catwin->CenterIn(Window()->Frame());
			catwin->Show();
			break;
		}
		case M_SHOW_EDIT_WINDOW:
		{
			int32 index = fListView->CurrentSelection();
			CategoryItem* item = (CategoryItem*)fListView->ItemAt(index);

			if (!item || item == fIncomeItem || item == fExpensesItem)
				break;

			CategoryEditWindow* catwin = new CategoryEditWindow(item->Text(), this);
			catwin->CenterIn(Window()->Frame());
			catwin->Show();
			break;
		}
		case M_ADD_CATEGORY:
		{
			BString name;
			bool expense;
			if (msg->FindString("name", &name) != B_OK
				|| msg->FindBool("expense", &expense) != B_OK)
				break;

			if (IsInternalCategory(name.String())) {
				ShowAlert(B_TRANSLATE("Can't use this category name"),
					B_TRANSLATE(
						"CapitalBe uses 'Income', 'Expenses', 'Opening balance', 'Split', "
						"'Transfer', and 'Uncategorized' for managing accounts, so you can't use "
						"them as category names.\n\n"
						"Please choose a different name for your new category."));
				break;
			}

			gDatabase.AddCategory(name.String(), expense);
			RefreshCategoryList();
			break;
		}
		case M_REMOVE_CATEGORY:
		{
			BString newcat;
			if (msg->FindString("newcat", &newcat) != B_OK)
				break;

			int32 index = fListView->CurrentSelection();
			CategoryItem* item = (CategoryItem*)fListView->ItemAt(index);

			if (!item || item == fIncomeItem || item == fExpensesItem)
				break;

			gDatabase.RecategorizeTransactions(item->Text(), newcat.String());
			gDatabase.RemoveCategory(item->Text());
			RefreshCategoryList();

			break;
		}
		case M_EDIT_CATEGORY:
		{
			BString newname;
			if (msg->FindString("newname", &newname) != B_OK)
				break;

			int32 index = fListView->CurrentSelection();
			CategoryItem* item = (CategoryItem*)fListView->ItemAt(index);

			if (!item || item == fIncomeItem || item == fExpensesItem)
				break;

			gDatabase.RecategorizeTransactions(item->Text(), newname.String());
			gDatabase.RemoveCategory(item->Text());
			RefreshCategoryList();
			break;
		}
		default:
		{
			BView::MessageReceived(msg);
			break;
		}
	}
}


float
CategoryView::RefreshCategoryList()
{
	if (fListView->CountItems() > 0) {
		if (fListView->HasItem(fIncomeItem))
			fListView->RemoveItem(fIncomeItem);
		if (fListView->HasItem(fExpensesItem))
			fListView->RemoveItem(fExpensesItem);

		fListView->MakeEmpty();
	}
	fListView->AddItem(fIncomeItem);
	fListView->AddItem(fExpensesItem);

	int32 maxchars;
	float maxlength;
	if (strlen(B_TRANSLATE_CONTEXT("Income", "CommonTerms"))
		> strlen(B_TRANSLATE_CONTEXT("Expenses", "CommonTerms"))) {
		maxchars = strlen(B_TRANSLATE_CONTEXT("Income", "CommonTerms"));
		maxlength = StringWidth(B_TRANSLATE_CONTEXT("Income", "CommonTerms"));
	} else {
		maxchars = strlen(B_TRANSLATE_CONTEXT("Expenses", "CommonTerms"));
		maxlength = StringWidth(B_TRANSLATE_CONTEXT("Expenses", "CommonTerms"));
	}

	CppSQLite3Query query = gDatabase.DBQuery(
		"SELECT * FROM categorylist ORDER BY name DESC", "CategoryView::CategoryView");
	while (!query.eof()) {
		BString name = query.getStringField(0);
		if (IsInternalCategory(name.String())) {
			query.nextRow();
			continue;
		}

		int expense = query.getIntField(1);
		if (expense == EXPENSES)
			fListView->AddUnder(new CategoryItem(name.String()), fExpensesItem);
		else if (expense == INCOME)
			fListView->AddUnder(new CategoryItem(name.String()), fIncomeItem);

		if (name.CountChars() > maxchars) {
			maxchars = name.CountChars();
			maxlength = StringWidth(name.String());
		}

		query.nextRow();
	}

	return maxlength;
}


CategoryWindow::CategoryWindow(const BRect& frame)
	: BWindow(frame, B_TRANSLATE("Categories"), B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE)
{
	CategoryView* view = new CategoryView("categoryview", B_WILL_DRAW);

	AddShortcut('A', B_COMMAND_KEY, new BMessage(M_SHOW_ADD_WINDOW), view);
	AddShortcut('R', B_COMMAND_KEY, new BMessage(M_REMOVE_CATEGORY), view);

	BLayoutBuilder::Group<>(this, B_VERTICAL).SetInsets(0).Add(view).End();
}


CategoryItem::CategoryItem(const BString& name)
	: BStringItem(name.String())
{
}


void
CategoryItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	owner->SetHighUIColor(
		IsSelected() ? B_LIST_SELECTED_BACKGROUND_COLOR : B_LIST_BACKGROUND_COLOR);
	owner->FillRect(frame);

	if (IsSelected()) {
		owner->SetHighUIColor(B_CONTROL_HIGHLIGHT_COLOR);
		owner->StrokeRect(frame);
	}

	owner->SetHighUIColor(IsSelected() ? B_LIST_SELECTED_ITEM_TEXT_COLOR : B_LIST_ITEM_TEXT_COLOR);

	// Compute vertical alignment factor
	font_height fh;
	owner->GetFontHeight(&fh);
	float fontFactor = ceilf(fh.ascent + fh.descent + fh.leading) / 4 + 1;

	BRegion region(frame);
	owner->ConstrainClippingRegion(&region);
	owner->DrawString(Text(), BPoint(frame.left + 3, frame.bottom - fontFactor));
	owner->ConstrainClippingRegion(NULL);
}


CategoryInputWindow::CategoryInputWindow(BView* target)
	: BWindow(BRect(), B_TRANSLATE("Add category"), B_FLOATING_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_V_RESIZABLE
			| B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	  fTarget(target)
{
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	fNameBox
		= new AutoTextControl("namebox", B_TRANSLATE("Name:"), "", new BMessage(M_NAME_CHANGED));
	fNameBox->SetCharacterLimit(32);

	fExpenses = new BRadioButton("expensesoption", B_TRANSLATE("Expenses category"), NULL);
	fIncome = new BRadioButton("incomeoption", B_TRANSLATE("Income category"), NULL);

	fExpenses->SetValue(B_CONTROL_ON);

	fOKButton = new BButton("okbutton", B_TRANSLATE("OK"), new BMessage(M_ADD_CATEGORY));
	fOKButton->MakeDefault(true);
	fOKButton->SetEnabled(false);

	BButton* cancelButton
		= new BButton("cancelbutton", B_TRANSLATE("Cancel"), new BMessage(B_QUIT_REQUESTED));

	// clang-format off
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGrid(0.f, 0.f)
			.Add(fNameBox->CreateLabelLayoutItem(), 0, 0)
			.Add(fNameBox->CreateTextViewLayoutItem(), 1, 0)
			.Add(BSpaceLayoutItem::CreateVerticalStrut(B_USE_DEFAULT_SPACING), 0, 1)
			.Add(fExpenses, 1, 2)
			.Add(fIncome, 1, 3)
			.End()
		.AddStrut(B_USE_BIG_SPACING)
		.AddGroup(B_HORIZONTAL)
			.Add(cancelButton)
			.Add(fOKButton)
			.End()
		.End();
	// clang-format on

	fNameBox->MakeFocus(true);
}


void
CategoryInputWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_NAME_CHANGED:
		{
			if (strlen(fNameBox->Text()) > 0) {
				if (!fOKButton->IsEnabled())
					fOKButton->SetEnabled(true);
			} else {
				if (fOKButton->IsEnabled())
					fOKButton->SetEnabled(false);
			}
			break;
		}
		case M_ADD_CATEGORY:
		{
			BMessenger msgr(fTarget);
			msg->AddString("name", fNameBox->Text());
			msg->AddBool("expense", fExpenses->Value() == B_CONTROL_ON);
			msgr.SendMessage(msg);
			Quit();
			break;
		}
		default:
		{
			BWindow::MessageReceived(msg);
			break;
		}
	}
}


CategoryRemoveWindow::CategoryRemoveWindow(const char* from, BView* target)
	: BWindow(BRect(0, 0, 440, 380), B_TRANSLATE("Remove category"), B_FLOATING_WINDOW_LOOK,
		B_MODAL_APP_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_AUTO_UPDATE_SIZE_LIMITS
			| B_CLOSE_ON_ESCAPE),
	  fTarget(target)
{
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	BTextView* directions = new BTextView("directions");
	BString text(
		B_TRANSLATE("Please choose a new category for all transactions currently in the "
					"'%%CATEGORY_NAME%%' category."));
	text.ReplaceFirst("%%CATEGORY_NAME%%", from);
	directions->SetText(text.String());
	directions->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	directions->SetWordWrap(true);
	directions->MakeEditable(false);
	directions->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fOKButton = new BButton("okbutton", B_TRANSLATE("OK"), new BMessage(M_REMOVE_CATEGORY));
	fOKButton->SetEnabled(false);

	BButton* cancelButton
		= new BButton("cancelbutton", B_TRANSLATE("Cancel"), new BMessage(B_QUIT_REQUESTED));

	fListView = new BOutlineListView("categorylist", B_SINGLE_SELECTION_LIST,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE);
	BScrollView* scrollView = new BScrollView("scrollview", fListView, 0, false, true);
	fListView->SetSelectionMessage(new BMessage(M_SELECT_ITEM));

	fIncomeItem = new CategoryItem(B_TRANSLATE_CONTEXT("Income", "CommonTerms"));
	fExpensesItem = new CategoryItem(B_TRANSLATE_CONTEXT("Expenses", "CommonTerms"));
	fListView->AddItem(fIncomeItem);
	fListView->AddItem(fExpensesItem);

	CppSQLite3Query query = gDatabase.DBQuery(
		"SELECT * FROM categorylist ORDER BY name DESC", "CategoryView::CategoryView");

	while (!query.eof()) {
		int expense = query.getIntField(1);
		BString name = query.getStringField(0);

		if (name.Compare(from) == 0) {
			query.nextRow();
			continue;
		}

		if (expense == EXPENSES)
			fListView->AddUnder(new CategoryItem(name.String()), fExpensesItem);
		else if (expense == INCOME)
			fListView->AddUnder(new CategoryItem(name.String()), fIncomeItem);

		query.nextRow();
	}

	// clang-format off
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(directions, 0)
		.Add(scrollView, 5.0)
		// .AddStrut(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.AddGlue()
			.Add(cancelButton)
			.Add(fOKButton)
			.End()
		.End();
	// clang-format on
}


void
CategoryRemoveWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_SELECT_ITEM:
		{
			int32 index = fListView->CurrentSelection();
			if (fListView->CurrentSelection() < 0)
				break;

			CategoryItem* item = (CategoryItem*)fListView->ItemAt(index);

			if (!item || item == fIncomeItem || item == fExpensesItem)
				fOKButton->SetEnabled(false);
			else
				fOKButton->SetEnabled(true);
			break;
		}
		case M_REMOVE_CATEGORY:
		{
			int32 index = fListView->CurrentSelection();
			CategoryItem* item = (CategoryItem*)fListView->ItemAt(index);

			if (!item || item == fIncomeItem || item == fExpensesItem)
				break;

			BMessenger msgr(fTarget);
			msg->AddString("newcat", item->Text());
			msgr.SendMessage(msg);

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


void
CategoryRemoveWindow::FrameResized(float w, float h)
{
}


CategoryEditWindow::CategoryEditWindow(const char* oldname, BView* target)
	: BWindow(BRect(), B_TRANSLATE("Edit category"), B_FLOATING_WINDOW_LOOK,
		B_MODAL_APP_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE
			| B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	  fOldName(oldname),
	  fTarget(target)
{
	BString temp;
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	BStringView* oldLabel = new BStringView("oldlabel", B_TRANSLATE("Name:"));
	BStringView* oldName = new BStringView("oldname", fOldName.String());
	oldName->SetExplicitMinSize(
		BSize(be_plain_font->StringWidth("aQuiteLongCategoryName"), B_SIZE_UNSET));

	fNameBox = new AutoTextControl(
		"namebox", B_TRANSLATE("New name:"), "", new BMessage(M_NAME_CHANGED));
	fNameBox->SetCharacterLimit(32);

	fOKButton = new BButton("okbutton", B_TRANSLATE("OK"), new BMessage(M_EDIT_CATEGORY));
	fOKButton->SetFlags(fOKButton->Flags() | B_FRAME_EVENTS);
	fOKButton->MakeDefault(true);

	BButton* cancelButton
		= new BButton("cancelbutton", B_TRANSLATE("Cancel"), new BMessage(B_QUIT_REQUESTED));

	fOKButton->SetEnabled(false);
	fNameBox->MakeFocus(true);

	// clang-format off
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGrid(1.0f, B_USE_SMALL_SPACING)
			.Add(oldLabel, 0, 0)
			.Add(oldName, 1, 0)
			.Add(fNameBox->CreateLabelLayoutItem(), 0, 1)
			.Add(fNameBox->CreateTextViewLayoutItem(), 1, 1)
			.End()
		.AddStrut(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(cancelButton)
			.Add(fOKButton)
			.End()
		.End();
	// clang-format on

	fNameBox->MakeFocus(true);
}


void
CategoryEditWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_NAME_CHANGED:
		{
			if (strlen(fNameBox->Text()) > 0) {
				if (!fOKButton->IsEnabled())
					fOKButton->SetEnabled(true);
			} else {
				if (fOKButton->IsEnabled())
					fOKButton->SetEnabled(false);
			}
			break;
		}
		case M_EDIT_CATEGORY:
		{
			BMessenger msgr(fTarget);
			msg->AddString("newname", fNameBox->Text());
			msgr.SendMessage(msg);
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
