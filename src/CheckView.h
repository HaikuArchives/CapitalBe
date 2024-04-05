#ifndef CHECKVIEW_H
#define CHECKVIEW_H

#include <Button.h>
#include <MessageFilter.h>
#include <Messenger.h>
#include <StringView.h>
#include <TextControl.h>
#include <View.h>
#include <time.h>

#include "Account.h"
#include "HelpButton.h"
#include "Notifier.h"

enum
{
	M_DATE_CHANGED = 'dtch',
	M_TYPE_CHANGED = 'tych',
	M_PAYEE_CHANGED = 'pych',
	M_AMOUNT_CHANGED = 'amch',
	M_CATEGORY_CHANGED = 'ctch',
	M_MEMO_CHANGED = 'mmch'
};

class CheckViewFilter;
class CategoryBox;
class CheckNumBox;
class CurrencyBox;
class DateBox;
class NavTextBox;
class PayeeBox;

class CheckView : public BView, public Observer {
public:
	CheckView(const char* name, int32 flags);
	~CheckView(void);
	void AttachedToWindow(void);
	void MessageReceived(BMessage* msg);
	void SetFields(const char* date, const char* type, const char* payee, const char* amount,
		const char* category, const char* memo);

	void HandleNotify(const uint64& value, const BMessage* msg);
	void MakeEmpty(void);
	void MakeFocus(bool value = true);
	void FrameResized(float width, float height);

private:
	void DoNextField(void);

	DateBox* fDate;
	CheckNumBox* fType;
	PayeeBox* fPayee;
	CurrencyBox* fAmount;
	CategoryBox* fCategory;
	NavTextBox* fMemo;

	BStringView *fDateLabel, *fTypeLabel, *fPayeeLabel, *fAmountLabel, *fCategoryLabel, *fMemoLabel;
	BButton *fEnter, *fTransfer;

	HelpButton* fHelpButton;
};


#endif
