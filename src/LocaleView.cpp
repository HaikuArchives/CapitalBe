/*
 * Contains code from Haiku LocaleWindow.h
 * Copyright 2005-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2024, Johan Wagenheim <johan@dospuntos.no>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "LocaleView.h"
#include <Catalog.h>
#include <FormattingConventions.h>
#include <LayoutBuilder.h>
#include <NumberFormat.h>
#include <ScrollView.h>
#include <Locale.h>
#include <cstdio>
#include <Language.h>
#include "CBLocale.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PrefWindow"

enum {
	M_NEW_CURRENCY_LOCALE
};

static int
compare_typed_list_items(const BListItem* _a, const BListItem* _b)
{
	static BCollator collator;

	LanguageListItem* a = (LanguageListItem*)_a;
	LanguageListItem* b = (LanguageListItem*)_b;

	return collator.Compare(a->Text(), b->Text());
}

LocaleView::LocaleView(const char* name, const int32& flags)
	: BView(name, flags)
{
	BFormattingConventions initialConventions;
	BLocale::Default()->GetFormattingConventions(&initialConventions);

	fLocaleBox = new BBox("LocaleBox");
	UpdateCurrencyLabel(initialConventions);

	fConventionsListView = new LanguageListView("formatting",
		B_SINGLE_SELECTION_LIST);
	BScrollView* scrollView = new BScrollView("scroller", fConventionsListView,
		B_WILL_DRAW | B_FRAME_EVENTS, true, true);
	fConventionsListView->SetSelectionMessage(
		new BMessage(M_NEW_CURRENCY_LOCALE));

	// get all available formatting conventions (by language)
	BMessage availableLanguages;
	BString conventionsID;
	fInitialConventionsItem = NULL;
	LanguageListItem* currentToplevelItem = NULL;
	if (BLocaleRoster::Default()->GetAvailableLanguages(&availableLanguages)
			== B_OK) {
		for (int i = 0;
			availableLanguages.FindString("language", i, &conventionsID) == B_OK;
			i++) {
			BFormattingConventions conventions(conventionsID);
			BString conventionsName;
			conventions.GetName(conventionsName);

			LanguageListItem* item;
			if (conventions.AreCountrySpecific()) {
				item = new LanguageListItemWithFlag(conventionsName, conventionsID,
					conventions.LanguageCode(), conventions.CountryCode());
			} else {
				item = new LanguageListItem(conventionsName, conventionsID,
					conventions.LanguageCode());
			}
			if (!strcmp(conventionsID, "en_US"))
				fDefaultConventionsItem = item;
			if (conventions.AreCountrySpecific()
				&& currentToplevelItem != NULL
				&& currentToplevelItem->Code() == item->Code()) {
				if (!strcmp(conventionsID, initialConventions.ID())) {
					fConventionsListView->Expand(currentToplevelItem);
					fInitialConventionsItem = item;
				}
				fConventionsListView->AddUnder(item, currentToplevelItem);
			} else {
				// This conventions-item isn't country-specific, add it at top-level
				fConventionsListView->AddItem(item);
				item->SetExpanded(false);
				currentToplevelItem = item;
				if (!strcmp(conventionsID, initialConventions.ID()))
					fInitialConventionsItem = item;
			}
		}
	} else {
		ShowAlert(B_TRANSLATE("No available languages"),
			B_TRANSLATE("CapitalBe couldn't find any available languages. "
				"You will not be able to set custom currencies."));
	}

	fConventionsListView->FullListSortItems(compare_typed_list_items);
	if (fInitialConventionsItem != NULL) {
		fConventionsListView->Select(fConventionsListView->IndexOf(
			fInitialConventionsItem));
	}
	int size = 21 * be_plain_font->Size();
	fConventionsListView->SetExplicitMinSize(BSize(size, size));

	BLayoutBuilder::Group<>(fLocaleBox, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(
			B_USE_DEFAULT_SPACING, B_USE_BIG_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.AddGrid(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
			.Add(scrollView, 0, 0)
			.End()
		.End();

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fLocaleBox)
		.End();
}

void
LocaleView::AttachedToWindow(void)
{
	fConventionsListView->SetTarget(this);
}

void
LocaleView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case M_NEW_CURRENCY_LOCALE:
		{
			// Country selection changed.
			// Get the new selected country from the ListView and send it to the
			// main app event handler.
			void* listView;
			if (message->FindPointer("source", &listView) != B_OK)
				break;

			BListView* conventionsList = static_cast<BListView*>(listView);
			if (conventionsList == NULL)
				break;

			LanguageListItem* item = static_cast<LanguageListItem*>
				(conventionsList->ItemAt(conventionsList->CurrentSelection()));
			if (item == NULL)
				break;

			BFormattingConventions conventions(item->ID());
			printf("Convention: %s\n", conventions.CountryCode());

			// _SettingsChanged();
			UpdateCurrencyLabel(conventions);
			break;
		}
		default:
			BView::MessageReceived(message);
			break;
	}
}

void
LocaleView::UpdateCurrencyLabel(BFormattingConventions conventions)
{
	BLanguage* language = new BLanguage(conventions.LanguageCode());
	BLocale* locale = new BLocale(language, &conventions);
	BNumberFormat numberFormatter(locale);
	BString curstr;

	if (numberFormatter.FormatMonetary(curstr, 1234567.89) == B_OK) {
		BString temp;
		temp.SetToFormat(B_TRANSLATE("Currency format: %s"), curstr.String());
		fLocaleBox->SetLabel(temp.String());
	}
}