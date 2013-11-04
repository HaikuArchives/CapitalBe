#include "CategoryWindow.h"

#include <Button.h>
#include <CheckBox.h>
#include <OutlineListView.h>
#include <ListItem.h>
#include <Message.h>
#include <Messenger.h>
#include <Region.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextView.h>
#include <View.h>

#include "AutoTextControl.h"
#include "Database.h"
#include "EscapeCancelFilter.h"
#include "Preferences.h"
#include "Translate.h"

enum
{
	M_SHOW_ADD_WINDOW='shaw',
	M_SHOW_REMOVE_WINDOW,
	M_SHOW_EDIT_WINDOW,
	M_EDIT_CATEGORY,
	M_ADD_CATEGORY,
	M_REMOVE_CATEGORY,
	M_NAME_CHANGED
};

class CategoryItem : public BStringItem
{
public:
	CategoryItem(const BString &name);
	void DrawItem(BView *owner, BRect frame, bool complete = false);
};

class CategoryView : public BView
{
public:
	CategoryView(const BRect &frame,const char *name, const int32 &resize,
					const int32 &flags);
	void AttachedToWindow(void);
	void MessageReceived(BMessage *msg);

private:
	float RefreshCategoryList(void);
	
	BOutlineListView	*fListView;
	
	BButton		*fAddButton,
				*fRemoveButton,
				*fEditButton;
	
	CategoryItem *fIncomeItem,
				*fSpendingItem;
	float		fBestWidth;
};

class CategoryInputWindow : public BWindow
{
public:
	CategoryInputWindow(const BRect &frame, BView *target);
	void MessageReceived(BMessage *msg);

private:
	AutoTextControl	*fNameBox;
	
	BButton		*fOKButton,
				*fCancelButton;
	
	BCheckBox	*fExpenseBox;
	BView	 	*fTarget;
};

class CategoryEditWindow : public BWindow
{
public:
	CategoryEditWindow(const BRect &frame, const char *oldname, BView *target);
	void MessageReceived(BMessage *msg);

private:
	AutoTextControl	*fNameBox;
	
	BButton		*fOKButton,
				*fCancelButton;
	
	BString 	fOldName;
	
	BView	 	*fTarget;
};

class CategoryRemoveWindow : public BWindow
{
public:
	CategoryRemoveWindow(const BRect &frame, const char *from, BView *target);
	void MessageReceived(BMessage *msg);
	void FrameResized(float w, float h);
private:
	
	BOutlineListView	*fListView;
	
	CategoryItem	*fIncomeItem,
					*fSpendingItem;
	
	BTextView	*fDirections;
	
	BButton		*fOKButton,
				*fCancelButton;
	BView	 	*fTarget;
	BScrollView *fScrollView;
};

CategoryView::CategoryView(const BRect &frame,const char *name,
							const int32 &resize,const int32 &flags)
 :	BView(frame,name,resize,flags)
{
	BString temp;
	SetViewColor(240,240,240);
	
	BRect r;
	
	// the buttons
	temp = TRANSLATE("Edit"); temp+="…";
	fEditButton = new BButton(BRect(0,0,1,1),"editbutton",temp.String(),
						new BMessage(M_SHOW_EDIT_WINDOW),B_FOLLOW_LEFT|B_FOLLOW_BOTTOM);
	
	fEditButton->ResizeToPreferred();
	fEditButton->MoveTo(15, Bounds().Height() - fEditButton->Frame().Height() - 15);
	
	temp = TRANSLATE("Remove"); temp+="…";
	fRemoveButton = new BButton(BRect(0,0,1,1),"removebutton",temp.String(),
						new BMessage(M_SHOW_REMOVE_WINDOW),B_FOLLOW_LEFT|B_FOLLOW_BOTTOM);
	
	fRemoveButton->ResizeToPreferred();
	fRemoveButton->MoveTo(fEditButton->Frame().right + 10, fEditButton->Frame().top);
	
	temp = TRANSLATE("Add"); temp+="…";
	fAddButton = new BButton(fRemoveButton->Frame(),"addbutton",temp.String(),
						new BMessage(M_SHOW_ADD_WINDOW),B_FOLLOW_LEFT|B_FOLLOW_BOTTOM);
	
	fAddButton->ResizeToPreferred();
	fAddButton->MoveTo(fRemoveButton->Frame().right + 10,
					fEditButton->Frame().top);
	AddChild(fEditButton);
	AddChild(fRemoveButton);
	AddChild(fAddButton);
	
	r = (Bounds().InsetByCopy(15,15));
	r.bottom = fRemoveButton->Frame().top - 15;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	
	// the category list
	fListView = new BOutlineListView(r,"categorylist",B_SINGLE_SELECTION_LIST,
									B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS |
									B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE);
	BScrollView *sv = new BScrollView("scrollview",fListView,B_FOLLOW_ALL,0,
										false,true);
	AddChild(sv);
		
	fIncomeItem = new CategoryItem(TRANSLATE("Income"));
	fSpendingItem = new CategoryItem(TRANSLATE("Spending"));
	
	float maxwidth = RefreshCategoryList();
	fBestWidth = (fRemoveButton->Frame().Width()*3) + 45;
	fBestWidth = MAX(fBestWidth, maxwidth + 35);
}

