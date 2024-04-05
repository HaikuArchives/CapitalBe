#ifndef NAVTEXTBOX_H
#define NAVTEXTBOX_H

#include "AutoTextControl.h"

class NavTextBox;

class NavTextBoxFilter : public AutoTextControlFilter {
public:
	NavTextBoxFilter(NavTextBox* box);
	filter_result KeyFilter(const int32& key, const int32& mod);
};

class NavTextBox : public AutoTextControl {
public:
	NavTextBox(const char* name, const char* label, const char* text, BMessage* msg,
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
	virtual bool Validate(const bool& alert = true);

	void UseTabFiltering(const bool& value) { fFilterTab = value; }

	bool IsTabFiltering(void) const { return fFilterTab; }

private:
	friend NavTextBoxFilter;

	bool fFilterTab;
};

#endif
