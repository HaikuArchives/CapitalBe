#include <Catalog.h>
#include <ControlLook.h>
#include <GridLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <Messenger.h>
#include <String.h>
#include <StringView.h>
#include <Window.h>
#include <stdlib.h>

#include "FilterView.h"
#include "MainWindow.h"
#include "RegisterView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FilterView"


FilterView::FilterView(const char* name, int32 flags)
	:
	BView(name, flags | B_FRAME_EVENTS),
	fMessenger(NULL)
{
	fPeriodMenu = new BMenu("timeperiod");
	// Important: keep the order according to filter_period_field in RegisterView.h
	fPeriodMenu->AddItem(
		new BMenuItem(B_TRANSLATE("All transactions"), new BMessage(M_FILTER_CHANGED)));
	fPeriodMenu->AddItem(new BMenuItem(B_TRANSLATE("This month"), new BMessage(M_FILTER_CHANGED)));
	fPeriodMenu->AddItem(new BMenuItem(B_TRANSLATE("Last month"), new BMessage(M_FILTER_CHANGED)));
	fPeriodMenu->AddItem(
		new BMenuItem(B_TRANSLATE("This quarter"), new BMessage(M_FILTER_CHANGED)));
	fPeriodMenu->AddItem(
		new BMenuItem(B_TRANSLATE("Last quarter"), new BMessage(M_FILTER_CHANGED)));
	fPeriodMenu->AddItem(new BMenuItem(B_TRANSLATE("This year"), new BMessage(M_FILTER_CHANGED)));
	fPeriodMenu->AddItem(new BMenuItem(B_TRANSLATE("Last year"), new BMessage(M_FILTER_CHANGED)));

	fPeriodMenu->SetLabelFromMarked(true);
	fPeriodMenu->SetRadioMode(true);
	fPeriodMenu->ItemAt(0L)->SetMarked(true);

	BMenuField* periodField = new BMenuField("periodfield", B_TRANSLATE("Period:"), fPeriodMenu);

	float maxwidth;
	for (int32 i = 0; i < fPeriodMenu->CountItems(); i++) {
		BMenuItem* item = fPeriodMenu->ItemAt(i);
		float labelwidth = StringWidth(item->Label());
		maxwidth = MAX(labelwidth, maxwidth);
	}
	maxwidth += StringWidth("XXXXXXX");

	BString label(B_TRANSLATE_CONTEXT("Payee", "CommonTerms"));
	label << ":";
	fPayee = new BTextControl("payee", label, NULL, new BMessage(M_FILTER_CHANGED));

	label = B_TRANSLATE_CONTEXT("Category", "CommonTerms");
	label << ":";
	fCategory = new BTextControl("category", label, NULL, new BMessage(M_FILTER_CHANGED));

	label = B_TRANSLATE_CONTEXT("Memo", "CommonTerms");
	label << ":";
	fMemo = new BTextControl("memo", label, NULL, new BMessage(M_FILTER_CHANGED));

	fCompareMenu = new BMenu("");
	fCompareMenu->AddItem(new BMenuItem("≦", new BMessage(M_FILTER_CHANGED)));
	fCompareMenu->AddItem(new BMenuItem("＝", new BMessage(M_FILTER_CHANGED)));
	fCompareMenu->AddItem(new BMenuItem("≧", new BMessage(M_FILTER_CHANGED)));

	fCompareMenu->SetLabelFromMarked(true);
	fCompareMenu->SetRadioMode(true);
	fCompareMenu->ItemAt(0L)->SetMarked(true);

	BMenuField* compareField = new BMenuField("compareField", "", fCompareMenu);
	compareField->SetExplicitSize(BSize(StringWidth("≦") * 2, B_SIZE_UNSET));

	label = B_TRANSLATE_CONTEXT("Amount", "CommonTerms");
	label << ":";
	fAmount = new NumBox("amount", label, NULL, new BMessage(M_FILTER_CHANGED));
	fAmount->UseTabFiltering(false);
	fAmount->AllowNegatives(false);

	fClear = new BButton("clearbutton", B_TRANSLATE("Clear"), new BMessage(M_CLEAR_FILTER));
	fFilter = new BButton("startbutton", B_TRANSLATE("Filter"), new BMessage(M_START_FILTER));

	// clang-format off
	BView* amountWidget = new BView("amountWidget", B_WILL_DRAW);
	BLayoutBuilder::Group<>(amountWidget, B_HORIZONTAL, -2)
		.Add(compareField)
		.Add(fAmount->CreateTextViewLayoutItem())
		.End();

	BGridLayout* filterGrid = BGridLayoutBuilder(0.0, B_USE_SMALL_SPACING)
		.Add(periodField->CreateLabelLayoutItem(), 0, 0)
		.Add(periodField->CreateMenuBarLayoutItem(), 1, 0)
		.Add(fPayee->CreateLabelLayoutItem(), 0, 1)
		.Add(fPayee->CreateTextViewLayoutItem(), 1, 1)
		.Add(fCategory->CreateLabelLayoutItem(), 0, 2)
		.Add(fCategory->CreateTextViewLayoutItem(), 1, 2)
		.Add(fMemo->CreateLabelLayoutItem(), 0, 3)
		.Add(fMemo->CreateTextViewLayoutItem(),1, 3)
		.Add(new BStringView("amountlabel", label), 0, 4)
		.Add(amountWidget, 1, 4);

	filterGrid->SetMinColumnWidth(1, maxwidth);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0)
		.Add(filterGrid)
		.AddStrut(B_USE_BIG_SPACING)
		.AddGroup(B_HORIZONTAL)
			.Add(fClear)
			.Add(fFilter)
			.End()
		.End();
	// clang-format on
}


