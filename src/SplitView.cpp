#include "SplitView.h"
#include <Box.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MessageFilter.h>
#include <Messenger.h>
#include <SeparatorView.h>
#include <StringView.h>
#include <Window.h>

#include "Account.h"
#include "CategoryBox.h"
#include "CheckNumBox.h"
#include "CheckView.h"
#include "CurrencyBox.h"
#include "DAlert.h"
#include "Database.h"
#include "DateBox.h"
#include "HelpButton.h"
#include "MsgDefs.h"
#include "NavTextBox.h"
#include "PayeeBox.h"
#include "SplitItem.h"
#include "SplitViewFilter.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SplitView"


SplitView::SplitView(const char* name, const TransactionData& trans, const int32& flags)
	: BView(name, flags | B_FRAME_EVENTS),
	  Observer(WATCH_SELECT | WATCH_TRANSACTION | WATCH_ACCOUNT)
{
	fTransaction = trans;

	fStartExpanded = false;
	fCheckNum = fTransaction.GetAccount()->LastCheckNumber();

	// Date
	BStringView* dateLabel = new BStringView("datelabel", B_TRANSLATE_CONTEXT("Date", "CommonTerms"));
	dateLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fDate = new DateBox("dateentry", "", "text", new BMessage(M_DATE_CHANGED));
	fDate->SetDate(trans.Date());
	BString tempstr;
	gDefaultLocale.DateToString(fTransaction.Date(), tempstr);
	fDate->SetText(tempstr.String());

	// Type
	BStringView* typeLabel = new BStringView("typelabel", B_TRANSLATE_CONTEXT("Type", "CommonTerms"));
	typeLabel->SetExplicitSize(BSize(StringWidth("ShortType"), B_SIZE_UNSET));

	fType = new CheckNumBox("typeentry", "", NULL, new BMessage(M_TYPE_CHANGED));
	fType->SetText(fTransaction.Type().Type());

	// Payee
	BStringView* payeeLabel = new BStringView("payeelabel", B_TRANSLATE_CONTEXT("Payee", "CommonTerms"));
	payeeLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fPayee = new PayeeBox("payeeentry", "", fTransaction.Payee(), new BMessage(M_PAYEE_CHANGED));
	fPayee->SetExplicitMinSize(BSize(StringWidth("QuiteLongPayeesName"), B_SIZE_UNSET));

	// Amount
	BStringView* amountLabel = new BStringView("amountlabel", B_TRANSLATE_CONTEXT("Amount", "CommonTerms"));
	amountLabel->SetExplicitSize(BSize(StringWidth("$10,000,000,000.00"), B_SIZE_UNSET));

	fAmount = new CurrencyBox("amountentry", "", NULL, new BMessage(M_AMOUNT_CHANGED));
	gCurrentLocale.CurrencyToString(
		(fTransaction.Amount() < 0) ? fTransaction.Amount().InvertAsCopy() : fTransaction.Amount(),
		tempstr);
	fAmount->SetText(tempstr.String());

	// Category
	BStringView* categoryLabel = new BStringView("categorylabel", B_TRANSLATE_CONTEXT("Category", "CommonTerms"));
	categoryLabel->SetExplicitSize(BSize(StringWidth("aVeryLongCategoryName"), B_SIZE_UNSET));

	fCategory = new CategoryBox(
		"categoryentry", "", fTransaction.NameAt(0), new BMessage(M_CATEGORY_CHANGED));

	// Memo
	BStringView* memoLabel = new BStringView("memolabel", B_TRANSLATE_CONTEXT("Memo", "CommonTerms"));
	memoLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fMemo = new NavTextBox("memoentry", "", fTransaction.Memo(), new BMessage(M_MEMO_CHANGED));

	// Controls
	fSplit = new BCheckBox(
		"expander", B_TRANSLATE("Show split"), new BMessage(M_EXPANDER_CHANGED), B_WILL_DRAW);
	fSplit->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	HelpButton* helpButton =
		new HelpButton(B_TRANSLATE("Help: Transaction editor"), "Transaction Editor.txt");

	// SplitContainer
	fSplitContainer = new BView("splitcontainer", B_WILL_DRAW);
	fSplitContainer->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fSplitContainer->Hide();

	fAddSplit = new BButton("addsplit", B_TRANSLATE("Add item"), new BMessage(M_ADD_SPLIT));
	fRemoveSplit =
		new BButton("removesplit", B_TRANSLATE("Remove item"), new BMessage(M_REMOVE_SPLIT));

	BString totalLabel(B_TRANSLATE("Total:"));
	totalLabel << " " << fTransaction.Amount().AbsoluteValue().AsFloat();
	fSplitTotal = new BStringView("splittotal", totalLabel.String());
	fSplitTotal->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fEnter = new BButton("enterbutton", B_TRANSLATE("Enter"), new BMessage(M_ENTER_TRANSACTION));

#ifndef ENTER_NAVIGATION
	fEnter->MakeDefault(true);
#endif

	// Split category
	BStringView* splitCategoryLabel = new BStringView("categorylabel", B_TRANSLATE_CONTEXT("Category", "CommonTerms"));
	splitCategoryLabel->SetExplicitSize(BSize(StringWidth("aVeryLongCategoryName"), B_SIZE_UNSET));
	fSplitCategory = new BTextControl("splitcategory", NULL, NULL, NULL, B_WILL_DRAW);

	// Split amount
	BStringView* splitAmountLabel = new BStringView("amountlabel", B_TRANSLATE_CONTEXT("Amount", "CommonTerms"));
	splitAmountLabel->SetExplicitSize(BSize(StringWidth("$10,000,000,000.00"), B_SIZE_UNSET));
	fSplitAmount = new BTextControl("splitamount", NULL, NULL, NULL, B_WILL_DRAW);

	// Split memo
	BStringView* splitMemoLabel = new BStringView("memolabel", B_TRANSLATE_CONTEXT("Memo", "CommonTerms"));
	splitMemoLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fSplitMemo = new BTextControl("splitmemo", NULL, NULL, NULL, B_WILL_DRAW);

	// Split list
	fSplitItems = new BListView("splititems");
	fSplitItems->SetSelectionMessage(new BMessage(M_SELECT_SPLIT));
	fSplitScroller = new BScrollView("split scroller", fSplitItems, 0, false, true);
	fSplitScroller->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fKeyFilter = new SplitViewFilter(this);

	fCurrentDate = fTransaction.Date();

	Locale locale = fTransaction.GetAccount()->GetLocale();

	int32 count = fTransaction.CountCategories();
	for (int32 i = 0; i < count; i++) {
		SplitItem* item = new SplitItem();
		item->SetCategory(fTransaction.NameAt(i));
		item->SetAmount(fTransaction.AmountAt(i));
		item->SetMemo(fTransaction.MemoAt(i));

		fSplitItems->AddItem(item);
	}

	if (fTransaction.CountCategories() > 1 ||
		strcmp(fTransaction.NameAt(0), B_TRANSLATE_CONTEXT("Split", "CommonTerms")) == 0) {
		fCategory->SetText(B_TRANSLATE("Split transaction"));
		fStartExpanded = true;
	}

	// clang-format off
	BLayoutBuilder::Group<>(fSplitContainer, B_VERTICAL, 0)
		.SetInsets(0)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
		.AddStrut(B_USE_SMALL_SPACING)
		.AddGrid(1.0f, 0.0f)
			.Add(fAddSplit, 0, 0)
			.Add(fRemoveSplit, 1, 0)
			.AddGlue(2, 0)
			.Add(fSplitTotal, 3, 0)
			.AddGlue(4, 0)
			.End()
		.AddStrut(B_USE_SMALL_SPACING)
		.AddGrid(1.0f, 0.0f)
			.SetColumnWeight(2, 2.0f)
			.Add(splitCategoryLabel, 0, 0)
			.Add(fSplitCategory, 0, 1)
			.Add(splitAmountLabel, 1, 0)
			.Add(fSplitAmount, 1, 1)
			.Add(splitMemoLabel, 2, 0)
			.Add(fSplitMemo, 2, 1)
			.End()
		.Add(fSplitScroller)
		.End();

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_WINDOW_SPACING)
		.AddGrid(1.0f, 0.0f)
			// .SetColumnWeight(1, 0.5f)
			.SetColumnWeight(2, 2.0f)
			.SetColumnWeight(3, 1.0f)
			.Add(dateLabel, 0, 0)
			.Add(fDate, 0, 1)
			.Add(typeLabel, 1, 0)
			.Add(fType, 1, 1)
			.Add(payeeLabel, 2, 0)
			.Add(fPayee, 2, 1)
			.Add(amountLabel, 3, 0)
			.Add(fAmount, 3, 1)
			.Add(categoryLabel, 0, 2)
			.Add(fCategory, 0, 3)
			.Add(memoLabel, 1, 2, 3)
			.Add(fMemo, 1, 3, 3)
			.End()
		.AddGroup(B_HORIZONTAL)
			.Add(fSplit)
			.AddGlue()
			.Add(helpButton)
			.Add(fEnter)
			.End()
		.AddGroup(B_VERTICAL, 0.0f)
			.Add(fSplitContainer)
			.End()
		.End();
}
// clang-format on

