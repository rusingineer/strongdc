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

#if !defined(AFX_TYPEDLISTVIEWCTRL_H__45847002_68C2_4C8A_9C2D_C4D8F65DA841__INCLUDED_)
#define AFX_TYPEDLISTVIEWCTRL_H__45847002_68C2_4C8A_9C2D_C4D8F65DA841__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ListViewArrows.h"

#ifndef AGARRAYTEMPLATES_H__
	#include "AGArrayTemplates.h"
#endif

template<class T, int ctrlId>
class TypedListViewCtrl : public CWindowImpl<TypedListViewCtrl, CListViewCtrl, CControlWinTraits>,
	ListViewArrows<TypedListViewCtrl<T, ctrlId> >
{
public:
	TypedListViewCtrl() : sortColumn(-1), sortAscending(true), hBrBg(NULL) { };

	typedef TypedListViewCtrl<T, ctrlId> thisClass;
	typedef CListViewCtrl baseClass;
	typedef ListViewArrows<thisClass> arrowBase;

	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBackground)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_GETINFOTIP, onInfoTip)
		DEFAULT_REFLECTION_HANDLER()
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

	LRESULT onEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		if (!hBrBg) {
			bHandled = FALSE;
			return S_OK;
		}
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

		full.right = 3;
		full.left = 0;
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

	LRESULT onGetDispInfo(int /* idCtrl */, LPNMHDR pnmh, BOOL& /* bHandled */) {
		NMLVDISPINFO* di = (NMLVDISPINFO*)pnmh;
		if(di->item.mask & LVIF_TEXT) {
			di->item.mask |= LVIF_DI_SETITEM;
			di->item.pszText = const_cast<char*>(((T*)di->item.lParam)->getText(di->item.iSubItem).c_str());
		}
		return 0;
	}

	LRESULT onInfoTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVGETINFOTIP* pInfoTip = (NMLVGETINFOTIP*) pnmh;
		BOOL NoColumnHeader = (BOOL)(GetWindowLong(GWL_STYLE) & LVS_NOCOLUMNHEADER);
		string InfoTip("");
		char Buffer[500];
		LV_COLUMN lvCol;
		LVITEM lvItem;

		for (int i = 0; i < GetHeader().GetItemCount(); ++i)
		{
			if (!NoColumnHeader) {
				lvCol.mask = LVCF_TEXT;
				lvCol.pszText = Buffer;
				lvCol.cchTextMax = sizeof(Buffer);

				GetColumn(i, &lvCol);
				InfoTip += lvCol.pszText;
				InfoTip += ": ";
			}
			lvItem.iItem = pInfoTip->iItem /*nItem*/;
			GetItemText(pInfoTip->iItem /*nItem*/, i, Buffer, 512);
			InfoTip += Buffer;
			InfoTip += "\r\n";
		}

		if (InfoTip.size() > 2)
			InfoTip.erase(InfoTip.size() - 2);

		strcpy(pInfoTip->pszText, InfoTip.c_str());
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
		bool operator()(T& a, const string& b) {
			return Util::stricmp(a.getText(0), b) == -1;
		}
	};
	int findItem(const string& b, int start = -1, bool aPartial = false) {
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
			} else if(comp == -1) {
				high = mid - 1;
			} else if(comp == 1) {
				low = mid + 1;
			}
		}

		comp = T::compareItems(a, b, sortColumn);
		if(!sortAscending)
			comp = -comp;
		if(comp == 1)
			mid++;

		return mid;
	}

	void setSortColumn(int aSortColumn) {
		sortColumn = aSortColumn;
		updateArrow();
	}
	int getSortColumn() { return sortColumn; }
	bool isAscending() { return sortAscending; }

	iterator begin() { return iterator(this); }
	iterator end() { return iterator(this, GetItemCount()); }

private:

	int sortColumn;
	bool sortAscending;
	HBRUSH hBrBg;

	static int CALLBACK compareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
		thisClass* t = (thisClass*)lParamSort;
		int result = T::compareItems((T*)lParam1, (T*)lParam2, t->sortColumn);
		return (t->sortAscending ? result : -result);
	}
};