void CategoryView::AttachedToWindow(void)
{
	fListView->SetTarget(this);
	fEditButton->SetTarget(this);
	fAddButton->SetTarget(this);
	fRemoveButton->SetTarget(this);
	
	fListView->Select(0);
	
	Window()->ResizeTo(fBestWidth,Window()->Frame().Height());
	fListView->MakeFocus(true);
}

void CategoryView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_SHOW_ADD_WINDOW:
		{
			BRect r(Window()->Frame());
			CategoryInputWindow *catwin = new CategoryInputWindow(BRect(100,100,300,225),this);
			catwin->MoveTo(r.left + ((Bounds().Width()-catwin->Bounds().Width())/2),
							r.top + ((Bounds().Height()-catwin->Bounds().Height())/2) );
			catwin->Show();
			break;
		}
		case M_SHOW_REMOVE_WINDOW:
		{
			int32 index = fListView->CurrentSelection();
			CategoryItem *item = (CategoryItem*)fListView->ItemAt(index);
			
			if(!item || item==fIncomeItem || item==fSpendingItem)
				break;
			
			BRect r(Window()->Frame());
			CategoryRemoveWindow *catwin = new CategoryRemoveWindow(r,item->Text(),this);
			catwin->MoveTo(r.left + ((Bounds().Width()-catwin->Bounds().Width())/2),
							r.top + ((Bounds().Height()-catwin->Bounds().Height())/2) );
			catwin->Show();
			break;
		}
		case M_SHOW_EDIT_WINDOW:
		{
			int32 index = fListView->CurrentSelection();
			CategoryItem *item = (CategoryItem*)fListView->ItemAt(index);
			
			if(!item || item==fIncomeItem || item==fSpendingItem)
				break;
			
			BRect r(Window()->Frame());
			CategoryEditWindow *catwin = new CategoryEditWindow(r,item->Text(),this);
			catwin->MoveTo(r.left + ((Bounds().Width()-catwin->Bounds().Width())/2),
							r.top + ((Bounds().Height()-catwin->Bounds().Height())/2) );
			catwin->Show();
			break;
		}
		case M_ADD_CATEGORY:
		{
			BString name;
			bool expense;
			if(msg->FindString("name",&name)!=B_OK ||
					msg->FindBool("expense",&expense)!=B_OK)
				break;
			
			if(name.ICompare(TRANSLATE("Income"))==0 || name.ICompare(TRANSLATE("Spending"))==0 ||
				name.ICompare(TRANSLATE("Split"))==0)
			{
				ShowAlert(TRANSLATE("Can't use this category name"),
							TRANSLATE("Capital Be uses the words 'Income','Spending', and 'Split' "
							"for managing categories, so you can't use them as category names. "
							"Please choose a different name for your new category."));
				break;
			}
			
			gDatabase.AddCategory(name.String(),expense);
			RefreshCategoryList();
			break;
		}
		case M_REMOVE_CATEGORY:
		{
			BString newcat;
			if(msg->FindString("newcat",&newcat)!=B_OK)
				break;
			
			int32 index = fListView->CurrentSelection();
			CategoryItem *item = (CategoryItem*)fListView->ItemAt(index);
			
			if(!item || item==fIncomeItem || item==fSpendingItem)
				break;
			
			gDatabase.RecategorizeTransactions(item->Text(), newcat.String());
			gDatabase.RemoveCategory(item->Text());
			RefreshCategoryList();
			
			break;
		}
		case M_EDIT_CATEGORY:
		{
			
			BString newname;
			if(msg->FindString("newname",&newname)!=B_OK)
				break;
			
			int32 index = fListView->CurrentSelection();
			CategoryItem *item = (CategoryItem*)fListView->ItemAt(index);
			
			if(!item || item==fIncomeItem || item==fSpendingItem)
				break;
			
			gDatabase.RecategorizeTransactions(item->Text(), newname.String());
			gDatabase.RemoveCategory(item->Text());
			RefreshCategoryList();
			break;
		}
		default:
		{
			BView::MessageReceived(msg);
			break;
		}
	}
}

