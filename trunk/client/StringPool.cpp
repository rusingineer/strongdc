#include "stdinc.h"
#include "DCPlusPlus.h"

#include "StringPool.h"

StringPool* pooled_string::pool = new StringPool();

const std::string& StringPool::addString(const std::string& s) 
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

void StringPool::removeString(const std::string& s)
{
	StringMap::iterator i = sm.find(s); 
	if(i == sm.end())
		dcassert(0);
	
	size_t& ref = i->second;
	if(--ref == 0)
		sm.erase(i);
}

pooled_string::pooled_string(const pooled_string& s) : data(NULL)
{
	if(s.data)
		put(*s.data);
}

pooled_string::~pooled_string() 
{ 
	if(data) 
		pool->removeString(*data); 
}

pooled_string& pooled_string::operator=(const std::string& s) 
{ 
	if(data == NULL || s != *data)
	{
		if(data) 
			pool->removeString(*data);
			
		put(s); 
	}
	return *this;
}
