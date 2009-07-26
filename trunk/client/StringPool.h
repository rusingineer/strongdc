#ifndef _STRINGPOOL_H
#define _STRINGPOOL_H

class StringPool {

	typedef std::tr1::unordered_map<std::string, size_t> StringMap;
    StringMap sm;

public:

    StringPool() 
    {
    }

    const std::string& addString(const std::string& s);
    
    void removeString(const std::string& s);
};

class pooled_string 
{

    const std::string* data;
	static StringPool* pool;

public:

	pooled_string() : data(NULL) 
	{
	}
	
	pooled_string(const pooled_string&);
	
	~pooled_string();
	
	inline pooled_string& operator=(const pooled_string& sp) { data = sp.data; return *this; }
	pooled_string& operator=(const std::string& s);
	
	inline operator const std::string&() const { return get(); }

	inline void put(const std::string& s) { data = &pool->addString(s); }
	inline const std::string& get() const { return *data; }

};

#endif	// _STRINGPOOL_H