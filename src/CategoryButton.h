#ifndef CATEGORYBUTTON_H
#define CATEGORYBUTTON_H


#include <Button.h>
#include <PopUpMenu.h>
#include <SupportDefs.h>

#include "CategoryBox.h"


class CategoryButton : public BButton {
public:
	CategoryButton(CategoryBox* box);

	void AttachedToWindow(void);
	virtual void MessageReceived(BMessage* message);

private:
	BBitmap* DrawIcon();
	void ShowPopUpMenu();

	CategoryBox* fCategoryBox;
	bool fShowingPopUpMenu;
};


class CategoryPopUp : public BPopUpMenu {
public:
					CategoryPopUp(const char* name, BMessenger target);
	virtual 		~CategoryPopUp();

private:
	BMessenger 		fTarget;
};

#endif	// CATEGORYBUTTON_H
