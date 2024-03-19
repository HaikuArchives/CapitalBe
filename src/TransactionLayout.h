#ifndef TRANSACTION_LAYOUT
#define TRANSACTION_LAYOUT

#include <View.h>

uint8
TLeftPadding(void);
uint8
TRowHeight(void);
uint8
TNumWidth(void);
uint8
TDateWidth(void);
uint8
TAmountWidth(void);

void
InitTransactionItemLayout(BView* owner);

#endif
