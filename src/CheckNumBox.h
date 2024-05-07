#ifndef CHECKNUMBOX_H
#define CHECKNUMBOX_H

#include "AutoTextControl.h"

enum {
	M_TYPE_AUTOCOMPLETE = 'tyac'
};

class CheckNumBox;

class CheckNumBoxFilter : public AutoTextControlFilter {
public:
	CheckNumBoxFilter(CheckNumBox* box);
	filter_result KeyFilter(const int32& key, const int32& mod);
};

class CheckNumBox : public AutoTextControl {
public:
	CheckNumBox(const char* name, const char* label, const char* text, BMessage* msg,
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE);

	bool Validate(void);

private:
	friend CheckNumBoxFilter;
};

#endif
