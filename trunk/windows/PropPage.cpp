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

#include "PropPage.h"

#include "../client/SettingsManager.h"

#define SETTING_STR_MAXLEN 1024

void PropPage::read(HWND page, Item const* items, ListItem* listItems /* = NULL */, HWND list /* = 0 */)
{
#if DIM_EDIT_EXPERIMENT
	CDimEdit *tempCtrl;
#endif
	dcassert(page != NULL);

	bool const useDef = true;
	for(Item const* i = items; i->type != T_END; i++)
	{
		switch(i->type)
		{
		case T_STR:
#if DIM_EDIT_EXPERIMENT
			// worry about subclassing combo boxes
			// http://support.microsoft.com/default.aspx?scid=kb%3Ben-us%3B174667

			tempCtrl = new CDimEdit;
			//CWnd *foo = GetDlgItem(page, i->itemID);
			if (/*foo->IsKindOf(CEdit) &&*/ tempCtrl) {
				tempCtrl->SubclassWindow(GetDlgItem(page, i->itemID));
				tempCtrl->SetDimText(settings->get((SettingsManager::StrSetting)i->setting, true));
				tempCtrl->SetDimColor(RGB(192, 192, 192));
				ctrlMap[i->itemID] = tempCtrl;
			}
#endif
	/*		if (((SettingsManager::IntSetting)i->setting) != SettingsManager::CLIENTVERSION) {
				if (GetDlgItem(page, i->itemID) == NULL) {
					// Control not exist ? Why ??
					throw;
				}
				::SetDlgItemText(page, i->itemID,
					settings->get((SettingsManager::StrSetting)i->setting, useDef).c_str());
			} else*/ {
				if(!SettingsManager::getInstance()->isDefault(i->setting)) {
					::SetDlgItemText(page, i->itemID,
						settings->get((SettingsManager::StrSetting)i->setting, useDef).c_str());
				}
			}
			break;
		case T_INT:
			if (((SettingsManager::IntSetting)i->setting) != SettingsManager::IN_PORT) {
				if (GetDlgItem(page, i->itemID) == NULL) {
					// Control not exist ? Why ??
					throw;
				}
				::SetDlgItemInt(page, i->itemID,
					settings->get((SettingsManager::IntSetting)i->setting, useDef), FALSE);
			} else {
				if(!SettingsManager::getInstance()->isDefault(i->setting)) {
					::SetDlgItemInt(page, i->itemID,
						settings->get((SettingsManager::IntSetting)i->setting, useDef), FALSE);
				}
			}
			break;
		case T_INT64:
			if(!SettingsManager::getInstance()->isDefault(i->setting)) {
				string s = Util::toString(settings->get((SettingsManager::Int64Setting)i->setting, useDef));
				::SetDlgItemText(page, i->itemID, s.c_str());
			}
			break;
		case T_BOOL:
			if (GetDlgItem(page, i->itemID) == NULL) {
				// Control not exist ? Why ??
				throw;
			}
			if(settings->getBool((SettingsManager::IntSetting)i->setting, useDef))
				::CheckDlgButton(page, i->itemID, BST_CHECKED);
			else
				::CheckDlgButton(page, i->itemID, BST_UNCHECKED);
		}
	}

	if(listItems != NULL) {
		CListViewCtrl ctrl;

		ctrl.Attach(list);
		CRect rc;
		ctrl.GetClientRect(rc);
		ctrl.SetExtendedListViewStyle(LVS_EX_CHECKBOXES | (BOOLSETTING(FULL_ROW_SELECT) ? LVS_EX_FULLROWSELECT : 0));
		ctrl.InsertColumn(0, "Dummy", LVCFMT_LEFT, rc.Width(), 0);

		LVITEM lvi;
		lvi.mask = LVIF_TEXT;
		lvi.iSubItem = 0;

		for(int i = 0; listItems[i].setting != 0; i++) {
			lvi.iItem = i;
			lvi.pszText = const_cast<char*>(ResourceManager::getInstance()->getString(listItems[i].desc).c_str());
			ctrl.InsertItem(&lvi);
			ctrl.SetCheckState(i, SettingsManager::getInstance()->getBool(SettingsManager::IntSetting(listItems[i].setting), true));
		}
		ctrl.SetColumnWidth(0, LVSCW_AUTOSIZE);
		ctrl.Detach();
	}
}

void PropPage::write(HWND page, Item const* items, ListItem* listItems /* = NULL */, HWND list /* = NULL */)
{
	dcassert(page != NULL);

	char *buf = new char[SETTING_STR_MAXLEN];
	for(Item const* i = items; i->type != T_END; i++)
	{
		switch(i->type)
		{
		case T_STR:
			{
				if (GetDlgItem(page, i->itemID) == NULL) {
					// Control not exist ? Why ??
					throw;
				}
				::GetDlgItemText(page, i->itemID, buf, SETTING_STR_MAXLEN);
				settings->set((SettingsManager::StrSetting)i->setting, buf);
#if DIM_EDIT_EXPERIMENT
				if (ctrlMap[i->itemID]) {
					ctrlMap[i->itemID]->UnsubclassWindow();
					delete ctrlMap[i->itemID];
					ctrlMap.erase(i->itemID);
				}
#endif
				break;
			}
		case T_INT:
			{
				if (GetDlgItem(page, i->itemID) == NULL) {
					// Control not exist ? Why ??
					throw;
				}
				::GetDlgItemText(page, i->itemID, buf, SETTING_STR_MAXLEN);
				settings->set((SettingsManager::IntSetting)i->setting, string(buf));
				break;
			}
		case T_INT64:
			{
				::GetDlgItemText(page, i->itemID, buf, SETTING_STR_MAXLEN);
				settings->set((SettingsManager::Int64Setting)i->setting, string(buf));
				break;
			}
		case T_BOOL:
			{
				if (GetDlgItem(page, i->itemID) == NULL) {
					// Control not exist ? Why ??
					throw;
				}
				if(::IsDlgButtonChecked(page, i->itemID) == BST_CHECKED)
					settings->set((SettingsManager::IntSetting)i->setting, true);
				else
					settings->set((SettingsManager::IntSetting)i->setting, false);
			}
		}
	}
	delete[] buf;

	if(listItems != NULL) {
		CListViewCtrl ctrl;
		ctrl.Attach(list);

		int i;
		for(i = 0; listItems[i].setting != 0; i++) {
			SettingsManager::getInstance()->set(SettingsManager::IntSetting(listItems[i].setting), ctrl.GetCheckState(i));
		}

		ctrl.Detach();
	}
}

void PropPage::translate(HWND page, TextItem* textItems) 
{
	if (textItems != NULL) {
		for(int i = 0; textItems[i].itemID != 0; i++) {
			::SetDlgItemText(page, textItems[i].itemID,
				ResourceManager::getInstance()->getString(textItems[i].translatedString).c_str());
		}
	}
}

/**
 * @file
 * $Id$
 */

