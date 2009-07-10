#include "stdinc.h"
#include "DCPlusPlus.h"

#include "StringPool.h"

StringTable* StringPool::table = new StringTable();

const std::string& StringTable::addString(const std::string& s) 
{
	StringMap::iterator i = sm.find(s);
	if(i == sm.end())
	{
		return sm.insert(std::make_pair(s, 1)).first->first;
	}
	else
	{
		i->second++;
		return i->first;
	}
}

void StringTable::removeString(const std::string& s)
{
	StringMap::iterator i = sm.find(s);
	if(i == sm.end())
		dcassert(0);
	
	size_t& ref = i->second;
	if(--ref == 0)
		sm.erase(i);
}

StringPool::StringPool(const StringPool& s) : pointer(NULL)
{
	if(s.pointer)
		put(*s.pointer);
}

StringPool::~StringPool() 
{ 
	if(pointer) 
		table->removeString(*pointer); 
}

StringPool& StringPool::operator=(const std::string& s) 
{ 
	if(pointer == NULL || s != *pointer)
	{
		if(pointer) 
			table->removeString(*pointer);
			
		put(s); 
	}
	return *this;
}