float CategoryView::RefreshCategoryList(void)
{
	if(fListView->CountItems()>0)
	{
		if(fListView->HasItem(fIncomeItem))
			fListView->RemoveItem(fIncomeItem);
		if(fListView->HasItem(fSpendingItem))
			fListView->RemoveItem(fSpendingItem);
		
		fListView->MakeEmpty();
	}
	fListView->AddItem(fIncomeItem);
	fListView->AddItem(fSpendingItem);
	
	int32 maxchars;
	float maxlength;
	if(strlen(TRANSLATE("Income"))>strlen(TRANSLATE("Spending")))
	{
		maxchars = strlen(TRANSLATE("Income"));
		maxlength = StringWidth(TRANSLATE("Income"));
	}
	else
	{
		maxchars = strlen(TRANSLATE("Spending"));
		maxlength = StringWidth(TRANSLATE("Spending"));
	}
	
	CppSQLite3Query query = gDatabase.DBQuery("select * from categorylist order by name desc",
											"CategoryView::CategoryView");
	while(!query.eof())
	{
		int expense = query.getIntField(1);
		BString name = query.getStringField(0);
		
		if(expense==0)
			fListView->AddUnder(new CategoryItem(DeescapeIllegalCharacters(name.String())),
									fSpendingItem);
		else
			fListView->AddUnder(new CategoryItem(DeescapeIllegalCharacters(name.String())),
									fIncomeItem);
		
		if(name.CountChars()>maxchars)
		{
			maxchars = name.CountChars();
			maxlength = StringWidth(name.String());
		}
		
		query.nextRow();
	}
	
	return maxlength;
}

CategoryWindow::CategoryWindow(const BRect &frame)
 :	BWindow(frame,TRANSLATE("Categories"),B_DOCUMENT_WINDOW_LOOK,B_NORMAL_WINDOW_FEEL,
 			B_ASYNCHRONOUS_CONTROLS)
{
	AddCommonFilter(new EscapeCancelFilter);
	
	CategoryView *view = new CategoryView(Bounds(),"categoryview",B_FOLLOW_ALL,B_WILL_DRAW);
	AddChild(view);
	
	AddShortcut('A',B_COMMAND_KEY, new BMessage(M_SHOW_ADD_WINDOW),view);
	AddShortcut('R',B_COMMAND_KEY, new BMessage(M_REMOVE_CATEGORY),view);
}

CategoryItem::CategoryItem(const BString &name)
 :	BStringItem(name.String())
{
}

void CategoryItem::DrawItem(BView *owner, BRect frame, bool complete)
{
	if(IsSelected())
	{
		owner->SetHighColor(GetColor(BC_SELECTION_FOCUS));
		owner->SetLowColor(GetColor(BC_SELECTION_FOCUS));
	}
	else
	{
		owner->SetHighColor(255, 255, 255);
		owner->SetLowColor(255, 255, 255);
	}
	owner->FillRect(frame);
	if(IsSelected())
	{
		owner->SetHighColor(100, 100, 100);
		owner->StrokeRect(frame);
	}
	
	owner->SetHighColor(0,0,0);
	
	BRegion region(frame);
	owner->ConstrainClippingRegion(&region);
	owner->DrawString(Text(),BPoint(frame.left+1,frame.bottom-2));
	owner->ConstrainClippingRegion(NULL);
}

