#if !defined(AGARRAYTEMPLATES_H__)
#define AGARRAYTEMPLATES_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// CAGPtrArray

class CAGPtrArray {
public:

// Construction
	CAGPtrArray();

// Attributes
	int GetSize() const;
	int GetUpperBound() const;
	void SetSize(int nNewSize, int nGrowBy = -1);
	void SetAllocSize(int nNewSize, int nGrowBy = -1);

// Operations
	// Clean up
	void FreeExtra();
	void RemoveAll();

	// Accessing elements
	void* GetAt(int nIndex) const;
	void SetAt(int nIndex, void* newElement);

	void*& ElementAt(int nIndex);

	// Direct Access to the element data (may return NULL)
	const void** GetData() const;
	void** GetData();

	// Potentially growing the array
	void SetAtGrow(int nIndex, void* newElement);

	int Add(void* newElement);
	int AddSpeedy(void* newElement);

	int Append(const CAGPtrArray& src);
	void Copy(const CAGPtrArray& src);

	// overloaded operator helpers
	void* operator[](int nIndex) const;
	void*& operator[](int nIndex);

	// Operations that move elements around
	void InsertAt(int nIndex, void* newElement, int nCount = 1);

	void RemoveAt(int nIndex, int nCount = 1);
	void InsertAt(int nStartIndex, CAGPtrArray* pNewArray);

// Implementation
protected:
	void** m_pData;   // the actual array of data
	int m_nSize;     // # of elements (upperBound - 1)
	int m_nMaxSize;  // max allocated
	int m_nGrowBy;   // grow amount


public:
	~CAGPtrArray();

protected:
	// local typedefs for class templates
	typedef void* BASE_TYPE;
	typedef void* BASE_ARG_TYPE;
};

// CAGTypedPtrArray

template<class TYPE>
class CAGTypedPtrArray : public CAGPtrArray {
public:
	// Accessing elements
	TYPE GetAt(int nIndex) const
		{ return (TYPE)CAGPtrArray::GetAt(nIndex); }
	TYPE& ElementAt(int nIndex)
		{ return (TYPE&)CAGPtrArray::ElementAt(nIndex); }
	void SetAt(int nIndex, TYPE ptr)
		{ CAGPtrArray::SetAt(nIndex, ptr); }

	// Potentially growing the array
	void SetAtGrow(int nIndex, TYPE newElement)
	   { CAGPtrArray::SetAtGrow(nIndex, newElement); }

	int Add(TYPE newElement)
	   { return CAGPtrArray::Add(newElement); }

	int AddSpeedy(TYPE newElement)
	   { return CAGPtrArray::AddSpeedy(newElement); }

	int Append(const CAGTypedPtrArray<TYPE>& src)
	   { return CAGPtrArray::Append(src); }

	void Copy(const CAGTypedPtrArray<TYPE>& src)
		{ CAGPtrArray::Copy(src); }

	// Operations that move elements around
	void InsertAt(int nIndex, TYPE newElement, int nCount = 1)
		{ CAGPtrArray::InsertAt(nIndex, newElement, nCount); }

	void InsertAt(int nStartIndex, CAGTypedPtrArray<TYPE>* pNewArray)
	   { CAGPtrArray::InsertAt(nStartIndex, pNewArray); }

	// overloaded operator helpers
	TYPE operator[](int nIndex) const
		{ return (TYPE)CAGPtrArray::operator[](nIndex); }
	TYPE& operator[](int nIndex)
		{ return (TYPE&)CAGPtrArray::operator[](nIndex); }
};

// CAGDestroyTypedPtrArray

template <class Type>
class CAGDestroyTypedPtrArray : public CAGTypedPtrArray<Type*> {
public:
	typedef CAGTypedPtrArray<Type*> TParentClass;

	CAGDestroyTypedPtrArray() {
	}

	virtual ~CAGDestroyTypedPtrArray() {
	}

	void RemoveAt(int nIndex, int nCount = 1, bool bDelete = true) {
		if (bDelete) {
			Type* pItem;
			for (int cnt = nIndex; cnt < nIndex+nCount; cnt++) {
				pItem = GetAt(cnt);
				if (pItem != NULL)
					delete pItem;
			}
		}
		TParentClass::RemoveAt(nIndex, nCount);
	}

	void RemoveAll(bool bDelete = true) {
		if (bDelete) {
			Type* pItem;
			for (int cnt = 0; cnt < GetSize(); cnt++) {
				pItem = GetAt(cnt);
				if (pItem != NULL)
					delete pItem;
			}
		}
		TParentClass::RemoveAll();
	}

	void RemoveAllNoDestructSize(bool bDelete = true) {
		if (bDelete) {
			Type* pItem;
			for (int cnt = 0; cnt < GetSize(); cnt++) {
				pItem = TParentClass::ElementAt(cnt);
				if (pItem != NULL) {
					delete pItem;
					TParentClass::ElementAt(cnt) = NULL;
				}
			}
		}
		m_nSize = 0;
	}

};

#endif // AGARRAYTEMPLATES_H__