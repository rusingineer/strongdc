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
class ColumnBase {
public:
	ColumnBase() : root(NULL) { }
	~ColumnBase() { clearData(); }

	/*
	 * Returns reference to col's text.
	 * Complexity: linear
	 */
	const tstring& getText(const uint8_t col) const {
		Node* node = root;
		while(node != NULL) {
			if(node->key == col) {
				return node->data;
			}
			node = node->next;
		}
		return Util::emptyStringT;
	}

	/*
	 * Stores column value for later usage, or deletes it if adding empty string
	 * Complexity: constant for first update, else linear
	 */
	void setText(const uint8_t name, const tstring& val, bool firstUpdate = false) {
		if(!firstUpdate) {
			// if we're not doing the first update, check whether the column already exists
			Node* node = root;
			Node* prev = NULL;
			while(node != NULL) {
				if(node->key == name) {
					// column already exists
					if(val.empty()) {
						// but we delete it
						if(prev == NULL) {
							root = node->next;
						} else {
							prev->next = node->next;
						}
						delete node;
					} else {
						// we set the new column value
						node->data = val;
					}
					return;
				}
				prev = node;
				node = node->next;
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
			node = node->next;

			delete tmp;
		}
		root = NULL;
	}

	inline const void* getRoot() const { return root; }

private:
	struct Node {
		Node(uint8_t _key, const tstring& _data, Node* _next = NULL) : key(_key), data(_data), next(_next) { }
		~Node() { }

		tstring data;
		Node* next;
		uint8_t key;
	};

	Node* root;
};

#pragma pack(pop)   /* restore original alignment from stack */

#endif
