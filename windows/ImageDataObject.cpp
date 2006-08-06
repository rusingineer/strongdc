/* 
 * 
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
#include "ImageDataObject.h"

// Static member functions

void CImageDataObject::InsertBitmap(IRichEditOle* pRichEditOle, HBITMAP hBitmap) {
	SCODE sc;

	// Get the image data object

	CImageDataObject *pods = new CImageDataObject;
	LPDATAOBJECT lpDataObject;
	pods->QueryInterface(IID_IDataObject, (void **)&lpDataObject);

	pods->SetBitmap(hBitmap);

	// Get the RichEdit container site

	IOleClientSite *pOleClientSite;	
	pRichEditOle->GetClientSite(&pOleClientSite);

	// Initialize a Storage Object

	IStorage *pStorage;	

	LPLOCKBYTES lpLockBytes = NULL;
	sc = ::CreateILockBytesOnHGlobal(NULL, TRUE, &lpLockBytes);
	dcassert(lpLockBytes != NULL);
	
	sc = ::StgCreateDocfileOnILockBytes(lpLockBytes,
		STGM_SHARE_EXCLUSIVE|STGM_CREATE|STGM_READWRITE, 0, &pStorage);
	if (sc != S_OK)	{
		lpLockBytes = NULL;
	}
	dcassert(pStorage != NULL);

	// The final ole object which will be inserted in the richedit control

	IOleObject *pOleObject; 
	pOleObject = pods->GetOleObject(pOleClientSite, pStorage);

	OleSetContainedObject(pOleObject, TRUE);

	// Now Add the object to the RichEdit 

	REOBJECT reobject;
	ZeroMemory(&reobject, sizeof(REOBJECT));
	reobject.cbStruct = sizeof(REOBJECT);
	
	CLSID clsid;

	if(pOleObject != NULL) {
		sc = pOleObject->GetUserClassID(&clsid);

		reobject.clsid = clsid;
		reobject.cp = REO_CP_SELECTION;
		reobject.dvaspect = DVASPECT_CONTENT;
		reobject.dwFlags = REO_BELOWBASELINE;
		reobject.poleobj = pOleObject;
		reobject.polesite = pOleClientSite;
		reobject.pstg = pStorage;

		// Insert the bitmap at the current location in the richedit control

		pRichEditOle->InsertObject(&reobject);

		// Release all unnecessary interfaces

		pOleObject->Release();
	}

	pOleClientSite->Release();
	lpLockBytes->Release();
	pStorage->Release();
	lpDataObject->Release();
	pRichEditOle->Release();
	DeleteObject(hBitmap);
}

void CImageDataObject::SetBitmap(HBITMAP hBitmap) {
	dcassert(hBitmap);

	STGMEDIUM stgm;
	stgm.tymed = TYMED_GDI;
	stgm.hBitmap = hBitmap;
	stgm.pUnkForRelease = NULL;

	FORMATETC fm;
	fm.cfFormat = CF_BITMAP;
	fm.ptd = NULL;
	fm.dwAspect = DVASPECT_CONTENT;
	fm.lindex = -1;
	fm.tymed = TYMED_GDI;

	this->SetData(&fm, &stgm, TRUE);		
}

IOleObject *CImageDataObject::GetOleObject(IOleClientSite *pOleClientSite, IStorage *pStorage) {
	dcassert(m_stgmed.hBitmap);

	SCODE sc;
	IOleObject *pOleObject;
	sc = ::OleCreateStaticFromData(this, IID_IOleObject, OLERENDER_FORMAT, 
			&m_fromat, pOleClientSite, pStorage, (void **)&pOleObject);

	return pOleObject;
}
