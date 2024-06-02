/*
 * Copyright 2024, Johan Wagenheim <johan@dospuntos.no>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef LOCALEWINDOW_H
#define LOCALEWINDOW_H


#include "LanguageListView.h"
#include <Box.h>
#include <FormattingConventions.h>
#include <SupportDefs.h>
#include <View.h>


class LocaleView : public BView
{
public:
	LocaleView(const char* name, const int32& flags = B_WILL_DRAW);
	void AttachedToWindow(void);
	void MessageReceived(BMessage* message);
	void GetCurrentLocale(BString& currentLocale) { currentLocale = fCurrentLocale; }

private:
	LanguageListView* fConventionsListView;
	BBox* fLocaleBox;
	LanguageListItem* fInitialConventionsItem;
	LanguageListItem* fDefaultConventionsItem;
	void UpdateCurrencyLabel(BString code);
	BString fCurrentLocale;
};

#endif // _H
