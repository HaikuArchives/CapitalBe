/*
 * Copyright 2009. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 */
#ifndef IMPORT_H
#define IMPORT_H

#include <Entry.h>

bool ImportQIF(const entry_ref& ref);
bool ExportQIF(const entry_ref& ref);

#endif	// IMPORT_H
