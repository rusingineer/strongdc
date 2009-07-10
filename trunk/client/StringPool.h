#ifndef _STRINGPOOL_H
#define _STRINGPOOL_H

class StringTable {

	typedef std::unordered_map<std::string, size_t> StringMap;
    StringMap sm;

public:

    StringTable() 
    {
    }

    const std::string& addString(const std::string& s);
    
    void removeString(const std::string& s);
};

class StringPool 
{

    const std::string* pointer;
	static StringTable* table;

public:

	StringPool() : pointer(NULL) 
	{
	}
	
	StringPool(const StringPool&);
	
	~StringPool();
	
	inline StringPool& operator=(const StringPool& sp) { pointer = sp.pointer; return *this; }
	StringPool& operator=(const std::string& s);
	
	inline operator const std::string&() const { return get(); }

	inline void put(const std::string& s) { pointer = &table->addString(s); }
	inline const std::string& get() const { return *pointer; }

};

#endif	// _STRINGPOOL_H