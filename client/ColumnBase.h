/*
 * Copyright (C) 2001-2007 Big Muscle
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

#ifndef _COLUMNBASE_H
#define _COLUMNBASE_H

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */

/*
 * Container for objects with columns info.
 * Optimized for cases with a lot of empty columns.
 * Allows unique keys only, if keys already contained, the value is updated.
 *
 * It must be called within one thread.
 */

struct Node {
public:
	Node(uint8_t _key, const tstring& _data, Node* _next = NULL) : key(_key), data(_data), next(_next) { }
	~Node() { }

	inline const tstring& getData() const { return data; }
	inline Node* getNext() const { return next; }
	inline uint8_t getKey() const { return key; }

	inline void setData(const tstring& value) { data = value; }
	inline void setNext(Node* nextNode) { next = nextNode; }

private:
	tstring data;
	Node* next;
	uint8_t key;
};

class ColumnBase {
public:
	ColumnBase() : root(NULL) { }
	~ColumnBase() { clearData(); }

	inline const tstring& operator[](uint8_t col) const { return get(col); }
	inline const void* getRoot() const { return root; }

	/*
	 * Returns reference to col's text.
	 * Complexity: linear
	 */
	const tstring& get(const uint8_t col) const {
		Node* node = root;
		while(node != NULL) {
			if(node->getKey() == col) {
				return node->getData();
			}
			node = node->getNext();
		}
		return Util::emptyStringT;
	}

	/*
	 * Stores column value for later usage, or deletes it if adding empty string
	 * Complexity: constant for first update, else linear
	 */
	void set(const uint8_t name, const tstring& val, bool firstUpdate = false) {
		if(!firstUpdate) {
			// if we're not doing the first update, check whether the column already exists
			Node* node = root;
			Node* prev = NULL;
			while(node != NULL) {
				if(node->getKey() == name) {
					// column already exists
					if(val.empty()) {
						// but we delete it
						if(prev == NULL) {
							root = node->getNext();
						} else {
							prev->setNext(node->getNext());
						}
						delete node;
					} else {
						// we set the new column value
						node->setData(val);
					}
					return;
				}
				prev = node;
				node = node->getNext();
			}
		}
		if(!val.empty())
			root = new Node(name, val, root);
	}

	/*
	 * Clears all texts.
	 * Complexity: linear
	 */
	void clearData() {
		Node* node = root;
		while(node != NULL) {
			Node* tmp = node;
			node = node->getNext();

			delete tmp;
		}
		root = NULL;
	}

private:
	Node* root;
};

#pragma pack(pop)   /* restore original alignment from stack */

#endif
