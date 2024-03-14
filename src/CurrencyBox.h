#ifndef CURRENCYBOX_H
#define CURRENCYBOX_H

#include "NavTextBox.h"

class CurrencyBox;

class CurrencyBoxFilter : public NavTextBoxFilter {
  public:
	CurrencyBoxFilter(CurrencyBox *box);
	filter_result KeyFilter(const int32 &key, const int32 &mod);
};

class CurrencyBox : public NavTextBox {
  public:
	CurrencyBox(
		const char *name, const char *label, const char *text, BMessage *msg,
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE
	);

	bool Validate(bool alert = true);

  private:
	friend CurrencyBoxFilter;
};

#endif