SplitView::~SplitView(void)
{
	delete fMessenger;
}

void
SplitView::AttachedToWindow(void)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	Window()->AddCommonFilter(fKeyFilter);
	fMessenger = new BMessenger(this);
	fDate->GetFilter()->SetMessenger(new BMessenger(this));
	fType->GetFilter()->SetMessenger(new BMessenger(this));
	fPayee->GetFilter()->SetMessenger(new BMessenger(this));
	fAmount->GetFilter()->SetMessenger(new BMessenger(this));
	fCategory->GetFilter()->SetMessenger(new BMessenger(this));
	fMemo->GetFilter()->SetMessenger(new BMessenger(this));

	fEnter->SetTarget(this);
	fDate->MakeFocus(true);
	fSplit->SetTarget(this);
	fAddSplit->SetTarget(this);
	fRemoveSplit->SetTarget(this);
	fSplitItems->SetTarget(this);

	if (fStartExpanded) {
		//	fSplit->Invoke();
		ToggleSplit();
		fSplitCategory->MakeFocus(true);
	}

	if (fSplitItems->CountItems() > 0)
		fSplitItems->Select(0L);
}

void
SplitView::DetachedFromWindow(void)
{
	Window()->RemoveCommonFilter(fKeyFilter);
}

void
SplitView::MessageReceived(BMessage* msg)
{
	int32 start;
	BString string;
	switch (msg->what) {
		case M_EXPANDER_CHANGED:
		{
			ToggleSplit();
			break;
		}
		case M_TYPE_AUTOCOMPLETE:
		{
			msg->FindInt32("start", &start);
			msg->FindString("string", &string);
			fType->SetText(string.String());
			fType->TextView()->Select(start, string.CountChars());
			break;
		}
		case M_PAYEE_AUTOCOMPLETE:
		{
			msg->FindInt32("start", &start);
			msg->FindString("string", &string);
			fPayee->SetText(string.String());
			fPayee->TextView()->Select(start, string.CountChars());
			break;
		}
		case M_CATEGORY_AUTOCOMPLETE:
		{
			msg->FindInt32("start", &start);
			msg->FindString("string", &string);
			if (fSplitContainer->IsHidden()) {
				fCategory->SetText(string.String());
				fCategory->TextView()->Select(start, string.CountChars());
			} else {
				fSplitCategory->SetText(string.String());
				fSplitCategory->TextView()->Select(start, string.CountChars());
			}
			break;
		}
		case M_ENTER_TRANSACTION:
		{
			// The text filter sends this message whenever the user hits Enter
			// from the Memo field. The SplitView instance should do whatever is
			// needed to post the transaction into the register
			if (!fDate->Validate())
				break;

			if (!fType->Validate())
				break;

			if (!fPayee->Validate())
				break;

			if (!fAmount->Validate())
				break;

			if (!fCategory->Validate())
				break;

			if (!ValidateSplitItems())
				break;
			Account* account = fTransaction.GetAccount();
			if (!account)
				ShowBug("NULL transaction account in SplitView::M_ENTER_TRANSACTION");

			Category* cat = MakeCategory();
			fTransaction.Set(account, fDate->Text(), fType->Text(), fPayee->Text(), fAmount->Text(),
				NULL, fMemo->Text(), fTransaction.Status());
			fTransaction.SetCategory(*cat);
			delete cat;

			try {
				gDatabase.RemoveTransaction(fTransaction.GetID());

				// This adds the transaction data without generating a new transaction id
				gDatabase.AddTransaction(fTransaction, false);
			} catch (CppSQLite3Exception& e) {
				debugger(e.errorMessage());
			}
			Window()->PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case M_PREVIOUS_FIELD:
		{
			if (fDate->ChildAt(0)->IsFocus()) {
				if (fDate->Validate())
					fEnter->MakeFocus(true);
			} else if (fType->ChildAt(0)->IsFocus())
				fDate->MakeFocus(true);
			else if (fPayee->ChildAt(0)->IsFocus()) {
				if (fPayee->Validate())
					fType->MakeFocus(true);
			} else if (fAmount->ChildAt(0)->IsFocus()) {
				if (fAmount->Validate())
					fPayee->MakeFocus(true);
			} else if (fCategory->ChildAt(0)->IsFocus()) {
				if (fCategory->Validate())
					fAmount->MakeFocus(true);
			} else if (fMemo->ChildAt(0)->IsFocus()) {
				if (fSplitContainer->IsHidden())
					fCategory->MakeFocus(true);
				else
					fAmount->MakeFocus(true);
			} else if (fSplit->IsFocus())
				fMemo->MakeFocus(true);
			else if (fAddSplit->IsFocus())
				fSplit->MakeFocus(true);
			else if (fRemoveSplit->IsFocus())
				fAddSplit->MakeFocus(true);
			else if (fSplitCategory->ChildAt(0)->IsFocus())
				fRemoveSplit->MakeFocus(true);
			else if (fSplitAmount->ChildAt(0)->IsFocus()) {
				if (ValidateSplitAmountField())
					fSplitCategory->MakeFocus(true);
			} else if (fSplitMemo->ChildAt(0)->IsFocus())
				fSplitAmount->MakeFocus(true);
			else if (fSplitScroller->IsFocus())
				fSplitMemo->MakeFocus();
			else if (fEnter->IsFocus()) {
				if (fSplitContainer->IsHidden())
					fSplit->MakeFocus(true);
				else
					fSplitItems->MakeFocus(true);
			} else {
				// We should *never* be here
				ShowBug("M_PREVIOUS_FIELD received for unrecognized view");
			}
			break;
		}
		case M_NEXT_FIELD:
		{
			// This message is received from the text filter in order to
			// use the Enter key to change from one entry field to another
			if (fDate->ChildAt(0)->IsFocus()) {
				if (fDate->Validate())
					fType->MakeFocus(true);
			} else if (fType->ChildAt(0)->IsFocus())
				fPayee->MakeFocus(true);
			else if (fPayee->ChildAt(0)->IsFocus()) {
				if (fPayee->Validate())
					fAmount->MakeFocus(true);
			} else if (fAmount->ChildAt(0)->IsFocus()) {
				if (fAmount->Validate()) {
					if (fSplitContainer->IsHidden())
						fCategory->MakeFocus(true);
					else
						fMemo->MakeFocus(true);
				}
			} else if (fCategory->ChildAt(0)->IsFocus()) {
				if (fCategory->Validate())
					fMemo->MakeFocus(true);
			} else if (fMemo->ChildAt(0)->IsFocus())
				fSplit->MakeFocus(true);
			else if (fSplit->IsFocus()) {
				if (fSplitContainer->IsHidden())
					fEnter->MakeFocus(true);
				else
					fAddSplit->MakeFocus(true);
			} else if (fAddSplit->IsFocus())
				fRemoveSplit->MakeFocus(true);
			else if (fRemoveSplit->IsFocus())
				fSplitCategory->MakeFocus(true);
			else if (fSplitCategory->ChildAt(0)->IsFocus())
				fSplitAmount->MakeFocus(true);
			else if (fSplitAmount->ChildAt(0)->IsFocus()) {
				if (ValidateSplitAmountField())
					fSplitMemo->MakeFocus(true);
			} else if (fSplitMemo->ChildAt(0)->IsFocus())
				fSplitItems->MakeFocus(true);
			else if (fSplitItems->IsFocus())
				fEnter->MakeFocus(true);
			else if (fEnter->IsFocus())
				fDate->MakeFocus(true);
			else {
				// We should *never* be here
				ShowBug("M_NEXT_FIELD received for unrecognized view");
			}
			break;
		}
		case M_ADD_SPLIT:
		{
			if (fSplitContainer->IsHidden())
				ToggleSplit();

			SplitItem* item = new SplitItem();
			item->SetCategory(B_TRANSLATE("Uncategorized"));
			item->SetAmount(
				fTransaction.Amount().AbsoluteValue() - CalculateTotal().AbsoluteValue());
			fSplitItems->AddItem(item);
			fSplitItems->Select(fSplitItems->IndexOf(item));
			fSplitCategory->MakeFocus(true);
			fSplitCategory->TextView()->SelectAll();
			break;
		}
		case M_REMOVE_SPLIT:
		{
			if (fSplitContainer->IsHidden())
				break;

			int32 selection = fSplitItems->CurrentSelection();
			if (selection < 0)
				break;

			int32 newselection;
			if (fSplitItems->CountItems() == 1)
				newselection = -1;
			else
				newselection = (selection == 0) ? 0 : selection - 1;

			SplitItem* item = (SplitItem*)fSplitItems->ItemAt(selection);
			fSplitItems->RemoveItem(item);
			fSplitItems->Select(newselection);
			fSplitCategory->TextView()->SelectAll();
			break;
		}
		case M_SELECT_SPLIT:
		{
			int32 selection = fSplitItems->CurrentSelection();
			if (selection < 0)
				break;

			SplitItem* item = (SplitItem*)fSplitItems->ItemAt(selection);
			fSplitCategory->SetText(item->GetCategory());

			BString amount;
			gCurrentLocale.CurrencyToString(item->GetAmount().AbsoluteValue(), amount);
			fSplitAmount->SetText(amount.String());

			fSplitMemo->SetText(item->GetMemo());
			fSplitCategory->TextView()->SelectAll();
			break;
		}
		case M_EDIT_KEY:
		{
			// we receive this message when the user uses an editing key, such as
			// backspace or delete. The reason we do this is to allow the textbox
			// filter to let the editing key pass through and still save the changes
			// to disk
			int32 command;
			if (msg->FindInt32("command", &command) == B_OK)
				fMessenger->SendMessage(command);
			break;
		}
		// These shouldn't even be necessary, but I guess for performance
		// reasons, BTextControls don't send update messages with every keystroke,
		// which, unfortunately, is what we want.
		case M_SPLIT_CATEGORY_CHANGED:
		{
			int32 selection = fSplitItems->CurrentSelection();
			SplitItem* splititem = (SplitItem*)fSplitItems->ItemAt(selection);
			if (!splititem)
				break;

			splititem->SetCategory(fSplitCategory->Text());
			fSplitItems->InvalidateItem(selection);

			if (strlen(fSplitCategory->Text()) < 1)
				fTransaction.SetNameAt(selection, B_TRANSLATE("Uncategorized"));
			else
				fTransaction.SetNameAt(selection, fSplitCategory->Text());
			break;
		}
		case M_SPLIT_AMOUNT_CHANGED:
		{
			int32 selection = fSplitItems->CurrentSelection();
			SplitItem* splititem = (SplitItem*)fSplitItems->ItemAt(selection);
			if (!splititem)
				break;

			Fixed fixed;
			if (gCurrentLocale.StringToCurrency(fSplitAmount->Text(), fixed) != B_OK)
				break;

			splititem->SetAmount(fixed);
			fSplitItems->InvalidateItem(selection);

			fTransaction.SetAmountAt(selection, fixed);

			BString totalLable(B_TRANSLATE("Total: %sum%"));
			BString tempTotal;
			Fixed totalFixed(CalculateTotal().AbsoluteValue());
			gCurrentLocale.CurrencyToString(totalFixed, tempTotal);
			totalLable.ReplaceFirst("%sum%", tempTotal);
			fSplitTotal->SetText(totalLable.String());

			// Color total if the splits don't add up to the transaction amount
			float totalAmount = CalculateTotal().AbsoluteValue().AsFloat();
			rgb_color totalColor = (totalFixed == fTransaction.Amount().AbsoluteValue())
									   ? ui_color(B_PANEL_TEXT_COLOR)
									   : ui_color(B_FAILURE_COLOR);
			fSplitTotal->SetHighColor(totalColor);
			break;
		}
		case M_SPLIT_MEMO_CHANGED:
		{
			int32 selection = fSplitItems->CurrentSelection();
			SplitItem* splititem = (SplitItem*)fSplitItems->ItemAt(selection);
			if (!splititem)
				break;

			splititem->SetMemo(fSplitMemo->Text());
			fSplitItems->InvalidateItem(selection);

			fTransaction.SetMemoAt(selection, fSplitMemo->Text());
			break;
		}
		case M_NEXT_SPLIT:
		{
			if (fSplitContainer->IsHidden())
				break;

			int32 selection = fSplitItems->CurrentSelection();
			if (selection == fSplitItems->CountItems() - 1 || selection < 0)
				break;

			fSplitItems->Select(selection + 1);
			break;
		}
		case M_PREVIOUS_SPLIT:
		{
			if (fSplitContainer->IsHidden())
				break;

			int32 selection = fSplitItems->CurrentSelection();
			if (selection < 1)
				break;

			fSplitItems->Select(selection - 1);
			break;
		}
		default:
		{
			BView::MessageReceived(msg);
		}
	}
}

void
SplitView::SetFields(const char* date, const char* type, const char* payee, const char* amount,
	const char* category, const char* memo)
{
	fDate->SetText(date);
	fType->SetText(type);
	fPayee->SetText(payee);
	fAmount->SetText(amount);
	fCategory->SetText(category);
	fMemo->SetText(memo);
}

void
SplitView::HandleNotify(const uint64& value, const BMessage* msg)
{
	if (value & WATCH_SELECT) {
		if (value & WATCH_ACCOUNT)
			MakeEmpty();
		else if (value & WATCH_TRANSACTION) {
			TransactionData* trans;
			if (msg->FindPointer("item", (void**)&trans) == B_OK)
				fCurrentDate = trans->Date();
		}
	}
}

void
SplitView::MakeEmpty(void)
{
	fDate->SetText("");
	fType->SetText("");
	fPayee->SetText("");
	fAmount->SetText("");
	fCategory->SetText("");
	fMemo->SetText("");
}

void
SplitView::MakeFocus(bool value)
{
	fDate->MakeFocus(value);
}

void
SplitView::FrameResized(float width, float height)
{
}

bool
SplitView::ValidateSplitAmountField(void)
{
	if (strlen(fSplitAmount->Text()) < 1)
		fSplitAmount->SetText("0");

	Fixed amount;
	if (gCurrentLocale.StringToCurrency(fSplitAmount->Text(), amount) != B_OK) {
		ShowAlert(B_TRANSLATE_CONTEXT("CapitalBe didn't understand the amount", "TextInput"),
			B_TRANSLATE_CONTEXT("There may be a typo or the wrong kind of currency symbol "
								"for this account.",
				"TextInput"));
		fSplitAmount->MakeFocus(true);
		return false;
	} else {
		BString string;
		gCurrentLocale.CurrencyToString(amount, string);
		fSplitAmount->SetText(string.String());
	}

	return true;
}

bool
SplitView::ValidateSplitItems(void)
{
	Fixed total;
	for (int32 i = 0; i < fSplitItems->CountItems(); i++) {
		SplitItem* item = (SplitItem*)fSplitItems->ItemAt(i);
		total += item->GetAmount().AbsoluteValue();
	}

	Fixed amount;
	if (gCurrentLocale.StringToCurrency(fAmount->Text(), amount) != B_OK) {
		fAmount->MakeFocus(true);
		ShowAlert(B_TRANSLATE_CONTEXT("CapitalBe didn't understand the amount", "TextInput"),
			B_TRANSLATE_CONTEXT("There may be a typo or the wrong kind of currency symbol "
								"for this account.",
				"TextInput"));

		return false;
	}

	if (total != amount.AbsoluteValue()) {
		BString errormsg, totalstr;
		gCurrentLocale.CurrencyToString(total, totalstr);

		errormsg = B_TRANSLATE(
			"When the split items are added together, they need to add up "
			"to %%ENTERED_AMOUNT%%. Currently, they add up to %%TOTAL_AMOUNT%%");
		errormsg.ReplaceFirst("%%ENTERED_AMOUNT%%", fAmount->Text());
		errormsg.ReplaceFirst("%%TOTAL_AMOUNT%%", totalstr.String());

		ShowAlert(B_TRANSLATE("The split total must match the amount box"), errormsg.String());
		fSplitAmount->MakeFocus(true);
		return false;
	}
	return true;
}

bool
SplitView::ValidateAllFields(void)
{
	BString date;
	if (strlen(fDate->Text()) < 1) {
		gDefaultLocale.DateToString(fCurrentDate, date);
		fDate->SetText(date.String());
	} else if (!fDate->Validate())
		return false;

	if (!fPayee->Validate())
		return false;

	if (!fCategory->Validate())
		return false;

	if (!fAmount->Validate())
		return false;

	if (!(ValidateSplitItems()))
		return false;

	return true;
}

void
SplitView::ToggleSplit(void)
{
	if (fSplitContainer->IsHidden()) {
		fSplit->SetValue(B_CONTROL_ON);

		fSplitContainer->Show();
		fCategory->SetEnabled(false);
		if (fCategory->ChildAt(0)->IsFocus())
			fMemo->MakeFocus();
	} else {
		fSplit->SetValue(B_CONTROL_OFF);

		fSplitContainer->Hide();
		fCategory->SetEnabled(true);
	}
}

Category*
SplitView::MakeCategory(void)
{
	// This makes a category object from the existing data
	Category* cat = new Category();
	Locale locale = fTransaction.GetAccount()->GetLocale();
	if (fSplitContainer->IsHidden() && fSplitItems->CountItems() <= 1) {
		if (strlen(fCategory->Text()) > 0 && strlen(fAmount->Text()) > 0) {
			Fixed amt;
			if (locale.StringToCurrency(fAmount->Text(), amt) == B_OK) {
				BString typestr(fType->Text());
				if (typestr.CountChars() < 1 || (typestr.ICompare("dep") != 0 && amt.IsPositive()))
					amt.Invert();

				cat->AddItem(fCategory->Text(), amt);
			}
		}
		return cat;
	}

	for (int32 i = 0; i < fSplitItems->CountItems(); i++) {
		SplitItem* item = (SplitItem*)fSplitItems->ItemAt(i);
		if (!item || strlen(item->GetCategory()) < 1 || item->GetAmount().IsZero())
			continue;

		Fixed fixed = item->GetAmount();

		BString typestr(fType->Text());
		if (typestr.CountChars() < 1 || (typestr.ICompare("dep") != 0 && fixed.IsPositive()))
			fixed.Invert();

		cat->AddItem(item->GetCategory(), fixed);
	}

	return cat;
}

Fixed
SplitView::CalculateTotal(void)
{
	Fixed total;

	for (int32 i = 0; i < fSplitItems->CountItems(); i++) {
		SplitItem* item = (SplitItem*)fSplitItems->ItemAt(i);
		total += item->GetAmount().AbsoluteValue();
	}

	return total;
}