CategoryInputWindow::CategoryInputWindow(const BRect &frame, BView *target)
 :	BWindow(frame,TRANSLATE("Add Category"),B_FLOATING_WINDOW_LOOK,B_MODAL_APP_WINDOW_FEEL,
 			B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE |
 			B_NOT_V_RESIZABLE),
 	fTarget(target)
{
	BString temp;
	AddCommonFilter(new EscapeCancelFilter);
	AddShortcut('W',B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	
	BView *view = new BView(Bounds(),"background",B_FOLLOW_ALL,B_WILL_DRAW);
	AddChild(view);
	
	view->SetViewColor(240,240,240);
	
	temp = TRANSLATE("Category Name"); temp+=":";
	fNameBox = new AutoTextControl(BRect(15,15,20,20),"namebox",temp.String(),
									"",new BMessage(M_NAME_CHANGED),
									B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fNameBox->SetCharacterLimit(32);
	view->AddChild(fNameBox);
	fNameBox->SetDivider(fNameBox->StringWidth(temp.String()));
	fNameBox->ResizeToPreferred();
	
	fExpenseBox = new BCheckBox(BRect(10,10,20,20),"expensebox",TRANSLATE("Spending Category"),
								NULL,B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT);
	view->AddChild(fExpenseBox);
	fExpenseBox->ResizeToPreferred();
	fExpenseBox->MoveTo(15,	fNameBox->Frame().bottom + 5);
	fExpenseBox->SetValue(B_CONTROL_ON);
	
	fOKButton = new BButton(BRect(0,0,1,1),"okbutton",TRANSLATE("Cancel"),
						new BMessage(M_ADD_CATEGORY),B_FOLLOW_LEFT|B_FOLLOW_BOTTOM);
	
	fOKButton->ResizeToPreferred();
	fOKButton->MoveTo(Bounds().Width() - fOKButton->Frame().Width() - 15,
					fExpenseBox->Frame().bottom + 5);
	fOKButton->SetLabel(TRANSLATE("OK"));
	
	ResizeTo( (fOKButton->Frame().Width()*2) + 45, fOKButton->Frame().bottom + 15);
	
	fOKButton->MakeDefault(true);

	fCancelButton = new BButton(fOKButton->Frame(),"cancelbutton",TRANSLATE("Cancel"),
						new BMessage(B_QUIT_REQUESTED),B_FOLLOW_LEFT|B_FOLLOW_BOTTOM);
	
	fCancelButton->ResizeToPreferred();
	fCancelButton->MoveTo(fOKButton->Frame().left - fCancelButton->Frame().Width() - 10,
					fOKButton->Frame().top);
	view->AddChild(fCancelButton);
	view->AddChild(fOKButton);
	
	fNameBox->ResizeTo(Bounds().Width() - 30,fNameBox->Frame().Height());
	fOKButton->SetEnabled(false);
	fNameBox->MakeFocus(true);
	
	SetSizeLimits(Frame().Width(),30000,Frame().Height(),30000);
}

void CategoryInputWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_NAME_CHANGED:
		{
			if(strlen(fNameBox->Text())>0)
			{
				if(!fOKButton->IsEnabled())
					fOKButton->SetEnabled(true);
			}
			else
			{
				if(fOKButton->IsEnabled())
					fOKButton->SetEnabled(false);
			}
			break;
		}
		case M_ADD_CATEGORY:
		{
			BMessenger msgr(fTarget);
			msg->AddString("name",fNameBox->Text());
			msg->AddBool("expense",fExpenseBox->Value() == B_CONTROL_ON);
			msgr.SendMessage(msg);
			break;
		}
		default:
		{
			BWindow::MessageReceived(msg);
			break;
		}
	}
}

