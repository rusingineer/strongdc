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
#include "../client/DCPlusPlus.h"

#ifndef AGARRAYTEMPLATES_H__
	#include "AGArrayTemplates.h"
#endif

CAGPtrArray::CAGPtrArray() {
	m_pData = NULL;
	m_nSize = m_nMaxSize = m_nGrowBy = 0;
}

CAGPtrArray::~CAGPtrArray() {
	delete[] (BYTE*)m_pData;
}

void CAGPtrArray::SetSize(int nNewSize, int nGrowBy) {
	dcassert(nNewSize >= 0);

	if (nGrowBy != -1)
		m_nGrowBy = nGrowBy;  // set new size

	if (nNewSize == 0)	{
		delete[] (BYTE*)m_pData;
		m_pData = NULL;
		m_nSize = m_nMaxSize = 0;
	} else if (m_pData == NULL)	{
		// create one with exact size
#ifdef SIZE_T_MAX
		dcassert(nNewSize <= SIZE_T_MAX/sizeof(void*));    // no overflow
#endif
		m_pData = (void**) new BYTE[nNewSize * sizeof(void*)];

		memset2(m_pData, 0, nNewSize * sizeof(void*));  // zero fill

		m_nSize = m_nMaxSize = nNewSize;
	} else if (nNewSize <= m_nMaxSize) {
		// it fits
		if (nNewSize > m_nSize) {
			// initialize the new elements
			memset2(&m_pData[m_nSize], 0, (nNewSize-m_nSize) * sizeof(void*));
		}
		m_nSize = nNewSize;
	} else {
		// otherwise, grow array
		int nGrowBy = m_nGrowBy;
		if (nGrowBy == 0) {
			// heuristically determine growth when nGrowBy == 0
			//  (this avoids heap fragmentation in many situations)
			nGrowBy = min(1024, max(4, m_nSize / 8));
		}
		int nNewMax;
		if (nNewSize < m_nMaxSize + nGrowBy)
			nNewMax = m_nMaxSize + nGrowBy;  // granularity
		else
			nNewMax = nNewSize;  // no slush

		dcassert(nNewMax >= m_nMaxSize);  // no wrap around
#ifdef SIZE_T_MAX
		dcassert(nNewMax <= SIZE_T_MAX/sizeof(void*)); // no overflow
#endif
		void** pNewData = (void**) new BYTE[nNewMax * sizeof(void*)];

		// copy new data from old
		memcpy2(pNewData, m_pData, m_nSize * sizeof(void*));

		// construct remaining elements
		dcassert(nNewSize > m_nSize);

		memset2(&pNewData[m_nSize], 0, (nNewSize-m_nSize) * sizeof(void*));

		// get rid of old stuff (note: no destructors called)
		delete[] (BYTE*)m_pData;
		m_pData = pNewData;
		m_nSize = nNewSize;
		m_nMaxSize = nNewMax;
	}
}

void CAGPtrArray::SetAllocSize(int nNewSize, int nGrowBy) {
	SetSize(nNewSize, nGrowBy);
	m_nSize = 0;
}

int CAGPtrArray::Append(const CAGPtrArray& src) {
	dcassert(this != &src);   // cannot append to itself

	int nOldSize = m_nSize;
	SetSize(m_nSize + src.m_nSize);

	memcpy2(m_pData + nOldSize, src.m_pData, src.m_nSize * sizeof(void*));

	return nOldSize;
}

void CAGPtrArray::Copy(const CAGPtrArray& src) {
	dcassert(this != &src);   // cannot append to itself

	SetSize(src.m_nSize);

	memcpy2(m_pData, src.m_pData, src.m_nSize * sizeof(void*));
}

void CAGPtrArray::FreeExtra() {
	if (m_nSize != m_nMaxSize) {
		// shrink to desired size
#ifdef SIZE_T_MAX
		dcassert(m_nSize <= SIZE_T_MAX/sizeof(void*)); // no overflow
#endif
		void** pNewData = NULL;
		if (m_nSize != 0) {
			pNewData = (void**) new BYTE[m_nSize * sizeof(void*)];
			// copy new data from old
			memcpy2(pNewData, m_pData, m_nSize * sizeof(void*));
		}

		// get rid of old stuff (note: no destructors called)
		delete[] (BYTE*)m_pData;
		m_pData = pNewData;
		m_nMaxSize = m_nSize;
	}
}