template<class T, int ctrlId>
class TypedListViewCtrlCleanup : public CWindowImpl<TypedListViewCtrlCleanup, CListViewCtrl, CControlWinTraits>,
	ListViewArrows<TypedListViewCtrlCleanup<T, ctrlId> >
{
public:
	TypedListViewCtrlCleanup() : sortColumn(-1), sortAscending(true), hBrBg(NULL) { };

	typedef TypedListViewCtrlCleanup<T, ctrlId> thisClass;
	typedef CListViewCtrl baseClass;
	typedef ListViewArrows<thisClass> arrowBase;

	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBackground)
		REFLECTED_NOTIFY_CODE_HANDLER(LVN_GETINFOTIP, onInfoTip)
		DEFAULT_REFLECTION_HANDLER()
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

	LRESULT onEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		if (!hBrBg) {
			bHandled = FALSE;
			return S_OK;
		}
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

		full.right = 3;
		full.left = 0;
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

	LRESULT onGetDispInfo(int /* idCtrl */, LPNMHDR pnmh, BOOL& /* bHandled */) {
		NMLVDISPINFO* di = (NMLVDISPINFO*)pnmh;
		if(di->item.mask & LVIF_TEXT) {
			di->item.mask |= LVIF_DI_SETITEM;
			// di->item.pszText = const_cast<char*>(((T*)di->item.lParam)->getText(di->item.iSubItem).c_str());
			T* pItem = ArrayGetItemAt((int) di->item.lParam);
			dcassert(pItem != NULL);
			di->item.pszText = const_cast<char*>(pItem->getText(di->item.iSubItem).c_str());
		}
		return 0;
	}

	LRESULT onInfoTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVGETINFOTIP* pInfoTip = (NMLVGETINFOTIP*) pnmh;
		BOOL NoColumnHeader = (BOOL)(GetWindowLong(GWL_STYLE) & LVS_NOCOLUMNHEADER);
		string InfoTip("");
		char Buffer[500];
		LV_COLUMN lvCol;
		LVITEM lvItem;

		for (int i = 0; i < GetHeader().GetItemCount(); ++i)
		{
			if (!NoColumnHeader) {
				lvCol.mask = LVCF_TEXT;
				lvCol.pszText = Buffer;
				lvCol.cchTextMax = sizeof(Buffer);

				GetColumn(i, &lvCol);
				InfoTip += lvCol.pszText;
				InfoTip += ": ";
			}
			lvItem.iItem = pInfoTip->iItem /*nItem*/;
			GetItemText(pInfoTip->iItem /*nItem*/, i, Buffer, 512);
			InfoTip += Buffer;
			InfoTip += "\r\n";
		}

		if (InfoTip.size() > 2)
			InfoTip.erase(InfoTip.size() - 2);

		strcpy(pInfoTip->pszText, InfoTip.c_str());
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
		int nFound = ArrayFindItemIdx(item);
		int nDataPos = -1;
		if (nFound > -1) {
			nDataPos = nFound;
		} else nDataPos = ArrayInsertItem(item);
		return InsertItem(LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE, i, 
			LPSTR_TEXTCALLBACK, 0, 0, image, (LPARAM)nDataPos);
	}
	T* getItemData(int iItem) 
	{ 
		int nDataPos = GetItemData(iItem); 
		return ArrayGetItemAt(nDataPos);
	}

	T* getSelectedItem() { return (GetSelectedCount() > 0 ? getItemData(GetNextItem(-1, LVNI_SELECTED)) : NULL); }

	T* getStoredItemAt(int nDataPos) 
	{ 
		return ArrayGetItemAt(nDataPos);
	}

	void PrepareForSize(int nNewSize) 
	{ 
		m_ItemsPtrArray.SetAllocSize(nNewSize);
	}

	int findItem(T* item) { 
		int nDataPos = ArrayFindItemIdx(item); 
		LVFINDINFO fi = { LVFI_PARAM, NULL, (LPARAM)nDataPos};
		return FindItem(&fi, -1);
	}
	struct CompFirst {
		CompFirst() { } 
		bool operator()(T& a, const string& b) {
			return Util::stricmp(a.getText(0), b) == -1;
		}
	};
	int findItem(const string& b, int start = -1, bool aPartial = false) {
		LVFINDINFO fi = { aPartial ? LVFI_PARTIAL : LVFI_STRING, b.c_str() };
		return FindItem(&fi, start);
	}

	void forEach(void (T::*func)()) {
		int n = ArrayGetSize();
		for(int i = 0; i < n; ++i)
			(ArrayGetItemAt(i)->*func)();
	}
	void forEachSelected(void (T::*func)()) {
		int i = -1;
		while( (i = GetNextItem(i, LVNI_SELECTED)) != -1)
			(getItemData(i)->*func)();
	}
	template<class _Function>
	_Function forEachT(_Function pred) {
		int n = ArrayGetSize();
		for(int i = 0; i < n; ++i)
			pred(ArrayGetItemAt(i));
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
	void deleteItem(T* item, bool memFree = true) 
	{ 
		int i = findItem(item); 
		if(i != -1) DeleteItem(i, memFree); else {
			if(memFree) delete item;
		}
	}

	void deleteItem(int nListPos, bool memFree = true) 
	{ 
		DeleteItem(nListPos, memFree); 
	}

	void DeleteAll()
	{
		DeleteAllItems();
	}

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
			} else if(comp == -1) {
				high = mid - 1;
			} else if(comp == 1) {
				low = mid + 1;
			}
		}

		comp = T::compareItems(a, b, sortColumn);
		if(!sortAscending)
			comp = -comp;
		if(comp == 1)
			mid++;

		return mid;
	}

	void setSortColumn(int aSortColumn) {
		sortColumn = aSortColumn;
		updateArrow();
	}
	int getSortColumn() { return sortColumn; }
	bool isAscending() { return sortAscending; }

	iterator begin() { return iterator(this); }
	iterator end() { return iterator(this, GetItemCount()); }

