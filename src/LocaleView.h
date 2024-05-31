/*
 * Copyright 2024, Johan Wagenheim <johan@dospuntos.no>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef LOCALEWINDOW_H
#define LOCALEWINDOW_H


#include <Box.h>
#include <FormattingConventions.h>
#include <SupportDefs.h>
#include <View.h>
#include "LanguageListView.h"


class LocaleView : public BView {
public:
	LocaleView(const char* name, const int32& flags = B_WILL_DRAW);
	void 				AttachedToWindow(void);
	void 				MessageReceived(BMessage* message);
private:
	LanguageListView*	fConventionsListView;
	BBox* fLocaleBox;
	LanguageListItem*	fInitialConventionsItem;
	LanguageListItem*	fDefaultConventionsItem;
	double				fSampleAmount;
	void 				UpdateCurrencyLabel(BFormattingConventions conventions);
};

#endif // _H
