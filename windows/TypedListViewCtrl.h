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

#if !defined(AFX_TYPEDLISTVIEWCTRL_H__45847002_68C2_4C8A_9C2D_C4D8F65DA841__INCLUDED_)
#define AFX_TYPEDLISTVIEWCTRL_H__45847002_68C2_4C8A_9C2D_C4D8F65DA841__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../Client/SettingsManager.h"
#include "../client/HubManager.h"
#include "ListViewArrows.h"
class ColumnInfo {
public:
	ColumnInfo(const tstring &aName, int aPos, int aFormat, int aWidth): name(aName), pos(aPos), width(aWidth), 
		format(aFormat), visible(true) {}
		tstring name;
		bool visible;
		int pos;
		int width;
		int format;
};

template<class T, int ctrlId>
class TypedListViewCtrl : public CWindowImpl<TypedListViewCtrl, CListViewCtrl, CControlWinTraits>,
	ListViewArrows<TypedListViewCtrl<T, ctrlId> >
{
public:
	TypedListViewCtrl() : sortColumn(-1), sortAscending(true), hBrBg(WinUtil::bgBrush), leftMargin(0) { };
	~TypedListViewCtrl() {
		for(ColumnIter i = columnList.begin(); i != columnList.end(); ++i){
			delete (*i);
		}
	}

	typedef TypedListViewCtrl<T, ctrlId> thisClass;
	typedef CListViewCtrl baseClass;
	typedef ListViewArrows<thisClass> arrowBase;

	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBackground)
		MESSAGE_HANDLER(WM_MENUCOMMAND, onHeaderMenu)
		MESSAGE_HANDLER(WM_KEYDOWN, onChar)
		CHAIN_MSG_MAP(arrowBase)
	END_MSG_MAP();

	class iterator : public ::iterator<random_access_iterator_tag, T*> {
	public:
		iterator() : typedList(NULL), cur(0), cnt(0) { }
		iterator(const iterator& rhs) : typedList(rhs.typedList), cur(rhs.cur), cnt(rhs.cnt) { }
		iterator& operator=(const iterator& rhs) { typedList = rhs.typedList; cur = rhs.cur; cnt = rhs.cnt; return *this; }

		bool operator==(const iterator& rhs) const { return cur == rhs.cur; }
		bool operator!=(const iterator& rhs) const { return !(*this == rhs); }
		bool operator<(const iterator& rhs) const { return cur < rhs.cur; }

		int operator-(const iterator& rhs) const { 
			return cur - rhs.cur;
		}

		iterator& operator+=(int n) { cur += n; return *this; };
		iterator& operator-=(int n) { return (cur += -n); };
		
		T& operator*() { return *typedList->getItemData(cur); }
		T* operator->() { return &(*(*this)); }
		T& operator[](int n) { return *typedList->getItemData(cur + n); }
		
		iterator operator++(int) {
			iterator tmp(*this);
			operator++();
			return tmp;
		}
		iterator& operator++() {
			++cur;
			return *this;
		}

	private:
		iterator(thisClass* aTypedList) : typedList(aTypedList), cur(aTypedList->GetNextItem(-1, LVNI_ALL)), cnt(aTypedList->GetItemCount()) { 
			if(cur == -1)
				cur = cnt;
		}
		iterator(thisClass* aTypedList, int first) : typedList(aTypedList), cur(first), cnt(aTypedList->GetItemCount()) { 
			if(cur == -1)
				cur = cnt;
		}
		friend class thisClass;
		thisClass* typedList;
		int cur;
		int cnt;
	};

	LRESULT onGetDispInfo(int /* idCtrl */, LPNMHDR pnmh, BOOL& /* bHandled */) {
		NMLVDISPINFO* di = (NMLVDISPINFO*)pnmh;
		if(di->item.mask & LVIF_TEXT) {
			di->item.mask |= LVIF_DI_SETITEM;
			int pos = di->item.iSubItem;
			bool insert = false;
			int j = 0;
			TCHAR *buf = new TCHAR[512];
			LVCOLUMN lvc;
			lvc.mask = LVCF_TEXT;
			lvc.pszText = buf;
			lvc.cchTextMax = 512;
			GetColumn(pos, &lvc);
			if(columnList.size() > 0){
				for(ColumnIter i = columnList.begin(); i != columnList.end(); ++i, ++j){
					if((Util::stricmp(buf, (*i)->name.c_str()) == 0)){
						if((*i)->visible == true)
							insert = true;
						break;
					}
				}
			} else {
				insert = true;
			}
			if(insert == true)
				di->item.pszText = const_cast<TCHAR*>(((T*)di->item.lParam)->getText(j).c_str());

			delete[] buf;
		}
		return 0;
	}
	
	LRESULT onInfoTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVGETINFOTIP* pInfoTip = (NMLVGETINFOTIP*) pnmh;
		BOOL NoColumnHeader = (BOOL)(GetWindowLong(GWL_STYLE) & LVS_NOCOLUMNHEADER);
		tstring InfoTip(_T(""));
		TCHAR Buffer[300];
		LV_COLUMN lvCol;
		LVITEM lvItem;
		int indexes[32];
		GetColumnOrderArray(GetHeader().GetItemCount(), indexes);
		for (int i = 0; i < GetHeader().GetItemCount(); ++i)
		{
			if (!NoColumnHeader) {
				lvCol.mask = LVCF_TEXT;
				lvCol.pszText = Buffer;
				lvCol.cchTextMax = sizeof(Buffer);
				GetColumn(indexes[i], &lvCol);
				InfoTip += lvCol.pszText;
				InfoTip += _T(": ");
			}
			lvItem.iItem = pInfoTip->iItem /*nItem*/;
			GetItemText(pInfoTip->iItem /*nItem*/, indexes[i], Buffer, 300);
			Buffer[299] = NULL;

			InfoTip += Buffer;
			InfoTip += _T("\r\n");
		}

		if (InfoTip.size() > 2)
			InfoTip.erase(InfoTip.size() - 2);
		
		pInfoTip->cchTextMax = InfoTip.size();

 		_tcsncpy(pInfoTip->pszText, InfoTip.c_str(), INFOTIPSIZE);
		pInfoTip->pszText[INFOTIPSIZE - 1] = NULL;

		return 0;
	}

	// Sorting
	LRESULT onColumnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLISTVIEW* l = (NMLISTVIEW*)pnmh;
		if(l->iSubItem != sortColumn) {
			sortAscending = true;
			sortColumn = l->iSubItem;
		} else if(sortAscending) {
			sortAscending = false;
		} else {
			sortColumn = -1;
		}
		updateArrow();
		resort();
		return 0;
	}
	void resort() {
		if(sortColumn != -1) {
			SortItems(&compareFunc, (LPARAM)this);
		}
	}

	int insertItem(T* item, int image) {
		return insertItem(getSortPos(item), item, image);
	}
	int insertItem(int i, T* item, int image) {
		return InsertItem(LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE, i, 
			LPSTR_TEXTCALLBACK, 0, 0, image, (LPARAM)item);
	}
	T* getItemData(int iItem) { return (T*)GetItemData(iItem); }
	T* getSelectedItem() { return (GetSelectedCount() > 0 ? getItemData(GetNextItem(-1, LVNI_SELECTED)) : NULL); }

	int findItem(T* item) { 
		LVFINDINFO fi = { LVFI_PARAM, NULL, (LPARAM)item };
		return FindItem(&fi, -1);
	}
	struct CompFirst {
		CompFirst() { } 
		bool operator()(T& a, const tstring& b) {
			return Util::stricmp(a.getText(0), b) < 0;
		}
	};
	int findItem(const tstring& b, int start = -1, bool aPartial = false) {
		LVFINDINFO fi = { aPartial ? LVFI_PARTIAL : LVFI_STRING, b.c_str() };
		return FindItem(&fi, start);
	}

	void forEach(void (T::*func)()) {
		int n = GetItemCount();
		for(int i = 0; i < n; ++i)
			(getItemData(i)->*func)();
	}
	void forEachSelected(void (T::*func)()) {
		int i = -1;
		while( (i = GetNextItem(i, LVNI_SELECTED)) != -1)
			(getItemData(i)->*func)();
	}
	template<class _Function>
	_Function forEachT(_Function pred) {
		int n = GetItemCount();
		for(int i = 0; i < n; ++i)
			pred(getItemData(i));
		return pred;
	}
	template<class _Function>
	_Function forEachSelectedT(_Function pred) {
		int i = -1;
		while( (i = GetNextItem(i, LVNI_SELECTED)) != -1)
			pred(getItemData(i));
		return pred;
	}

	void forEachAtPos(int iIndex, void (T::*func)()) {
		(getItemData(iIndex)->*func)();
	}
	template<class _Function>
	_Function forEachAtPosT(int iIndex, _Function pred) {
		pred(getItemData(iIndex));
		return pred;
	}

	void updateItem(int i) {
		int k = GetHeader().GetItemCount();
		for(int j = 0; j < k; ++j)
			SetItemText(i, j, LPSTR_TEXTCALLBACK);
	}
	void updateItem(T* item) { int i = findItem(item); if(i != -1) updateItem(i); };
	void deleteItem(T* item) { int i = findItem(item); if(i != -1) DeleteItem(i); };

	int getSortPos(T* a) {
		int high = GetItemCount();
		if((sortColumn == -1) || (high == 0))
			return high;

		high--;

		int low = 0;
		int mid = 0;
		T* b = NULL;
		int comp = 0;
		while( low <= high ) {
			mid = (low + high) / 2;
			b = getItemData(mid);
			comp = T::compareItems(a, b, sortColumn);
			
			if(!sortAscending)
				comp = -comp;

			if(comp == 0) {
				return mid;
			} else if(comp < 0) {
				high = mid - 1;
			} else if(comp > 0) {
					low = mid + 1;
			} else if(comp == 2){
				if(sortAscending)
					low = mid + 1;
				else
					high = mid -1;
			} else if(comp == -2){
				if(!sortAscending)
					low = mid + 1;
				else
					high = mid -1;
			}
		}

		comp = T::compareItems(a, b, sortColumn);
		if(!sortAscending)
			comp = -comp;
		if(comp > 0)
			mid++;

		return mid;
	}

	void setSortColumn(int aSortColumn) {
		sortColumn = aSortColumn;
		updateArrow();
	}
	int getSortColumn() { return sortColumn; }
	int getRealSortColumn() { return findColumn(sortColumn); }
	bool isAscending() { return sortAscending; }
	void setAscending(bool s) {sortAscending = s;}

	iterator begin() { return iterator(this); }
	iterator end() { return iterator(this, GetItemCount()); }

	void setLeftEraseBackgroundMargin(int _leftMargin) {
		leftMargin = _leftMargin;
	}

	int insertColumn(int nCol, const tstring &columnHeading, int nFormat = LVCFMT_LEFT, int nWidth = -1, int nSubItem = -1 ){
		if(nWidth == 0)
			nWidth = 80;
		columnList.push_back(new ColumnInfo(columnHeading, nCol, nFormat, nWidth));
		return CListViewCtrl::InsertColumn(nCol, columnHeading.c_str(), nFormat, nWidth, nSubItem);
	}

	void showMenu(POINT &pt){
		headerMenu.DestroyMenu();
		headerMenu.CreatePopupMenu();
		MENUINFO inf;
		inf.cbSize = sizeof(MENUINFO);
		inf.fMask = MIM_STYLE;
		inf.dwStyle = MNS_NOTIFYBYPOS;
		headerMenu.SetMenuInfo(&inf);

		int j = 0;
		for(ColumnIter i = columnList.begin(); i != columnList.end(); ++i, ++j) {
			headerMenu.AppendMenu(MF_STRING, IDC_HEADER_MENU, (*i)->name.c_str());
			if((*i)->visible)
				headerMenu.CheckMenuItem(j, MF_BYPOSITION | MF_CHECKED);
		}
		headerMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	}
		
	LRESULT onEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		if(!leftMargin || !hBrBg) 
			return 0;

		dcassert(hBrBg);
		if(!hBrBg) return 0;

		bHandled = TRUE;
		HDC dc = (HDC)wParam;
		int n = GetItemCount();
		RECT r = {0, 0, 0, 0}, full;
		GetClientRect(&full);



		if (n > 0) {
			GetItemRect(0, &r, LVIR_BOUNDS);
			r.bottom = r.top + ((r.bottom - r.top) * n);
		}

		RECT full2 = full; // Keep a backup


		full.bottom = r.top;
		FillRect(dc, &full, hBrBg);

		full = full2; // Revert from backup
		full.right = r.left + leftMargin; // state image
		//full.left = 0;
		FillRect(dc, &full, hBrBg);

		full = full2; // Revert from backup
		full.left = r.right;
		FillRect(dc, &full, hBrBg);

		full = full2; // Revert from backup
		full.top = r.bottom;
		full.right = r.right;
		FillRect(dc, &full, hBrBg);

		
		return S_OK;
	}
	void setFlickerFree(HBRUSH flickerBrush) {
		hBrBg = flickerBrush;
	}
	
	LRESULT onHeaderMenu(UINT /*msg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ColumnInfo * ci = columnList[wParam];
		ci->visible = ! ci->visible;

		SetRedraw(FALSE);

		if(!ci->visible){
			removeColumn(ci);
		} else {
			if(ci->width == 0)
				ci->width = 80;
			ci->pos = static_cast<int>(wParam);
			CListViewCtrl::InsertColumn(ci->pos, ci->name.c_str(), ci->format, ci->width, static_cast<int>(wParam));
			LVCOLUMN lvcl = { 0 };
			lvcl.mask = LVCF_ORDER;
			lvcl.iOrder = ci->pos;
			SetColumn(ci->pos, &lvcl);
			for(int i = 0; i < GetItemCount(); i++) {
				LVITEM lvItem;
				lvItem.iItem = i;
				lvItem.iSubItem = 0;
				lvItem.mask = LVIF_IMAGE | LVIF_PARAM;
				GetItem(&lvItem);
				lvItem.iImage = ((T*)lvItem.lParam)->imageIndex();
				SetItem(&lvItem);
				updateItem(i);
			}
		}

		SetRedraw();
		Invalidate();
		UpdateWindow();

		return 0;
	}

	void saveHeaderOrder(SettingsManager::StrSetting order, SettingsManager::StrSetting widths, 
		SettingsManager::StrSetting visible) throw() {
		string tmp, tmp2, tmp3;
		TCHAR *buf = new TCHAR[128];
		int size = GetHeader().GetItemCount();
		for(int i = 0; i < size; ++i){
			LVCOLUMN lvc;
			lvc.mask = LVCF_TEXT | LVCF_ORDER | LVCF_WIDTH;
			lvc.cchTextMax = 128;
			lvc.pszText = buf;
			GetColumn(i, &lvc);
			for(ColumnIter j = columnList.begin(); j != columnList.end(); ++j){
				if(Util::stricmp(buf, (*j)->name.c_str()) == 0){
					(*j)->pos = lvc.iOrder;
					(*j)->width = lvc.cx;
				}
			}
		}

		for(ColumnIter i = columnList.begin(); i != columnList.end(); ++i){
			ColumnInfo* ci = *i;

			if(ci->visible){
				tmp3 += "1,";
			} else {
				ci->pos = size++;
				tmp3 += "0,";
			}

			tmp += Util::toString(ci->pos);
			tmp += ',';

			tmp2 += Util::toString(ci->width);
			tmp2 += ',';
		}

		tmp.erase(tmp.size()-1, 1);
		tmp2.erase(tmp2.size()-1, 1);
		tmp3.erase(tmp3.size()-1, 1);
		SettingsManager::getInstance()->set(order, tmp);
		SettingsManager::getInstance()->set(widths, tmp2);
		SettingsManager::getInstance()->set(visible, tmp3);
	}

	void saveFavoriteHeaderOrder(FavoriteHubEntry* Entry) throw() {
		string tmp, tmp2, tmp3;
		TCHAR *buf = new TCHAR[128];
		int size = GetHeader().GetItemCount();
		for(int i = 0; i < size; ++i){
			LVCOLUMN lvc;
			lvc.mask = LVCF_TEXT | LVCF_ORDER | LVCF_WIDTH;
			lvc.cchTextMax = 128;
			lvc.pszText = buf;
			GetColumn(i, &lvc);
			for(ColumnIter j = columnList.begin(); j != columnList.end(); ++j){
				if(Util::stricmp(buf, (*j)->name.c_str()) == 0){
					(*j)->pos = lvc.iOrder;
					(*j)->width = lvc.cx;
				}
			}
		}

		for(ColumnIter i = columnList.begin(); i != columnList.end(); ++i){
			ColumnInfo* ci = *i;

			if(ci->visible){
				tmp3 += "1,";
			} else {
				ci->pos = size++;
				tmp3 += "0,";
			}

			tmp += Util::toString(ci->pos);
			tmp += ',';

			tmp2 += Util::toString(ci->width);
			tmp2 += ',';
		}

		tmp.erase(tmp.size()-1, 1);
		tmp2.erase(tmp2.size()-1, 1);
		tmp3.erase(tmp3.size()-1, 1);
		Entry->setColumsOrder(tmp);
		Entry->setColumsWidth(tmp2);
		Entry->setColumsVisible(tmp3);
		HubManager::getInstance()->save();
	}
	
	void setVisible(string vis){
		StringTokenizer<string> tok(vis, ',');
		StringList l = tok.getTokens();

		StringIter i = l.begin();
		ColumnIter j = columnList.begin();
		for(; j != columnList.end() && i != l.end(); ++i, ++j){

			if(Util::toInt(*i) == 0){
				(*j)->visible = false;
				removeColumn(*j);
			}
		}
	}

	void setColumnOrderArray(int iCount, LPINT piArray ) {
		LVCOLUMN lvc;
		lvc.mask = LVCF_ORDER;
		for(int i = 0; i < iCount; ++i) {
			lvc.iOrder = columnList[i]->pos = piArray[i];
			SetColumn(i, &lvc);
		}
	}

	LRESULT onChar(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		if((GetKeyState(VkKeyScan('A') & 0xFF) & 0xFF00) > 0 && (GetKeyState(VK_CONTROL) & 0xFF00) > 0){
			int count = GetItemCount();
			for(int i = 0; i < count; ++i)
				ListView_SetItemState(m_hWnd, i, LVIS_SELECTED, LVIS_SELECTED);

			return 0;
		}
		
		bHandled = FALSE;
		return 1;
	}

	//find the current position for the column that was inserted at the specified pos
	int findColumn(int col){
		TCHAR *buf = new TCHAR[512];
		LVCOLUMN lvcl;
		lvcl.mask = LVCF_TEXT;
		lvcl.pszText = buf;
		lvcl.cchTextMax = 512;

		GetColumn(col, &lvcl);

		int result = -1;

		int i = 0;
		for(ColumnIter j = columnList.begin(); j != columnList.end(); ++i, ++j){
			if(Util::stricmp((*j)->name.c_str(), buf) == 0){
				result = i;
				break;
			}
		}

		delete[] buf;

		return result;
	}	
	
private:
	CMenu headerMenu;


	int sortColumn;
	bool sortAscending;
	int leftMargin;
	HBRUSH hBrBg;

	static int CALLBACK compareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
		thisClass* t = (thisClass*)lParamSort;
		int result = T::compareItems((T*)lParam1, (T*)lParam2, t->getRealSortColumn());

		if(result == 2)
			result = (t->sortAscending ? 1 : -1);
		else if(result == -2)
			result = (t->sortAscending ? -1 : 1);

		return (t->sortAscending ? result : -result);
	}

	typedef vector< ColumnInfo* > ColumnList;
	typedef ColumnList::iterator ColumnIter;

	ColumnList columnList;

	void removeColumn(ColumnInfo* ci){
		
		int column = findColumn(ci);

		if(column > -1){
			ci->width = GetColumnWidth(column);

			HDITEM hd;
			hd.mask = HDI_ORDER;
			GetHeader().GetItem(column, &hd);
			ci->pos = hd.iOrder;

			int itemCount = GetHeader().GetItemCount();
			if(itemCount >= 0 && sortColumn > itemCount - 2)
				setSortColumn(0);

			if(sortColumn == ci->pos)
				setSortColumn(0);

			DeleteColumn(column);

			for(int i = 0; i < GetItemCount(); i++) {
				LVITEM lvItem;
				lvItem.iItem = i;
				lvItem.iSubItem = 0;
				lvItem.mask = LVIF_PARAM | LVIF_IMAGE;
				GetItem(&lvItem);
				lvItem.iImage = ((T*)lvItem.lParam)->imageIndex();
				SetItem(&lvItem);
			}

		}		
	}

	int findColumn(ColumnInfo* ci){
		TCHAR *buf = new TCHAR[512];
		LVCOLUMN lvcl;
		lvcl.mask = LVCF_TEXT;
		lvcl.pszText = buf;
		lvcl.cchTextMax = 512;

		int columns = GetHeader().GetItemCount();

		int result = -1;

		for(int k = 0; k < columns; ++k){

			GetColumn(k, &lvcl);
			if(Util::stricmp(ci->name.c_str(), lvcl.pszText) == 0){
				result = k;
				break;
			}
		}

		delete[] buf;

		return result;
	}
};

#endif

/**
* @file
* $Id$
*/