FilterView::~FilterView()
{
}


void
FilterView::AttachedToWindow()
{
	fPeriodMenu->SetTargetForItems(this);
	fCompareMenu->SetTargetForItems(this);
	fPayee->SetTarget(this);
	fCategory->SetTarget(this);
	fMemo->SetTarget(this);
	fAmount->SetTarget(this);
	fClear->SetTarget(this);
	fFilter->SetTarget(this);
}


void
FilterView::MessageReceived(BMessage* msg)
{
	int32 start;
	BString string;
	switch (msg->what) {
		case M_SET_FILTER:
		{
			Account* acc = gDatabase.CurrentAccount();
			printf("FilterView: M_SET_FILTER\n");
			TransactionData data;
			gDatabase.GetTransaction(acc->CurrentTransaction(), acc->GetID(), data);

			BString amount;
			gCurrentLocale.CurrencyToString(data.Amount().AbsoluteValue(), amount);
			fPayee->SetText(data.Payee());
			fCategory->SetText(data.NameAt(0));
			fMemo->SetText(data.MemoAt(0));
			fCompareMenu->ItemAt(1)->SetMarked(true);
			fAmount->SetText(amount.String());

			fPayee->MakeFocus(true);
			fPayee->TextView()->SelectAll();
			SendFilterMessage();

			break;
		}
		case M_CLEAR_FILTER:
		{
			MakeEmpty();
			// intentional fall-thru
		}
		case M_START_FILTER:
		case M_FILTER_CHANGED:
		{
			SendFilterMessage();
			break;
		}
		default:
		{
			BView::MessageReceived(msg);
		}
	}
}


void
FilterView::MakeEmpty()
{
	fPeriodMenu->ItemAt(0L)->SetMarked(true);
	fPayee->SetText("");
	fCategory->SetText("");
	fMemo->SetText("");
	fCompareMenu->ItemAt(0L)->SetMarked(true);
	fAmount->SetText("");
}


bool
FilterView::IsEmpty()
{
	return fPeriodMenu->FindMarkedIndex() == 0 && strcmp(fPayee->Text(), "") == 0
		&& strcmp(fCategory->Text(), "") == 0 && strcmp(fMemo->Text(), "") == 0
		&& fCompareMenu->FindMarkedIndex() == 0 && strcmp(fAmount->Text(), "") == 0;
}


void
FilterView::SendFilterMessage()
{
	BMessage filterMsg(M_FILTER);
	filterMsg.AddString("payee", fPayee->Text());
	filterMsg.AddString("category", fCategory->Text());
	filterMsg.AddString("memo", fMemo->Text());
	filterMsg.AddString("amount", fAmount->Text());
	filterMsg.AddInt32("moreless", fCompareMenu->FindMarkedIndex());
	filterMsg.AddInt32("period", fPeriodMenu->FindMarkedIndex());

	fMessenger->SendMessage(&filterMsg);
}
