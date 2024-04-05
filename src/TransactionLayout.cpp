#include "TransactionLayout.h"


static uint8 sLeftPadding = 5;
static uint8 sRowHeight = 0;
static uint8 sNumWidth = 0;
static uint8 sDateWidth = 0;
static uint8 sAmountWidth = 0;

void
InitTransactionItemLayout(BView* owner)
{
	font_height fh;
	owner->GetFontHeight(&fh);
	float fontheight = fh.ascent + fh.descent + fh.leading;
	sRowHeight = (uint8)fontheight + 2;

	sNumWidth = (uint8)owner->StringWidth("000000");
	sDateWidth = (uint8)owner->StringWidth("0000000000");
	sAmountWidth = (uint8)owner->StringWidth("00,000,000.00");
}

uint8
TLeftPadding(void)
{
	return sLeftPadding;
}
uint8
TRowHeight(void)
{
	return sRowHeight;
}
uint8
TNumWidth(void)
{
	return sNumWidth;
}
uint8
TDateWidth(void)
{
	return sDateWidth;
}
uint8
TAmountWidth(void)
{
	return sAmountWidth;
}
