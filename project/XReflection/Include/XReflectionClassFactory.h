#pragma once
//XReflect
//(c) John Nessworthy 2019.  Email ideaboxconsult@gmail.com
//C++ reflection and JSON serialization Library

//This software does not come with any warranty express or implied and is to be used at your own risk.
//The author does not accept any responsbility for an losses due to it's use, however they are incurred.
//You are free to compile and use this code in your own application but please give me credit and do
//not claim that the work is your own.
//----------------------

#include <ostream>                                                      //ostream support for streaming
#include <istream>                                                      //istream support for streaming
#include <sstream>                                                      //stringstream support
#include <stdexcept>

#include <map>
#include <string>
#include "XReflectionJSONImpl.h" 	//JSON helper

#include "XReflectionDebug.h"   //debug logging

namespace XReflection
{
    //class information returned by GetInfoForRegisteredClass
    struct RegisteredClassInfo
    {
        RegisteredClassInfo() : className(""),isNative(false) {};
        std::string className;
        bool isNative;
    };

    //base class for all factories. 
    class iFactory
    {
    public:
        virtual void *create() = 0;		//create function for arbitrary type with no common base class
        virtual ~iFactory() {};
        virtual std::string getTypeID() = 0;
    };

//factory and class registry - written as a header only implementation
//to avoid linking to a library.

//this type uses a raw pointer because the pointers refer to static objects
//and I don't want shared_ptr to try and auto-delete them. 
    typedef std::map<string, iFactory*> factoryMap;  //changed for gcc
    class ClassRegistry
    {
    private:  //weird order because I use this function in a public method below and this is a header-inly impl.
              //strip namespaces from the incoming string.
              //Foo::Bar becomes Bar & Foo::Bar::Test becomes Test
             std::string stripnamespaces(std::string in)
             {//we pass 'in' by value because we intend to copy it and change it.
              //It's quicker to pass by value than to pass by reference and then make a copy. 
              size_t start;
              while ((start = in.find("::")) != string::npos)
              {
                 in.erase(0,start+2);	//strip up to and including the :: from left.
              }
              return in;
        }

        XReflection::factoryMap m_factories; //changed for gcc
    public:
        static ClassRegistry &Instance()
        {
            static ClassRegistry *myInstance = NULL;	//initialized once.
            if (!myInstance)
            {
                myInstance = new ClassRegistry;
            }
            return *myInstance;
        }

        //return null if no factory exists otherwise return factory for class name
        //will throw only if a class name is specified with a namespace in it
        //and we couldn't find it. If the class is a 'bare' name and we couldn't find it
        //then we will return null (so that calls to here with 'int' for example don't throw - 
        //we don't need a factory to create ints for us)

        //NOTE. The classes that are registered by registerclass are not prefixed by namespaces
        //by the macro - the user will usually just register the bare class name
        //and even if he does, the register function strips off any namespaces.
        //However, on occasion, a caller might be qualifying the class WITH a namespace here and trying
        //to find a class that was registered without one.
        //sometimes the JSON that gets created by the framework will contain a namespace. 
        //because if one calles TypeID.name C++ will give you the namespace if called from 
        //a different namespace.
        //so we will try and find it with the namespace qualifier, and without.
        //hence if we register the class as 'foo' then create("namespace::foo") or create("foo") will
        //be equivalent.
        //note to caller. do NOT delete the pointer. You are not the owner. 
        iFactory* getFactory(const std::string &classnam)
        {
            if (classnam == "") return NULL;		//don't bother if no classname
            factoryMap::iterator iter = m_factories.find(classnam);
            if (iter != m_factories.end())
            {
                //found a registered factory to match the clas name as given
                return iter->second;
            }
            //not found as given. see if stripping the namespaces will help.
            std::string bareclass = stripnamespaces(classnam);
            if (bareclass != classnam)
            {
                iter = m_factories.find(bareclass);   //and try again.
                if (iter != m_factories.end())
                {
                    //found a registered factory to match the clas name once the namespace was removed
                    return iter->second;
                }
                else //and because it's likely that this is a problem, throw an error.
                    throw std::runtime_error("ClassRegistry::GetFactory can not find factory for specific class that was qualified with a namespace");
            }
            return NULL;    //couldn't find the factory.
        }
        //insert into the global registry, so long as it doesn't exist already.
        //NOTE. That the REFLECTION_REGISTER macro in XReflection.h
        //creates a STATIC class that, in it's c'tor, calls this routine with
        //this 'this'pointer. This will point to a static object. And hence
        //we don't want to delete that (the compiler will take care of that)
        void registerclass(const std::string &classnam,iFactory *const factoryPtr)
        {
            std::string bareclass = stripnamespaces(classnam);  //if the user specified a namespace in the macro, strip it out - just being defensive.
            iFactory* iExisting = getFactory(bareclass);
            if (!iExisting) 
            {
                m_factories.insert(factoryMap::value_type(bareclass, factoryPtr));
                Logger::Instance() << "Registering: " << bareclass << "\n";
            }
        }

