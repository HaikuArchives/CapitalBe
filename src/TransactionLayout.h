#ifndef TRANSACTION_LAYOUT
#define TRANSACTION_LAYOUT

#include <View.h>

uint8 TLeftPadding();
uint8 TRowHeight();
uint8 TNumWidth();
uint8 TDateWidth();
uint8 TAmountWidth();

void InitTransactionItemLayout(BView* owner);

#endif