protected:
	// Zakazeme pouzivani techto metod zvenku
	BOOL DeleteItem(int nItem, bool memFree)
	{
		int nDataPos = GetItemData(nItem);
		BOOL bRet = baseClass::DeleteItem(nItem);
		if(memFree) ArrayDeleteItemAt(nDataPos);
		return bRet;
	}

	BOOL DeleteAllItems()
	{
		ArrayRemoveAll();
		return baseClass::DeleteAllItems();
	}

	DWORD_PTR GetItemData(int nItem) const
	{
		return baseClass::GetItemData(nItem);
	}
	BOOL SetItemData(int nItem, DWORD_PTR dwData)
	{
		return baseClass::SetItemData(nItem, dwData);
	}

	// Internal array methods

	int ArrayInsertItem(T* item)
	{
#ifdef _DEBUG
		int nFound = ArrayFindItemIdx(item);
		if (nFound > -1) {
			// Vicenasobne pouziti jedne instance ? Na tohle nejsme zarizeni !
//			dcassert(FALSE);
			return -1;
		}
#endif
		return m_ItemsPtrArray.AddSpeedy(item);
	}

	T* ArrayGetItemAt(int nDataPos)
	{
		dcassert(nDataPos > -1 && nDataPos < m_ItemsPtrArray.GetSize());
		return m_ItemsPtrArray.GetAt(nDataPos);
	}

	int ArrayFindItemIdx(T* item)
	{
		for (int i = 0; i < m_ItemsPtrArray.GetSize(); i++){
			if (m_ItemsPtrArray.ElementAt(i) == item)
				return i;
		}
		return -1;
	}

	int ArrayGetSize()
	{
		return m_ItemsPtrArray.GetSize();
	}

	void ArrayDeleteItemAt(int nDataPos)
	{
		dcassert(nDataPos > -1 && nDataPos < m_ItemsPtrArray.GetSize());
		T* pItem = m_ItemsPtrArray.GetAt(nDataPos);
		if (pItem != NULL) {
			delete pItem;
		}
		m_ItemsPtrArray.SetAt(nDataPos, NULL);
	}

	void ArrayRemoveAll()
	{
		m_ItemsPtrArray.RemoveAll();
	}

private:

	int sortColumn;
	bool sortAscending;
	HBRUSH hBrBg;

	CAGDestroyTypedPtrArray<T> m_ItemsPtrArray;

	static int CALLBACK compareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
		thisClass* t = (thisClass*)lParamSort;
		T* pItem1 = t->ArrayGetItemAt((int) lParam1);
		T* pItem2 = t->ArrayGetItemAt((int) lParam2);
		int result = T::compareItems(pItem1, pItem2, t->sortColumn);
		return (t->sortAscending ? result : -result);
	}
};

#endif

/**
* @file
* $Id$
*/

