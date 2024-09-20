/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	dospuntos (Johan Wagenheim)
 *	humdinger (Joachim Seemer)
 *	n0toose (Panagiotis Vasilopoulos)
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Database.h"
#include "Notifier.h"

#include <Application.h>
#include <FilePanel.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <MessageRunner.h>
#include <String.h>
#include <Window.h>

class RegisterView;

static const char kApplicationSignature[] = "application/x-vnd.wgp-CapitalBe";

// clang-format off
enum {
	M_FILE_NEW = 1000,
	M_FILE_OPEN,
	M_FILE_BACKUP,

	M_REPORT_BUG,
	M_FOCUS_FILTER,

	M_SHOW_NEW_ACCOUNT,
	M_IMPORT_ACCOUNT,
	M_SHOW_IMPORT_PANEL,
	M_SHOW_EXPORT_PANEL,
	M_EXPORT_ACCOUNT,
	M_DELETE_ACCOUNT,
	M_HIDE_ACCOUNT,
	M_SHOW_RECONCILE_WINDOW,
	M_SHOW_BUDGET_WINDOW,
	M_SHOW_CATEGORY_WINDOW,
	M_RECONCILE_ACCOUNT,
	M_PREVIOUS_ACCOUNT,
	M_NEXT_ACCOUNT,
	M_CLOSE_ACCOUNT,

	M_PREVIOUS_TRANSACTION,
	M_NEXT_TRANSACTION,
	M_FIRST_TRANSACTION,
	M_LAST_TRANSACTION,

	M_SPLIT_TRANSACTION,
	M_EDIT_TRANSACTION,
	M_DELETE_TRANSACTION,
	M_SCHEDULE_TRANSACTION,
	M_ENTER_TRANSFER,
	M_CONTEXT_CLOSE,
	M_USE_FOR_FILTER,
	M_USE_TRANSACTION,

	M_SHOW_REPORTS_WINDOW,
	M_SHOW_OPTIONS_WINDOW,
	M_SHOW_SCHEDULED_WINDOW,
	M_RUN_SCHEDULED_TRANSACTIONS,
	M_FILTER = 'filt',
	M_SET_FILTER,
};
// clang-format on

class MainWindow : public BWindow, public Observer {
public:
	MainWindow(BRect frame);
	~MainWindow();

	bool QuitRequested();
	void MessageReceived(BMessage* msg);

	void HandleNotify(const uint64& value, const BMessage* msg);
	void OpenAbout();

private:
	void _InitSettings();
	void _LoadData();
	void _SaveData();

	void _CreateTransfer(BMessage* msg);

	RegisterView* fRegisterView;
	BFilePanel *fImportPanel, *fExportPanel;

	BString fLastFile;
	BMenuItem* fAccountClosedItem;
	BMessageRunner* fRunner;

	bool fLoadError;
};

#endif
