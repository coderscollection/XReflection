#pragma once

//XReflect
//(c) John Nessworthy 2019.  Email ideaboxconsult@gmail.com
//C++ reflection and JSON serialization Library

//This software does not come with any warranty express or implied and is to be used at your own risk.
//The author does not accept any responsbility for an losses due to it's use, however they are incurred.
//You are free to compile and use this code in your own application but please give me credit and do
//not claim that the work is your own.
//----------------------

#include <string>
#include <iostream>

#ifdef _WIN32
   #include <windows.h>
#endif

//if we log in our code through here then
////we can keep windows versus linux debug logging issues to a minumum
////always terminates the line with \n. 
////This stops us needlessly cluttering the code up with \n chars.
////and macros and other rubbish. 
//
////This class is just a placeholder. It doesn't claim to be a logging solution
//there are all sorts of problems using singletons like this in templates across compilation units
//This should not be used in production code

namespace XReflection
{
	class Logger
	{
	public:
		//static interface
		static Logger &Instance()
		{
			static Logger myInstance;
			return myInstance;
		}

		//class interface
		template<typename T> void Log(T in)
		{
#ifdef _WIN32
			std::cout << in << std::flush;
#else
			//linux logging. However we want.
			std::cerr << in << std::flush;
#endif
		}
	};

	template <typename T> Logger& operator <<(Logger& rhs, T const& value)
	{
		rhs.Log(value);
		return rhs;
	}
}

