/* 
 * Copyright (C) 2001-2004 Jacek Sieka, j_s at telia com
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

#include "PropertiesDlg.h"

#include "GeneralPage.h"
#include "DownloadPage.h"
#include "UploadPage.h"
#include "AppearancePage.h"
#include "AdvancedPage.h"
#include "Advanced2Page.h"
#include "Sounds.h"
#include "Segment.h"
#include "UCPage.h"
#include "CZDCPage.h"
#include "LimitPage.h"
#include "PropPageTextStyles.h"
#include "FakeDetect.h"
#include "AVIPreview.h"
#include "OperaColorsPage.h"
#include "ClientsPage.h"
#include "ToolbarPage.h"
#include "DownloadDirsPage.h"
#include "Popups.h"
#include "SDCPage.h"
#include "UserListColours.h"

bool PropertiesDlg::needUpdate = false;
PropertiesDlg::PropertiesDlg(SettingsManager *s) : TreePropertySheet(CTSTRING(SETTINGS)) {
	pages[0] = new GeneralPage(s);
	pages[1] = new UploadPage(s);
	pages[2] = new DownloadPage(s);	
	pages[3] = new AppearancePage(s);
	pages[4] = new AdvancedPage(s);
	pages[5] = new Advanced2Page(s);
	pages[6] = new UCPage(s);
	pages[7] = new CZDCPage(s);
	pages[8] = new SDCPage(s);
	pages[9] = new DownloadDirsPage(s);
	pages[10] = new Popups(s);
	pages[11] = new PropPageTextStyles(s);
	pages[12] = new OperaColorsPage(s);
	pages[13] = new FakeDetect(s);	
	pages[14] = new LimitPage(s);
	pages[15] = new AVIPreview(s);	
	pages[16] = new Segment(s);
	pages[17] = new Sounds(s);
	pages[18] = new ToolbarPage(s);
	pages[19] = new ClientsPage(s);	
	pages[20] = new UserListColours(s);	

	for(int i=0; i<numPages; i++) {
		AddPage(pages[i]->getPSP());
	}

	// Hide "Apply" button
	m_psh.dwFlags |= PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
	m_psh.dwFlags &= ~PSH_HASHELP;
}

PropertiesDlg::~PropertiesDlg() {
	for(int i=0; i<numPages; i++) {
		delete pages[i];
	}
}

void PropertiesDlg::write() {
	for(int i=0; i<numPages; i++)
	{
		// Check HWND of page to see if it has been created
		const HWND page = PropSheet_IndexToHwnd((HWND)*this, i);

		if(page != NULL)
			pages[i]->write();	
	}
}

LRESULT PropertiesDlg::onOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	write();
	bHandled = FALSE;
	return TRUE;
}

/**
 * @file
 * $Id$
 */