        //retrieve a RegisteredClassInfo structure for a passed in class TypeID
        //this allows us to map class typeID to clean class names when sending JSON out.
        //because all our existing objects should have been registered. 
        RegisteredClassInfo getInfoForRegisteredClass(const std::string &typeID)
        {
           Logger::Instance() << "GetInfoForRegisteredClass: "<<typeID;
           if (typeID == "") throw std::runtime_error("ClassRegistry::getInfoForRegisteredClass empty typeID");
           RegisteredClassInfo returnvalue;
           factoryMap::iterator iter = m_factories.begin();
           iFactory *registeredClass=NULL;
           for ( ; iter != m_factories.end() ; ++iter)
           {
               registeredClass=iter->second;
               if (!registeredClass) throw std::runtime_error("ClassRegistry: Invalid Null ptr in list");
               if (typeID == registeredClass->getTypeID())
               {
                  returnvalue.className=iter->first;	//clean class name
                  returnvalue.isNative = false;	//to do
                  return returnvalue;			//normal return		
               }
           } //not found
           throw std::runtime_error(std::string("ClassRegistry::getInfoForRegisteredClass. Class not found. Have you missed registering a class? Class:")+typeID );
          }//end routine
    }; //end class

    //free function for named object creation
    //returns a pointer of type U* (typically Base Class *)
    //but creates an object of type 'name' (if that object is registered)
    //otherwise will create an object of type U (which is used for native types like int)
    //or objects like std::string or simple objects that don't need registration because
    //they are not inheriting from anything.
   /* template<typename U> U* create(const std::string& name)
    {
        //Logger::Instance()<<" Creating class: "<<name;
        iFactory* factory = ClassRegistry::Instance().getFactory(name);
        U *ptr;
        if (factory)
            ptr = static_cast<U*>(factory->create());  //there is a potential problem here that it's hard to error gracefully if the factory doesn't exist
        else	//no factory - perhaps it's a native type like an int or a string for example
            ptr = new U;
        return ptr;
    }
	*/

    //=====================
    //helper for JSON serialization (used INSIDE the framework - not for external calls)
	//this routine reads the stream, creates the object but does NOT populate it. It leaves the stream half-consumed.
	//this is not for external use. 

    //performs the same as above, but reads directly from the stream.
    //this code uses a function in the JSON implementation to read the stream data
    //which keeps the stream format separate from the factory implementation.
    //This works around the problem of having a header-only imeplemntation because I don't want a library for a helper function.
    ////and I don't want to duplicate the code in this compilation unit.
    template <typename DUMMY> inline std::string readOpenObject(DUMMY* obj, istream &is,bool bAllowMissing );
            
    //We will cheat for now by using our templated objects as a dummy parameters
    //whilst we work out a better solution. Gcc is a bit picky when it comes to compile time template generation
	//this routine will work for native types and also for objects (because you note it uses new U for natives)
    template<typename U> U* internalcreatehelper(istream &is)
    { 	
        std::string className;
	    U* dummy=NULL;
		className = XReflection::readOpenObject(dummy, is,false);  //if we are creating an object from a stream, clearly we need to know what it was so we insist on the 'class' field.

		U *ptr;
		iFactory* factory = ClassRegistry::Instance().getFactory(className);	//get object factory
		if (factory)
			ptr = static_cast<U*>(factory->create());	//there is a potential problem here that it's hard to error gracefully if the factory doesn't exist (and it won't exist for a native type)
		else	//no factory - perhaps it's a native type like an int or a string for example
			ptr = new U;
		return ptr;

	//	return XReflection::create<U>(className);				//create the object
	}
    
	//==========================
	//this routine IS for external callers. It reads a stream, creates the correct object and populates it from the JSON and then returns.
	//because this is for exernal callers, we can assume the object referenced is an object and not just a native type
	//so we don't need native type support in this. Which neatly avoids any compilation error for abstract base classes in this routine.
	//which would of course not compile if this routine contained 'new U' 
	template<typename U> U* createFromJSON(istream &is)
	{
		std::string className;
		U* ret = NULL;
		iostream::pos_type currentPosition = is.tellg();			//save current read position
		className = XReflection::readOpenObject(ret, is, false);  //if we are creating an object from a stream, clearly we need to know what it was
	
		iFactory* factory = ClassRegistry::Instance().getFactory(className);	//get object factory
		if (factory)
			ret = static_cast<U*>(factory->create());  //there is a potential problem here that it's hard to error gracefully if the factory doesn't exist
		else
			throw std::runtime_error("Unable to create object from JSON message. Perhaps object is not registered.");

		is.seekg(currentPosition);								//back up to where we ere.
		ret->fromJSON(is);										//populate object from the stream
		return ret;												//return the completed object
	}


} //end X Reflection namespace