void CAGPtrArray::SetAtGrow(int nIndex, void* newElement) {
	dcassert(nIndex >= 0);

	if (nIndex >= m_nSize)
		SetSize(nIndex+1);
	m_pData[nIndex] = newElement;
}

void CAGPtrArray::InsertAt(int nIndex, void* newElement, int nCount) {
	dcassert(nIndex >= 0);    // will expand to meet need
	dcassert(nCount > 0);     // zero or negative size not allowed

	if (nIndex >= m_nSize) {
		// adding after the end of the array
		SetSize(nIndex + nCount);  // grow so nIndex is valid
	} else {
		// inserting in the middle of the array
		int nOldSize = m_nSize;
		SetSize(m_nSize + nCount);  // grow it to new size
		// shift old data up to fill gap
		memmove(&m_pData[nIndex+nCount], &m_pData[nIndex],
			(nOldSize-nIndex) * sizeof(void*));

		// re-init slots we copied from

		memset2(&m_pData[nIndex], 0, nCount * sizeof(void*));
	}

	// insert new value in the gap
	dcassert(nIndex + nCount <= m_nSize);

	// copy elements into the empty space
	while (nCount--)
		m_pData[nIndex++] = newElement;
}

void CAGPtrArray::RemoveAt(int nIndex, int nCount) {
	dcassert(nIndex >= 0);
	dcassert(nCount >= 0);
	dcassert(nIndex + nCount <= m_nSize);

	// just remove a range
	int nMoveCount = m_nSize - (nIndex + nCount);

	if (nMoveCount)
		memmove(&m_pData[nIndex], &m_pData[nIndex + nCount],
			nMoveCount * sizeof(void*));
	m_nSize -= nCount;
}

void CAGPtrArray::InsertAt(int nStartIndex, CAGPtrArray* pNewArray) {
	dcassert(pNewArray != NULL);
	dcassert(nStartIndex >= 0);

	if (pNewArray->GetSize() > 0) {
		InsertAt(nStartIndex, pNewArray->GetAt(0), pNewArray->GetSize());
		for (int i = 0; i < pNewArray->GetSize(); i++)
			SetAt(nStartIndex + i, pNewArray->GetAt(i));
	}
}

int CAGPtrArray::GetSize() const
	{ return m_nSize; }

int CAGPtrArray::GetUpperBound() const
	{ return m_nSize-1; }

void CAGPtrArray::RemoveAll()
	{ SetSize(0); }

void* CAGPtrArray::GetAt(int nIndex) const {
	dcassert(nIndex >= 0 && nIndex < m_nSize);
	return m_pData[nIndex];
}

void CAGPtrArray::SetAt(int nIndex, void* newElement) {
	dcassert(nIndex >= 0 && nIndex < m_nSize);
	m_pData[nIndex] = newElement;
}

void*& CAGPtrArray::ElementAt(int nIndex) {
	dcassert(nIndex >= 0 && nIndex < m_nSize);
	return m_pData[nIndex];
}

const void** CAGPtrArray::GetData() const
	{ return (const void**)m_pData; }

void** CAGPtrArray::GetData()
	{ return (void**)m_pData; }

int CAGPtrArray::Add(void* newElement) {
	int nIndex = m_nSize;
		SetAtGrow(nIndex, newElement);
	return nIndex;
}

int CAGPtrArray::AddSpeedy(void* newElement) {
	int nIndex = m_nSize;
	dcassert(nIndex >= 0);

	if (nIndex >= m_nSize) {
		// Resizing
		int nNewSize = nIndex+1;
		if (nNewSize > m_nMaxSize) {
			int nGrowByGranuarity = min(5000, max(20, m_nSize / 2));
			if (m_nGrowBy < nGrowByGranuarity)
				m_nGrowBy = nGrowByGranuarity;
		}
		SetSize(nNewSize);
	}
	m_pData[nIndex] = newElement;
	return nIndex; 
}
	
void* CAGPtrArray::operator[](int nIndex) const
	{ return GetAt(nIndex); }

void*& CAGPtrArray::operator[](int nIndex)
	{ return ElementAt(nIndex); }
