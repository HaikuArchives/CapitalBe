/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	humdinger (Joachim Seemer)
 *	dospuntos (Johan Wagenheim)
 */
#include "CategoryBox.h"
#include "CBLocale.h"
#include "Database.h"
#include "MsgDefs.h"

#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CategoryWindow"


CategoryBoxFilter::CategoryBoxFilter(CategoryBox* box)
	: AutoTextControlFilter(box)
{
}


filter_result
CategoryBoxFilter::KeyFilter(const int32& key, const int32& mod)
{
	// Here is where all the *real* work for a date box is done.
	if (key == B_TAB) {
		if (mod & B_SHIFT_KEY)
			SendMessage(new BMessage(M_PREVIOUS_FIELD));
		else
			SendMessage(new BMessage(M_NEXT_FIELD));
		return B_SKIP_MESSAGE;
	}

#ifdef ENTER_NAVIGATION
	if (key == B_ENTER) {
		SendMessage(new BMessage(M_ENTER_NAVIGATION));
		return B_SKIP_MESSAGE;
	}
#endif

	//	if(key == B_ESCAPE && !IsEscapeCancel())
	//		return B_SKIP_MESSAGE;

	if (key < 32
		|| ((mod & B_COMMAND_KEY) && !(mod & B_SHIFT_KEY) && !(mod & B_OPTION_KEY)
			&& !(mod & B_CONTROL_KEY)))
		return B_DISPATCH_MESSAGE;

	Account* acc = gDatabase.CurrentAccount();
	if (!acc)
		return B_DISPATCH_MESSAGE;

	int32 start, end;
	TextControl()->TextView()->GetSelection(&start, &end);
	if (end == (int32)strlen(TextControl()->Text())) {
		TextControl()->TextView()->Delete(start, end);
		BString string = "";
		GetCurrentMessage()->FindString("bytes", &string);

		string.Prepend(TextControl()->Text());

		BString autocomplete = acc->AutocompleteCategory(string.String());
		if (autocomplete.CountChars() == 0 || IsInternalCategory(autocomplete.String()))
			autocomplete = string;

		BMessage automsg(M_CATEGORY_AUTOCOMPLETE);
		automsg.AddInt32("start", strlen(TextControl()->Text()) + 1);
		automsg.AddString("string", autocomplete.String());
		SendMessage(&automsg);
	}

	return B_DISPATCH_MESSAGE;
}


CategoryBox::CategoryBox(
	const char* name, const char* label, const char* text, BMessage* msg, uint32 flags)
	: AutoTextControl(name, label, text, msg, flags)
{
	SetFilter(new CategoryBoxFilter(this));
	SetCharacterLimit(32);
}


bool
CategoryBox::Validate()
{
	BString category(Text());

	if (category == "") {
		DAlert* alert = new DAlert(B_TRANSLATE("Category is missing"),
			B_TRANSLATE(
				"Do you really want to add this transaction without a category?\n\n"
				"Even then, you need to select a transaction type, 'income' or 'spending'.\n"),
			B_TRANSLATE("Income"), B_TRANSLATE("Spending"), B_TRANSLATE("Cancel"), B_WIDTH_AS_USUAL,
			B_WARNING_ALERT);
		int32 value = alert->Go();
		if (value == 0) {
			fType = "DEP";
			SetText(B_TRANSLATE_CONTEXT("Uncategorized", "CommonTerms"));
			return true;
		}
		if (value == 1) {
			fType = "ATM";
			SetText(B_TRANSLATE_CONTEXT("Uncategorized", "CommonTerms"));
			return true;
		} else
			return false;
	}

	if (IsInternalCategory(category.String())) {
		ShowAlert(B_TRANSLATE("Can't use this category name"),
			B_TRANSLATE(
				"CapitalBe uses 'Income', 'Spending', 'Split', 'Transfer', and 'Uncategorized' "
				"for managing accounts, so you can't use them as category names.\n\n"
				"Please choose a different name for your new category."));
		return false;
	}

	SetText(category);
	bool success = _SetTypeFromCategory(category);
	return success;
}


bool
CategoryBox::_SetTypeFromCategory(BString category)
{
	CppSQLite3Query query = gDatabase.DBQuery(
		"SELECT * FROM categorylist ORDER BY name ASC", "CategoryView::CategoryView");

	bool categoryExists = false;
	while (!query.eof()) {
		category_type type = (category_type)query.getIntField(1);
		BString name = DeescapeIllegalCharacters(query.getStringField(0));

		if (name.ICompare(category) == 0) {
			type == SPENDING ? fType = "ATM" : fType = "DEP";
			categoryExists = true;
			break;
		}
		query.nextRow();
	}

	bool success = true;
	if (!categoryExists
		&& category.ICompare(B_TRANSLATE_ALL("Split", "CommonTerms",
			   "The noun 'split', as in 'a split-category'"))
			!= 0) {
		bool success = _AddNewCategory(category);
	}

	return success;
}


bool
CategoryBox::_AddNewCategory(BString category)
{
	BString text(
		B_TRANSLATE("You created the new category '%categoryname%'.\n\n"
					"Please select a transaction type for it, 'income' or 'spending'."));
	text.ReplaceFirst("%categoryname%", category);

	DAlert* alert = new DAlert(B_TRANSLATE("New category"), text, B_TRANSLATE("Income"),
		B_TRANSLATE("Spending"), B_TRANSLATE("Cancel"), B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	int32 value = alert->Go();
	if (value == 0) {
		fType = "DEP";
		return true;
	}
	if (value == 1) {
		fType = "ATM";
		return true;
	}
	return false;
}
