#pragma once

#include <string>
#include <iostream>

#ifdef _WIN32
   #include <windows.h>
#endif

//this class is used to insert JSON literals into an existing message
class unquotedstring
{
public:
	unquotedstring() : m_s("") {}
	unquotedstring(const char *in) : m_s(in) {} 
	unquotedstring& operator = (const std::string &rhs)
	{
		m_s = rhs;
		return *this;
	}
	const char *buf(void) const { return m_s.c_str(); }
	std::string& str(void) { return m_s; } //return reference to underlying string for pefficient population
private:
	std::string m_s;
};

//a sample extension class that does nothing
class DteDateTime
{
public:
	void setToNow(void)
	{

	};

	void setToNowGMT(void)
	{

	};
private:

};


namespace XReflection
{
	//If we write NOTHING to JSON for an empty datetime,  It messes up the JSON.
	//so for our sample class we need to just serialize an empty string.
	namespace _impl
	{
		//-----------sample extension class-----------------
		inline istream& operator >> (istream &in, DteDateTime *)
		{
			std::string tmp;
			ReadString(in, tmp);
			//translate the string that we read into our object... 
			return in;
		}

		inline ostream& operator << (ostream &out, const DteDateTime &)
		{
			std::string tmp;
			//serialise our object into a string..... 
			WriteString(out, tmp);
			return out;
		}

		//-------------------------unquoted strings----------------------------
		//unquoted string. Used when payload is a JSON message that we don't want to escape or wrap
		inline istream& operator >> (istream &in, unquotedstring *p)
		{
			std::string tmp;
			ReadStringUnq(in, tmp);
			*p = tmp;				//call assignment operator
			return in;
		}

		//unquoted string. Used when payload is a JSON message that we don't want to escape or wrap
		inline ostream& operator << (ostream &out, const unquotedstring & obj)
		{
			std::string tmp = obj.buf();	//create our string
			WriteStringUnq(out, tmp);		//write unqouoted
			return out;
		}

	} //end _impl namespace
} //end XReflection namespace

