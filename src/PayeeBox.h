#ifndef PAYEEBOX_H
#define PAYEEBOX_H

#include "AutoTextControl.h"

enum {
	M_PAYEE_AUTOCOMPLETE = 'acpy'
};

class PayeeBox;

class PayeeBoxFilter : public AutoTextControlFilter {
public:
	PayeeBoxFilter(PayeeBox* box);
	filter_result KeyFilter(const int32& key, const int32& mod);
};

class PayeeBox : public AutoTextControl {
public:
	PayeeBox(const char* name, const char* label, const char* text, BMessage* msg,
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE);

	bool Validate(const bool& showalert = true);

private:
	friend PayeeBoxFilter;
};

#endif
