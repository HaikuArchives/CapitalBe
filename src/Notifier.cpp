#include "Notifier.h"
#include <Message.h>
#include <String.h>
#include <stdio.h>

void
PrintNotification(const uint64& value, const BMessage* msg)
{
	BString action;
	if (value & WATCH_CREATE)
		action << "WATCH_CREATE ";

	if (value & WATCH_DELETE)
		action << "WATCH_DELETE ";

	if (value & WATCH_CHANGE)
		action << "WATCH_CHANGE ";

	if (value & WATCH_RENAME)
		action << "WATCH_RENAME ";

	if (value & WATCH_SELECT)
		action << "WATCH_SELECT ";

	BString target;

	if (value & WATCH_ACCOUNT)
		target << "WATCH_ACCOUNT ";

	if (value & WATCH_TRANSACTION)
		target << "WATCH_TRANSACTION ";

	printf("Notify: %s : %s\n", action.String(), target.String());
	msg->PrintToStream();
}
