#ifndef CATEGORYBOX_H
#define CATEGORYBOX_H

#include "AutoTextControl.h"

class CategoryBox;

enum { M_CATEGORY_AUTOCOMPLETE = 'cata' };

class CategoryBoxFilter : public AutoTextControlFilter {
  public:
	CategoryBoxFilter(CategoryBox *box);
	filter_result KeyFilter(const int32 &key, const int32 &mod);
};

class CategoryBox : public AutoTextControl {
  public:
	CategoryBox(
		const char *name, const char *label, const char *text, BMessage *msg,
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE
	);

	bool Validate(void);

  private:
	friend CategoryBoxFilter;
};

#endif
