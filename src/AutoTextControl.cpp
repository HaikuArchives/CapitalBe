/*
 * Copyright 2007-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * A BTextControl which notifies on each keypress
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	humdinger (Joachim Seemer)
 */
#include "AutoTextControl.h"

#include <PropertyInfo.h>
#include <String.h>
#include <Window.h>

#include <ctype.h>
#include <stdio.h>

static property_info sProperties[] = {
	{"CharacterLimit", {B_GET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0},
		"Returns the maximum number of characters that the AutoTextControl will allow.", 0,
		{B_INT32_TYPE}},

	{"CharacterLimit", {B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0},
		"Sets the maximum number of characters that the AutoTextControl will allow.", 0,
		{B_INT32_TYPE}},
};


AutoTextControl::AutoTextControl(
	const char* name, const char* label, const char* text, BMessage* msg, uint32 flags)
	: BTextControl(name, label, text, msg, flags),
	  fFilter(NULL),
	  fCharLimit(0)
{
	SetFilter(new AutoTextControlFilter(this));
}


AutoTextControl::~AutoTextControl()
{
	if (Window())
		Window()->RemoveCommonFilter(fFilter);

	delete fFilter;
}


AutoTextControl::AutoTextControl(BMessage* data)
	: BTextControl(data)
{
	if (data->FindInt32("_charlimit", (int32*)&fCharLimit) != B_OK)
		fCharLimit = 0;
}


BArchivable*
AutoTextControl::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "AutoTextControl"))
		return new AutoTextControl(data);

	return NULL;
}


status_t
AutoTextControl::Archive(BMessage* data, bool deep) const
{
	status_t status = BTextControl::Archive(data, deep);

	if (status == B_OK)
		status = data->AddInt32("_charlimit", fCharLimit);

	if (status == B_OK)
		status = data->AddString("class", "AutoTextControl");

	return status;
}


status_t
AutoTextControl::GetSupportedSuites(BMessage* msg)
{
	msg->AddString("suites", "suite/vnd.DW-autotextcontrol");

	BPropertyInfo prop_info(sProperties);
	msg->AddFlat("messages", &prop_info);
	return BTextControl::GetSupportedSuites(msg);
}


BHandler*
AutoTextControl::ResolveSpecifier(
	BMessage* msg, int32 index, BMessage* specifier, int32 form, const char* property)
{
	return BControl::ResolveSpecifier(msg, index, specifier, form, property);
}


void
AutoTextControl::AttachedToWindow()
{
	BTextControl::AttachedToWindow();
	if (fFilter) {
		Window()->AddCommonFilter(fFilter);
		fFilter->SetMessenger(new BMessenger(NULL, Window()));
	}
}


void
AutoTextControl::DetachedFromWindow()
{
	if (fFilter) {
		fFilter->SetMessenger(NULL);
		Window()->RemoveCommonFilter(fFilter);
	}
	BTextControl::DetachedFromWindow();
}


void
AutoTextControl::SetCharacterLimit(const uint32& limit)
{
	fCharLimit = limit;
}


uint32
AutoTextControl::GetCharacterLimit(const uint32& limit)
{
	return fCharLimit;
}


void
AutoTextControl::SetFilter(AutoTextControlFilter* filter)
{
	if (fFilter) {
		if (Window())
			Window()->RemoveCommonFilter(fFilter);
		delete fFilter;
	}

	fFilter = filter;
	if (Window())
		Window()->AddCommonFilter(fFilter);
}


AutoTextControlFilter::AutoTextControlFilter(AutoTextControl* box)
	: BMessageFilter(B_PROGRAMMED_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN),
	  fBox(box),
	  fCurrentMessage(NULL),
	  fMessenger(NULL)
{
}


AutoTextControlFilter::~AutoTextControlFilter() {}


filter_result
AutoTextControlFilter::Filter(BMessage* msg, BHandler** target)
{
	int32 rawchar, mod;
	msg->FindInt32("raw_char", &rawchar);
	msg->FindInt32("modifiers", &mod);

	BView* view = dynamic_cast<BView*>(*target);
	if (!view || strcmp("_input_", view->Name()) != 0)
		return B_DISPATCH_MESSAGE;

	AutoTextControl* text = dynamic_cast<AutoTextControl*>(view->Parent());
	if (!text || text != fBox)
		return B_DISPATCH_MESSAGE;

	fCurrentMessage = msg;
	filter_result result = KeyFilter(rawchar, mod);
	fCurrentMessage = NULL;

	if (fBox->fCharLimit && result == B_DISPATCH_MESSAGE) {
		// See to it that we still allow shortcut keys
		if (mod & B_COMMAND_KEY)
			return B_DISPATCH_MESSAGE;

		// We don't use strlen() because it is not UTF-8 aware, which can affect
		// how many characters can be typed.
		if (isprint(rawchar) && (uint32)BString(text->Text()).CountChars() == text->fCharLimit)
			return B_SKIP_MESSAGE;
	}

	return result;
}


filter_result
AutoTextControlFilter::KeyFilter(const int32& rawchar, const int32& mod)
{
	if (fBox)
		fBox->Invoke();

	return B_DISPATCH_MESSAGE;
}


void
AutoTextControlFilter::SendMessage(BMessage* msg)
{
	if (fMessenger && msg)
		fMessenger->SendMessage(msg);
}


void
AutoTextControlFilter::SetMessenger(BMessenger* msgr)
{
	if (fMessenger)
		delete fMessenger;
	fMessenger = msgr;
}


BMessenger*
AutoTextControlFilter::GetMessenger()
{
	return fMessenger;
}
