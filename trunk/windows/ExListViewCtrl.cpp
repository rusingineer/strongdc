/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "ExListViewCtrl.h"

int ExListViewCtrl::moveItem(int oldPos, int newPos) {

	char buf[512];
	LVITEM lvi;
	lvi.iItem = oldPos;
	lvi.iSubItem = 0;
	lvi.mask = LVIF_IMAGE | LVIF_INDENT | LVIF_PARAM | LVIF_STATE;
	GetItem(&lvi);
	StringList l;

	for(int j = 0; j < GetHeader().GetItemCount(); j++) {
		GetItemText(oldPos, j, buf, 512);
		l.push_back(buf);
	}

	SetRedraw(FALSE);
	
	lvi.iItem = newPos;
	int i = InsertItem(&lvi);
	j = 0;
	for(StringIter k = l.begin(); k != l.end(); ++k, j++) {
		SetItemText(i, j, k->c_str());
	}
	
	if(i < oldPos)
		DeleteItem(oldPos + 1);
	else
		DeleteItem(oldPos);

	SetRedraw(TRUE);

	RECT rc;
	GetItemRect(i, &rc, LVIR_BOUNDS);
	InvalidateRect(&rc);
	
	return i;
}

int ExListViewCtrl::insert(StringList& aList, int iImage, LPARAM lParam) {

	char buf[128];
	int loc = 0;
	int count = GetItemCount();

	LVITEM a;
	a.mask = LVIF_IMAGE | LVIF_INDENT | LVIF_PARAM | LVIF_STATE | LVIF_TEXT;
	a.iItem = -1;
	a.iSubItem = sortColumn;
	a.iImage = iImage;
	a.state = 0;
	a.stateMask = 0;
	a.lParam = lParam;
	a.iIndent = 0;
	a.pszText = const_cast<char*>(sortColumn == -1 ? aList[0].c_str() : aList[sortColumn].c_str());
	a.cchTextMax = sortColumn == -1 ? aList[0].size() : aList[sortColumn].size();
	
	if(sortColumn == -1) {
		loc = count;
	} else if(count == 0) {
		loc = 0;
	} else {

		string& b = aList[sortColumn];
		int c = atoi(b.c_str());
		double f = atof(b.c_str());
		LPARAM data = NULL;			
		int low = 0;
		int high = count-1;
		int comp = 0;
		while(low <= high)
		{
			loc = (low + high)/2;
			
			// This is a trick, so that if fun() returns something bigger than one, use the
			// internal default sort functions
			comp = sortType;
			if(comp == SORT_FUNC) {
				data = GetItemData(loc);
				comp = fun(lParam, data);
			}
			
			if(comp == SORT_STRING) {
				GetItemText(loc, sortColumn, buf, 128);
				comp = compare(b, string(buf));
			} else if(comp == SORT_STRING_NOCASE) {
				GetItemText(loc, sortColumn, buf, 128);
				comp =  Util::stricmp(b.c_str(), buf);
			} else if(comp == SORT_INT) {
				GetItemText(loc, sortColumn, buf, 128);
				comp = compare(c, atoi(buf)); 
			} else if(comp == SORT_FLOAT) {
				GetItemText(loc, sortColumn, buf, 128);
				comp = compare(f, atof(buf));
			}
			
			if(!ascending)
				comp = -comp;

			if(comp == -1) {
				high = loc - 1;
			} else if(comp == 1) {
				low = loc + 1;
			} else {
				break;
			} 
		}

		comp = sortType;
		if(comp == SORT_FUNC) {
			comp = fun(lParam, data);
		}
		if(comp == SORT_STRING) {
			comp = compare(b, string(buf));
		} else if(comp == SORT_STRING_NOCASE) {
			comp =  Util::stricmp(b.c_str(), buf);
		} else if(comp == SORT_INT) {
			comp = compare(c, atoi(buf)); 
		} else if(comp == SORT_FLOAT) {
			comp = compare(f, atof(buf));
		}

		if(!ascending)
			comp = -comp;
		
		if(comp == 1)
			loc++;
	}
	dcassert(loc >= 0 && loc <= GetItemCount());
	a.iItem = loc;
	a.iSubItem = 0;
	int i = InsertItem(&a);
	int k = 0;
	for(StringIter j = aList.begin(); j != aList.end(); ++j, k++) {
		SetItemText(i, k, j->c_str());
	}
	return loc;
}

int ExListViewCtrl::insert(int nItem, StringList& aList, int iImage, LPARAM lParam) {

	dcassert(aList.size() > 0);

	int i = insert(nItem, aList[0], iImage, lParam);

	int k = 0;
	for(StringIter j = aList.begin(); j != aList.end(); ++j, k++) {
		SetItemText(i, k, j->c_str());
	}
	return i;
	
}

/**
 * @file
 * $Id$
 */