CategoryRemoveWindow::CategoryRemoveWindow(const BRect &frame, const char *from, BView *target)
 :	BWindow(frame,TRANSLATE("Remove Category"),B_FLOATING_WINDOW_LOOK,B_MODAL_APP_WINDOW_FEEL,
 			B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE),
 	fTarget(target)
{
	rgb_color backcolor = {240,240,240,255};
	
	AddCommonFilter(new EscapeCancelFilter);
	AddShortcut('W',B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	BView *view = new BView(Bounds(),"background",B_FOLLOW_ALL,B_WILL_DRAW | B_FRAME_EVENTS);
	AddChild(view);
	view->SetViewColor(backcolor);
	
	BRect r(Bounds());
	r.InsetBy(10,10);
	r.bottom /= 2;
		
	fDirections = new BTextView(r,"directions",r.OffsetToCopy(0,0),
								B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fDirections->MakeEditable(false);
	
	BString directions(
		TRANSLATE("Please choose a new category for all transactions currently in the "
					"%%CATEGORY_NAME%% category."));
	directions.ReplaceFirst("%%CATEGORY_NAME%%",from);
	fDirections->SetText(directions.String());
	fDirections->SetViewColor(backcolor);
	fDirections->SetWordWrap(true);
	view->AddChild(fDirections);
//	fDirections->ResizeTo(fDirections->Bounds().Width() - 10,
//							(fDirections->LineHeight() * fDirections->CountLines())+20);
//	fDirections->FrameResized(Bounds().Width(),Bounds().Height());
	
	// the buttons
	fOKButton = new BButton(BRect(0,0,1,1),"okbutton",TRANSLATE("Cancel"),
						new BMessage(M_REMOVE_CATEGORY),B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
	
	fOKButton->ResizeToPreferred();
	fOKButton->SetLabel(TRANSLATE("OK"));

	fCancelButton = new BButton(fOKButton->Frame(),"cancelbutton",TRANSLATE("Cancel"),
						new BMessage(B_QUIT_REQUESTED),B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
	
	fCancelButton->ResizeToPreferred();
	
	// Put off moving the buttons and adding them so that we have proper keyboard navigation
	// order. We only created them here so that we can use their proper size in calculating
	// the control layout
	
	// the category list	
	r = (Bounds().InsetByCopy(15,15));
	r.top = fDirections->Frame().bottom + 10;
	r.bottom = Bounds().bottom - 20 - fOKButton->Frame().Height();
	r.right -= B_V_SCROLL_BAR_WIDTH;
	
	fListView = new BOutlineListView(r,"categorylist",B_SINGLE_SELECTION_LIST,
									B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS |
									B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE);
	fScrollView = new BScrollView("scrollview",fListView,B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM,0,
										false,true);
	view->AddChild(fScrollView);
		
	fIncomeItem = new CategoryItem(TRANSLATE("Income"));
	fSpendingItem = new CategoryItem(TRANSLATE("Spending"));
	fListView->AddItem(fIncomeItem);
	fListView->AddItem(fSpendingItem);
	
	CppSQLite3Query query = gDatabase.DBQuery("select * from categorylist order by name desc",
											"CategoryView::CategoryView");
	
	int32 maxchars;
	float maxlength;
	if(strlen(TRANSLATE("Income"))>strlen(TRANSLATE("Spending")))
	{
		maxchars = strlen(TRANSLATE("Income"));
		maxlength = view->StringWidth(TRANSLATE("Income"));
	}
	else
	{
		maxchars = strlen(TRANSLATE("Spending"));
		maxlength = view->StringWidth(TRANSLATE("Spending"));
	}
	
	while(!query.eof())
	{
		int expense = query.getIntField(1);
		BString name = DeescapeIllegalCharacters(query.getStringField(0));
		
		if(name.Compare(from)==0)
		{
			query.nextRow();
			continue;
		}
		
		if(expense==0)
			fListView->AddUnder(new CategoryItem(name.String()),
									fSpendingItem);
		else
			fListView->AddUnder(new CategoryItem(name.String()),
									fIncomeItem);
		
		if(name.CountChars()>maxchars)
		{
			maxchars = name.CountChars();
			maxlength = view->StringWidth(name.String());
		}
		
		query.nextRow();
	}
	
	float bestwidth = (fOKButton->Frame().Width()*2) + 45;
	bestwidth = MAX(bestwidth, maxlength + 35);
	ResizeTo(bestwidth,Frame().Height());

	fOKButton->MoveTo(Bounds().Width() - fOKButton->Frame().Width() - 15,
					Bounds().Height() - fOKButton->Frame().Height() - 15);
	view->AddChild(fOKButton);
	
	fCancelButton->MoveTo(fOKButton->Frame().left - fCancelButton->Frame().Width() - 10,
					fOKButton->Frame().top);
	view->AddChild(fCancelButton);
}

void CategoryRemoveWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_REMOVE_CATEGORY:
		{
			int32 index = fListView->CurrentSelection();
			CategoryItem *item = (CategoryItem*)fListView->ItemAt(index);
			
			if(!item || item==fIncomeItem || item==fSpendingItem)
				break;
			
			BMessenger msgr(fTarget);
			msg->AddString("newcat",item->Text());
			msgr.SendMessage(msg);
			
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
		{
			BWindow::MessageReceived(msg);
			break;
		}
	}
}

void CategoryRemoveWindow::FrameResized(float w, float h)
{
	fDirections->ResizeTo(Bounds().Width() - 20,
						(fDirections->LineHeight() * fDirections->CountLines())+10);
	fDirections->SetTextRect(fDirections->Bounds());
	fScrollView->MoveTo(10,fDirections->Frame().bottom);
	fScrollView->ResizeTo(fScrollView->Frame().Width(),fOKButton->Frame().top - 10 - fScrollView->Frame().top);
}

CategoryEditWindow::CategoryEditWindow(const BRect &frame, const char *oldname, BView *target)
 :	BWindow(frame,TRANSLATE("Edit Category"),B_FLOATING_WINDOW_LOOK,B_MODAL_APP_WINDOW_FEEL,
 			B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE),
 	fOldName(oldname),
 	fTarget(target)
{
	BString temp;
	AddCommonFilter(new EscapeCancelFilter);
	AddShortcut('W',B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	
	BView *view = new BView(Bounds(),"background",B_FOLLOW_ALL,B_WILL_DRAW | B_FRAME_EVENTS);
	AddChild(view);
	
	view->SetViewColor(240,240,240);
	
	temp = TRANSLATE("Category Name"); temp << ": " << fOldName;
	BStringView *oldname = new BStringView(BRect(15,15,16,16),"oldname",temp.String());
	oldname->ResizeToPreferred();
	view->AddChild(oldname);
	
	
	temp = TRANSLATE("New Category Name"); temp+=":";
	fNameBox = new AutoTextControl(BRect(15,15,20,20),"namebox",temp.String(),
									"",new BMessage(M_NAME_CHANGED),
									B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fNameBox->SetCharacterLimit(32);
	view->AddChild(fNameBox);
	fNameBox->SetDivider(fNameBox->StringWidth(temp.String()));
	fNameBox->ResizeToPreferred();
	fNameBox->MoveTo(15, oldname->Frame().bottom + 10);
	
	
	fOKButton = new BButton(BRect(0,0,1,1),"okbutton",TRANSLATE("Cancel"),
						new BMessage(M_EDIT_CATEGORY),B_FOLLOW_NONE);
	fOKButton->SetFlags(fOKButton->Flags() | B_FRAME_EVENTS);
	fOKButton->ResizeToPreferred();
	fOKButton->SetLabel(TRANSLATE("OK"));
	
	
	float bestwidth = (fOKButton->Frame().Width()*2) + 45;
	bestwidth = MAX(bestwidth, oldname->Frame().right + 15);
	
	ResizeTo( bestwidth, fNameBox->Frame().bottom + fOKButton->Bounds().Height() + 30);
	fOKButton->MoveTo(Bounds().right - fOKButton->Frame().Width() - 15,
					fNameBox->Frame().bottom + 15);
	
	
	fOKButton->MakeDefault(true);

	fCancelButton = new BButton(fOKButton->Frame(),"cancelbutton",TRANSLATE("Cancel"),
						new BMessage(B_QUIT_REQUESTED),B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
	
	fCancelButton->ResizeToPreferred();
	fCancelButton->MoveTo(fOKButton->Frame().left - fCancelButton->Frame().Width() - 10,
					fOKButton->Frame().top + 
					(B_BEOS_VERSION > 0x510 ? 0: 2) );
	view->AddChild(fCancelButton);
	view->AddChild(fOKButton);
	
	fNameBox->ResizeTo(Bounds().Width() - 30,fNameBox->Frame().Height());
	fOKButton->SetEnabled(false);
	fNameBox->MakeFocus(true);
	
	SetSizeLimits(Frame().Width(),30000,Frame().Height(),30000);
}

void CategoryEditWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_NAME_CHANGED:
		{
			if(strlen(fNameBox->Text())>0)
			{
				if(!fOKButton->IsEnabled())
					fOKButton->SetEnabled(true);
			}
			else
			{
				if(fOKButton->IsEnabled())
					fOKButton->SetEnabled(false);
			}
			break;
		}
		case M_EDIT_CATEGORY:
		{
			BMessenger msgr(fTarget);
			msg->AddString("newname",fNameBox->Text());
			msgr.SendMessage(msg);
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
		{
			BWindow::MessageReceived(msg);
			break;
		}
	}
}
