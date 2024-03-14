#ifndef DATEBOX_H
#define DATEBOX_H

#include "AutoTextControl.h"

class DateBox;

class DateBoxFilter : public AutoTextControlFilter {
public:
	DateBoxFilter(DateBox* box);
	filter_result KeyFilter(const int32& key, const int32& mod);
};

class DateBox : public AutoTextControl {
public:
	DateBox(const char* name, const char* label, const char* text, BMessage* msg,
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE);

	void SetDate(const time_t& date) { fCurrentDate = date; }

	time_t GetDate(void) const { return fCurrentDate; }

	bool Validate(const bool& alert = true);

	void UseTabFiltering(const bool& value) { fFilterTab = value; }

	bool IsTabFiltering(void) const { return fFilterTab; }

private:
	friend DateBoxFilter;
	time_t fCurrentDate;
	bool fFilterTab;
};

#endif
