/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 */
#ifndef CATEGORYBOX_H
#define CATEGORYBOX_H

#include "AutoTextControl.h"
#include "Database.h"

class CategoryBox;

// clang-format off
enum {
	M_CATEGORY_AUTOCOMPLETE = 'cata'
};
// clang-format on

class CategoryBoxFilter : public AutoTextControlFilter {
public:
	CategoryBoxFilter(CategoryBox* box);
	filter_result KeyFilter(const int32& key, const int32& mod);
};

class CategoryBox : public AutoTextControl {
public:
	CategoryBox(const char* name, const char* label, const char* text, BMessage* msg,
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE);

	bool Validate();
	void SetType(BString type) { fType = type; }
	BString GetType() { return fType; }

private:
	bool _SetTypeFromCategory(BString category);
	bool _AddNewCategory(BString category);
	friend CategoryBoxFilter;
	BString fType;
};

#endif // CATEGORYBOX_H
