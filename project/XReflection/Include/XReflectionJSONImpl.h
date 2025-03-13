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
#include <ostream>      //ostream support -  to JSON
#include <istream>      //istream support - from JSON

//treatment of quoted strings can change with C++ 14
//now. Despite boost stating on their website:
//"Quoted" I/O Manipulators for Strings are not yet accepted into Boost as public components. 
//they have included it. And it's preferable to use this than to write our own implementation.
#ifdef CPP14
#include <iomanip>      //std::quoted
#else
#include "boost/io/detail/quoted_manip.hpp" //quoted
#endif

#include <boost/bind/placeholders.hpp>          //for the placeholders used in boost::bind
//support for smart pointer types - add headers here
#include <boost/shared_ptr.hpp>
//support for STL containers - add headers here
#include <vector>

#include "XReflectionDebug.h"    //debugging

#include "XReflectionClassFactory.h" //test change
//----------------------------

using namespace std;
//======================JSON output of a class=========================

namespace XReflection
{
        namespace LITERALS
        {
                //literals used in this file and also in XReflectionExtensions.h
                static const std::string CLASSFIELD = "class";
                static const char COLON = ':';          //separator name : value
                static const char STARTOBJECT = '{';
                static const char ENDOBJECT = '}';
				static const char STARTARR = '[';
				static const char ENDARR = ']';
				static const char COMMA = ',';
				static const char SPACE = ' ';
                static const char QUOTE = '"';
                static const char NULLPTR = '0';        //used to identify null pointers in objects. Note this is a single char so that
                                                                                        //the overloads of << and >> don't mess up with considerations about char*/std::string
                                                                                        //and also so that we can spot it in advance using peek() on the stream.
                                                                                        //it we wanted to change this to 'null' for example it will be more hassle.
        }
        using namespace XReflection::LITERALS;
//----------------------------------------
        //helper functions for reading and writing go in an unnanamed namespace.
        namespace
        {
            //-------write helpers---------

            //write the string with appropriate escape characters and surrounded by quotes
            void WriteString(ostream &out, const std::string &in)
            {
                #ifdef CPP14
                out << std::quoted(in);		//STL will only work with C++ 14
                #else
                out << boost::io::quoted(in);	//use boost instead
                #endif
            }

            //This is a special type to allow us to embed a JSON literal within a class in such as way that it becomes part of the bigger message
			//unquoted string. Again we need to check if this is JSON or not before proceeding.
			//we do that by ensuring the first character is a { or [ in the string.
			void WriteStringUnq(ostream &out, const std::string &in)
			{
				if (in[0] == STARTOBJECT || in[0]==STARTARR)
				{
					//it's raw JSON. Output it literally with no quoting
					out << in.c_str();
				}
				else
				{
					//its not JSON. Output it as a string literal as usual. 
					#ifdef CPP14
					out << std::quoted(in);		//STL will only work with C++ 14
					#else
					out << boost::io::quoted(in);	//use boost instead
					#endif
				}
			}

            //------------------read helpers------------------------
            //it's all a bit 'C' like here because it's embedded within the templates and I wanted to keep it efficient.

           //skip any spaces in the stream up to the next valid character
            void skipSpaces(istream &is)
            {
                if (is.peek() == SPACE)
                {
                    char tmp;
				    do { is >> tmp; } 
                    while (is.peek() == SPACE && !is.eof());
                }
            }

            //expects stream current position is at a field value (ie just after a :)
            //will skip to the next , } or ]
            void skipToFieldEnd(istream &is)
            {
                char t =is.peek();
                if (t != COMMA && t != ENDOBJECT && t!=ENDARR)
                {
                    char tmp;
				    do 
                    {
                         is >> tmp;
                         t =is.peek();
                    }  while (t != COMMA && t != ENDOBJECT && t!=ENDARR && !is.eof());
                }        
            }


            //read a string value and strip off the quotes and escape characters
            void ReadString(istream &in, std::string&out)
            {
                #ifdef CPP14
                in >> std::quoted(out);			//STL will only work with C++ 14			
                #else			
                in >> boost::io::quoted(out);	//use boost instead
                #endif
            }

            //when the value of a field can be arbitrary
            //such as TRUE,true,0,1 for a boolean
            //then we use this reader.
            //will read until the next , or } or ] character
            //will skip any leading or trailing spaces or CR/TAB/LF (indeed will actually skip ANY spaces so T r u   e  would read as 'True' too)
            //because it reads until the next comma.
            //this is a 'value'. It's not likely to be massive.
            void ReadBoolean(istream &in, std::string&out)
            {
					register int BOOL_SIZE_LIMIT = 16;	 //buffer size limit. If we are sending something longer than 16 chars for a boolean then we've got issues anyways
					char *buf = new char[BOOL_SIZE_LIMIT];	//allocate a buffer for our boolean string/embedded JSON
													//this is a bit risky but this is quicker and will likely work for cases now.
					char *st = buf;					//insertion ptr
					char t,nxt;
					do 
					{
						in.read(&t, 1);	        //read current character
                        nxt= (char)in.peek();	//get NEXT character without reading it
                        if (t==SPACE || (t<(char)32)) continue; //skip this character if it's a space or a special character (tab/CR/LF)
						*st++ = t;			    //store and inc bufferptr
						if (!--BOOL_SIZE_LIMIT) //ensure no buffer overrun
						{
							delete[] buf;
							throw std::runtime_error("XReflectionJSONImpl.h JSON problem. Length of boolean value too large.");
						}
					} while (!in.eof() && nxt != COMMA && nxt!=ENDARR && nxt!=ENDOBJECT); //don't carry on past the end of the value,array,object OR the end of the file.
					*st = 0;		//terminate our buffer
					out = buf;		//assign to the output string
					delete[] buf;	//kill our buffer
            }

			//special string formatter. Used when embedding JSON objects within a special unquotedstring
			//object - used to allow us to embed JSON (that we won't parse) within an unquotedstring object
			//and hence allow us to 'pass on' JSON that we don't understand in the framework.
			//because a member might be a JSON string...
			//----------
			//will unquote a string "goo" but will
			//also accept obnject text {foo} or [foo] and will read it to the final brace at the end of the object.
			//string can either be {some_text} or [some_text] 
			//***written in 'C' using native types for speed.****
			void ReadStringUnq(istream &in, std::string&out)
			{
                skipSpaces(in);             //skip any leading spaces
				char t = (char)in.peek();	//get next valid character

                //hopefully now we are pointing to the start of an object or an array
				if (t == STARTOBJECT || t== STARTARR)
				{	//is not a quoted string. It's an object. We know the first char is [or { 
					register int limit = 65536;			//buffer size limit - 
					//we could use a #define and save ourselves an allocation here but not great style
					//with any luck the compiler will optimize it away anyways. 
					register short nest = 0;	//we are unlikely to encounter very many levels of nesting. An int is overkill. 
					char *buf = new char[limit];	//allocate a 64k buffer for our unquoted string/embedded JSON
													//this is a bit risky but this is quicker and will likely work for cases now.
					char *st = buf;					//insertion ptr
					char t;
					do //when we start this loop we KNOW the next char is a { or [ see IF above
					{
						in.read(&t, 1);	//read one char
						if (t == STARTOBJECT || t==STARTARR) ++nest;		//increase nest level because we hit a { or [
						else if (t == ENDOBJECT || t==ENDARR) --nest;		//decrese when we hit a } or ]
						*st++ = t;			//store and inc bufferptr
						if (!--limit)		//ensure no buffer overrun
						{
							delete[] buf;
							throw std::runtime_error("XReflectionJSONImpl.h Internal error. Length of UnquotedString exceeds 64K buffer.");
						}
					} while (nest); //and carry on until we are at zero in which case we reached the matching } that we are looking for
					//we simply read all the characters until we close out the object
					*st = 0;		//terminate our buffer
					out = buf;		//assign to the output string
					delete[] buf;	//kill our buffer
				}
				else
				{ //we treat a string otherwise because embedded JSON must be wrapped with {}or []
					//this will of course run into problems with malformed JSON
					//because the only format we accept is Foo: "string" or foo : [some JSON] or foo : { some JSON }
                    //but what if this string value is actually like this "stringmember" : 2  (we have a type mismatch with the sender and we expect a string but the sender sent an int? 

					//but then we will wind up throwing an exception elsewhere
					//should can check here to make sure? It would make debugging easier.

					//TO DO - we can check the type of the member here and do a more exhaustive check.
					//if (t != QUOTE) throw std::runtime_error("string members MUST be wrapped by double quotes OR be unquoted string valid JSON inside curly or square braces");
#ifdef CPP14
					in >> std::quoted(out);			//STL will only work with C++ 14			
#else			
					in >> boost::io::quoted(out);	//use boost instead
#endif
				}
			}

            //if stream points to a null indicator then return true and skip it otherwise, return with stream read point unchanged.
            //note that the JSON keyword 'null' is not the same as '0' when it comes to pointers.
			//'0' means a C++ null (zero) pointer. However 'null' would mean 'missing' - IE - the field is just not there
			//this is not currently supported. This is why we indicate null pointers as zeroes.
			bool IsNull(istream &in)
            {
                char discard;
                bool bRet = in.peek() == NULLPTR;  //C++ null pointer indicator next?
                if (bRet) in >> discard;
                return bRet;
            }

            //read the stream and verifiy the next character was the one passed in
            //makes the code slightly shorter
			//will read through any spaces in the stream
			//so if it's looking for a : and the stream is <space><space>: then it will still return true after skipping the spaces
			//if the character was NOT the character specified (after skipping the spaces and so on)
			//then return FALSE and back up as if it never happened.
            bool ReadNextCharIs(istream&in, char c)
            {
				iostream::pos_type currentPosition = in.tellg();		//current read position
                char tmp;
				do { in >> tmp; } while (tmp == SPACE && !in.eof());	//skip any spaces
				if (in.eof()) throw std::runtime_error("Unexpected End of File. Is stream truncated?");
                bool bRet = (tmp == c);
				if (!bRet) in.seekg(currentPosition);		//if it wasn't the character, back up the stream.
				return (bRet) ? true : false;	//there was a stupid warning so used this rather than return (bret);
            }

            //this is called when the field at the JSON stream does not match any members in the current object
            //We need to skip it. It might be a valid derived class variable or an out-of-order base class variable or simply a variable we don't need.
            //it could be "Foo" : 0,  or "foo" : {some massive object} or even "foo" : [{objects},{objects} ] we need to skip it.
            void skipField(istream &is)
            {
                std::string fieldNameToSkip;
                ReadString(is, fieldNameToSkip);							//get the name of the field
                #ifdef REFLECTION_DEBUG
                Logger::Instance()<<"Skpping field: "<<fieldNameToSkip<;
                #endif
				if (!ReadNextCharIs(is, COLON))  //look for colon in "name" : value
			    {
				   std::string betterErr = "JSON problem. Unrecognized field is badly formatted. Expected colon(:) between name and value for unrecognized field: " + fieldNameToSkip;
				   throw std::runtime_error(betterErr); 
			    }
                
                //now we have the value. It might be a straight value or it might be an array or an object that needs skipping

                //skip any spaces first
                skipSpaces(is);
                char t=is.peek();
                if (t == STARTARR || t==STARTOBJECT) 
                {
                    //it's a complex object. Skip it using ReadStringUnq which will take us past the final }
                    std::string tmp;    //thow it away
                    ReadStringUnq(is,tmp);
                }
                else
                {
                    if (t == QUOTE)
                    {   //if it's text then we need to skip the text field to the end quote.
                        std::string tmp;
                        ReadString(is,tmp);
                    }
                    else
                    {
                        //it's a simple value like 123 or TRUE then skip to the next ,}]
                        skipToFieldEnd(is);
                    }
                }
                //and in any case - finally read the comma if there is one
                ReadNextCharIs(is, COMMA);
            }
        }//end namespace
//-----------------------------

    //implementations go into the _impl namespace
    namespace _impl
    {
            class JSONWriterFldVisitor;
            //visit each member in the class with the passed in stream visitor
            template<typename MainClass, typename Visitor>
            void visitAllJsonWrite(MainClass & c, Visitor &v, ostream &os)
            {
                    //we use a 'trick' here using the template to get a compile-time constant expression for our number of members
                    typedef boost::mpl::range_c<int, 0, XReflHelper::members<MainClass>::count> members;
					//iterate through all the member variables in this object and write each one to the output.
                    boost::mpl::for_each<members>(boost::bind<void>(JSONWriterFldVisitor(os), boost::ref(c), boost::ref(v),boost::placeholders::_1));
            }//note boost_1_59 needs to be _1 and boost 1_64 onwards boost::placeholders::_1

            class JSONWriterFldVisitor
            {
            public:
                JSONWriterFldVisitor(ostream &os) : m_os(os) {};
                template<typename MainClass, typename Visitor, typename I>
                void operator()(MainClass& myclass, Visitor &myvisitor, I)
                {
                        bool bIsLast = (I::value == XReflHelper::members<MainClass>::count - 1 );    //if this is the very last member in the class set a flag.
                        myvisitor(m_os, bIsLast, XReflHelper::getFieldByIndex<I::value>(myclass));
                }
                ostream &m_os;
            };

            class JSONWriterVisitor
            {
            public:
                    JSONWriterVisitor() {};
					//note. FieldData here is going to be the 'type' of our member metadata structure
					//see XReflection.h template<typename ThisClass> struct member_metadata<i, ThisClass>
					//each member will have a specific template struct created to hold it's metadata called:
					//template <type of this class> member_metadata<1, Type of this class> 
					//these structs are generated at compile-time
					//and every class will have an instance called member_metadata for each item
					//identified by an index. 

                    template<typename FieldData> void operator()(ostream &os, bool bIsLast,FieldData f)
                    {
                            //we rely on the object (whatever it is)
                            //to correctly serialize itself using << so for example
                            //a string type will quote itself
                            //an object type will wrap itself in braces and so on.
                            //This is neatly managed because we can overload the << operator
                            //for the std::string and because we already are overloading the
                            //<< operator on objects - which makes this nicely recursive
                            
                            //work around for the pain-in the neck compiler
                            if (f.type() == "bool") os << std::boolalpha;  //unfortuantely we have to work around bools here.
                            //the std:: implementation will map our bools to 'true'/'false' for us

                            os << f.name() << COLON << f.value(); //output name:value pair by REFERENCE
                            
                            if (!bIsLast) os << COMMA;      //add comma if NOT the last member of this class.
                    }
            private:
                    JSONWriterVisitor(JSONWriterVisitor &) {};   //no copies.
            };

//====================overloads of the << operator========================

            //We overload the << operator for our std::string so that our strings will be quoted in the JSON.
            //Note - consider escape characters in our strings.
            
            //boolean handling can't be overloaded but fortunately we can ask the stream to output bools as 'true'/'false'
            //which we do elsewhere in the code by calling os << std::boolalpha in our workaround above.

            inline ostream& operator << (ostream &out, const std::string & obj)
            {
                WriteString(out, obj);
                return out;
            }


            //support for pointers to things
            //this ensures that pointers to things show the object in the JSON and not just the numeric pointer
            template <typename U> ostream& operator << (ostream &out, U* p)
            {
                    if (p) out << (*p); else out << NULLPTR; //output the deferenced object or null indicator
                    return out;
            }
    } //end _impl namespace

   //interface
   template<typename MainClass> void toJSON(MainClass& obj, ostream &os)
   {
            //now the fields and data - this will iterate through the fields and dump them all in JSON format
            //if the fields are themselves objects then they will call toJSON on themselves and so forth.
            _impl::JSONWriterVisitor tmp;  //create our visitor to dump the fields to the stream
            _impl::visitAllJsonWrite(obj, tmp, os);
    }
  
 //these routines are used to open,close and separate objects and base/derived class items
//note that this routine writes the object opener and the class field.
   template<typename MainClass> void writeOpenObject(MainClass& obj, ostream &os)
   {
       os << STARTOBJECT;    //open the object 
       std::string objType = obj.getCppClassName();    //get our class name from the routine we also generated
       WriteString(os, CLASSFIELD);    //the class field 'ie: Class' 
       os << COLON;
       WriteString(os, objType);    //class name
       os << COMMA;
   }

//note that this reference is a dummy parameter
   template<typename MainClass> inline void writeCloseObject(MainClass&, ostream &os)
   {
       os << ENDOBJECT;
   }

//note that this reference is a dummy parameter
   template<typename MainClass> inline void writeBaseDerivedSeparator(MainClass&, ostream &os)
   {
       os << COMMA;
   }

   //============================JSON input of a class===================================
  //read the object open { and return the class name - and throw if not there
  //if the stream is the format {"class":"foo","field","bar" 
  //the stream position will wind up at the start of the first field,value pair
  //
  //==============
  //we have had to cheat a bit here by inlining it and adding dummy template arguments to allow gcc to not be annoying
  //when it knows the type at compile-time. See remarks in ReflectionClasFactory.h
   //this function will read a 'class' specifier and return it. 
   //if it doesn't exist, it will just return "" (if bAllowMissing is true) or throw (if bAllowMissing is false)
   //if it does exist, it will ALWAYS throw if the field is malformed.
  template<typename DUMMY> inline std::string readOpenObject(DUMMY *, istream &is,bool bAllowMissing)
  {
    //Logger::Instance()<<"read openobject\n";
     //first we expect the open object character
    if (!ReadNextCharIs(is, STARTOBJECT)) throw std::runtime_error("opening { not found at start of JSON input object");
	iostream::pos_type readOpenPosition = is.tellg();		//current read position
	std::string classField;
    ReadString(is, classField);
	if (classField != CLASSFIELD)
	{//sometimes we allow the class field to be missing (for pointers) but sometimes we REALLY need it
		if (!bAllowMissing) throw std::runtime_error("JSON missing compulsory 'class' field for an object. Every object starts with 'class':'Foo'");
		//if the field is allowed to be missing, we need to back up to remove the field name we just read.
		is.seekg(readOpenPosition);
		return "";		//and return nothing
	}
	//it was the class field. Read it and return it
	if (!ReadNextCharIs(is, COLON)) throw std::runtime_error("JSON missing : between 'class' field and class name. Should be 'class':'foo' ");
    std::string className;
    ReadString(is, className);	//get the class name from the message
    //this is the class name of the object that's coming up in the stream
    //which may or may not need to be dynamically allocated via a base class pointer
    if (!ReadNextCharIs(is, COMMA)) throw std::runtime_error("JSON missing comma after 'class':'foo' for an object. ");
    return className;
  }

  template<typename MainClass> void readCloseObject(MainClass& obj, istream &is)
  {
	  if (!ReadNextCharIs(is, ENDOBJECT))
	  { //error. Try and be helpful. Read a couple of fragments of the JSON
		  std::string A, B, C;
		  ReadString(is, A);
		  ReadString(is, B);
		  ReadString(is, C);
		  throw std::runtime_error(std::string("closing } not found in JSON after interrogating last member in class: " + obj.getCppClassName() + " Could be bad JSON but this is often caused by too many fields in JSON for the members to accept. Has there been a change? Check your JSON against your class definition. The next remaining (unused) fragment in the JSON input stream were: " + A+B+C));
	  }
  }

  //implementation
   namespace _impl //implementation namespace
   {
                           //helper  to use the correct overload of >> so we don't
                           //have to duplicate treatment for maps,vectors of objects and pointers.
                           //this helper will (in the case of an object) serialize into it.
                           //in the case of a pointer to an object, it will create a new one, reset the pointer that was passed in, and then deserialize into that
                           //this must be in the _impl namespace for the operator resolution
                           //to work properly. It is used
                           //for STL types in XReflectionExtensions.h but because
                           //it's important functionality and related to the overloaded >>
                           //I've placed it in here.
                           template<typename T>
                           struct Deserializer
                           { //not pointer types
                                   static void in(T*obj, istream &in)
                                   {
                                       in >> obj;      //use pointer overload for a non-pointer type on purpose
                                   }
                           };
                           //the use of pointers (potentially to pointers) is needed in the deserialization
                           //because we need to create new objects on the heap and assign them to existing pointers.
                           //thus T** (pointer to pointer) is an appropriate mechanism.
                           template<typename T>
                           struct Deserializer<T*>
                           {  //deserialize into a pointer type 
                              //By definition, this pointer type will be uninitialised (not zeros) so we will need to initialize it and write it back first.
                                   static void in(T**obj, istream &in)
                                   {
                                       if (IsNull(in))
                                       {
                                           //if object is a null ptr but object already has a pointer (which it shouldn't) then error it.
                                           if (*obj) throw std::runtime_error(std::string("The object already contains a non-null ptr in a member  but JSON contained nullptr. Error. Do not serialize into objects that already have allocated pointers unless you know the incoming object will never have a null pointer in there! I can't clean up your memory leaks! "));
                                       }
                                       else
                                       { //object is valid.
                                           *obj = XReflection::internalcreatehelper<T>(in); //open the object, read and create.
                                           if (!obj) throw std::runtime_error("Internal error. Create gave nullptr");
                                           in >> *obj;     //we still need to use the pointer (T*) overload
                                       }
                                 } //end function
                           };

                           class JSONReaderFldVisitor;
                           //visit each member in the class with the passed in stream visitor
                           template<typename MainClass, typename Visitor>
                           void visitAllJsonRead(MainClass & c, Visitor &v, istream &is)
                           {
                                   typedef boost::mpl::range_c<int, 0, XReflHelper::members<MainClass>::count> members;
								   //here we will visit every member variable in our object and present it with the JSON stream
								   //if the members are in the same order as our JSON then it will all be efficient.
								   //and every member will be populated after one iteration of the 'while' loop.

								   //however, what happens if the members are NOT in the same order as our JSON? 
								   //In that case - we would need to repeatedly iterate the members until one iteration
								   //did not result in ANY members responding.
								   //at which point we can be sure that the stream no longer is pointing to a member of this
								   //class, and we can stop looking. 
									//we know a member responded because it sets bReadOK to true if the name matched and it read, otherwise it leaves it unchanged.
								   bool bAnyMembersResponded,AtEndOfObject;
								   do
								   {
									   bAnyMembersResponded = false;
                                       #ifdef REFLECTION_DEBUG
                                       Logger::Instance()<<"Visiting all members in class tree of: "<<c.getCppClassName()<<" \n";
                                       #endif
                                       //this will iterate all the members of the class and present each member with the current JSON position to deal with.
                                       //We need to know whether the value was read OK by one of them or whether the value was recognized by NONE of them.
									   boost::mpl::for_each<members>(boost::bind<void>(JSONReaderFldVisitor(is), boost::ref(c), boost::ref(v), boost::ref(bAnyMembersResponded), boost::placeholders::_1));
									   //if even one member responds and reads it then bAnyMembersResponded will be set to 'true' otherwise UNCHANGED at false.

                                       //when JSON stream reaches the end of the object then the next item will be the closing bracket AND no members will respond (the visitor will just return)
                                       ////note boost 1_50 is _1 and 1_64 on boost::placeholders::_1

                                       //circumstances where a member does not respond;
                                       //1. The stream is at the end of the object or block } 
                                       //2. The stream has reached the end of the members for this (base) class and is now pointing at derived variables
                                       //3. The stream is pointing at a variable that does not exist (in this base class or indeed any derived class either)

                                       //this is problematic because if we skip this next variable we may accidentally be skipping a more-derived class member
                                       #ifdef REFLECTION_DEBUG
                                       Logger::Instance()<<"Members responded: " << ((bAnyMembersResponded) ? "Yes\n" : "No\n");
                                       #endif
                                       //if we are NOT at the end of the object (where we naturally expect no members to respond) then we need to skip the field amd go around this loop again
                                       //the only issue with this - is that in a class hierarchy where we serialize the super class, base class, derived class... 
									   //right now we are trying an approach where we revert the stream to the beginning for the derived class
                                       //the problem with this... is that then the stream is here {base,base,derived,derived} and because none of the items
                                       //in the member list respond to the first 'base' item it comes here and tries to skip the whole message. ooops. 
                                       
                                       //experimental - this is a test solution to the above problem--
                                       AtEndOfObject = is.peek() == ENDOBJECT;
                                      //if no members responded and we are not at the end of the object
									  //then we skip the entry and just pretend that someone responded so we can go around the loop again.
                                       if (!bAnyMembersResponded && !AtEndOfObject) 
									   {
                                           skipField(is);
                                           bAnyMembersResponded=true;   //pretend that a member responded to our skipped field
									   }                                   

								  } while (bAnyMembersResponded && !AtEndOfObject); //experimental
                                  #ifdef REFLECTION_DEBUG
                                  Logger::Instance()<<"Finished visiting for class \n";
                                  #endif
                           }

                           class JSONReaderFldVisitor
                           {
                           public:
                                   JSONReaderFldVisitor(istream &is) : m_is(is) {};
                                   template<typename MainClass, typename Visitor, typename I>
                                   void operator()(MainClass& myclass, Visitor &myvisitor, bool &bReadOK, I)
                                   {
									   //here we access the member variable at index 'I' in our class and visit it.
                                           //bool bIsLast = (I::value == XReflHelper::members<MainClass>::count - 1);  //if this is the very last member in the class set a flag.
                                           //myvisitor(m_is, bIsLast, XReflHelper::getFieldByIndex<I::value>(myclass));
										 myvisitor(m_is, bReadOK, XReflHelper::getFieldByIndex<I::value>(myclass));
                                   }
                                   istream &m_is;
                           };

                           class JSONReaderVisitor
                           {
                           public:
                                   JSONReaderVisitor() {};
                                   //fieldData is some template of type:  struct member_metadata<i, ThisClass>
                                   //where ThisClass is the type of the underlying member variable. 
                                   //bReadOK will be set to true if the member was populated because the JSON is at the correct place in the stream
                                   //and will be UNCHANGED (this is important) if the member did not match the JSON. This allows us to
                                   //support JSON that's in a different order to our members admittedly with an efficiency loss.
                                   
                                   //we will in the current implementation run into issues when the JSON is missing a value we need
                                   //or contains a field that we don't need.

                                   template<typename FieldData> void operator()(istream &is, bool& bReadOK, FieldData f)
                                   {
									   if (is.peek() == ENDOBJECT) return;				//refuse to continue if already at end of an object

									   //iostream::pos_type savedPos = is.tellg();		//remember the current read position   
									   //if this happens to be the start of an object structure...	  
                                           //if we encounter it here, it means that it doesn't relate to a pointer type
                                           //(because it will have already been read by an overloaded >> operator for a pointer type)
                                           //and hence if this is a value type then we can't use it, so we throw it away.

										   if (is.peek() == STARTOBJECT)
                                           {   //If this is the first field in an OBJECT we try and read the opener and the class info
                                               //this will be present If it's not already been eaten up because THIS object is a pointer type
                                               //and the >> operator needed it to create the object that we are now processing.
                                               //remember this code is recursive.

                                               //we have been forced to include a dummy parameter to this template so that gcc will work correclly
                                               //this needs to be fixed 
                                                XReflection::readOpenObject(this, is,true);	//we can ignore the class field  - and we don't care if it wasn't in the JSON
                                            }

                                           //now we have a clean set of name:value pairs to process for the object that we are in, that should match the members.
                                            //we rely on the object (whatever it is)
                                           //to correctly deserialize itself using >> so for example
                                           //a string type will unquote itself, an object type will unwrap itself and so on.


                                            //NOTE. We remember the current read position here so that if we don't recognize the member and back up
                                            //we don't wind up going back before the opening brace again because this will confuse the routine
                                            //that skips fields into skipping the whole message. 

                                           iostream::pos_type savedPos = is.tellg();		//remember the current read position
										   std::string JsonName;
                                           ReadString(is, JsonName);							//get the name of the field
										   if (!ReadNextCharIs(is, COLON))  //look for colon in "name" : value
										   {
											   std::string betterErr = "JSON problem. Expected colon(:) between name and value for field: " + JsonName;
											   throw std::runtime_error(betterErr); 
										   }
										   const std::string &membername = f.name();

										   if (JsonName != membername)
										   {
											   //JSON name doesn't match our member. Perhaps JSON is in the wrong order. Back up and exit.
                                               //the code will basically try every member it has in turn against this JSON field until it populates something.
                                                //note that bReadOK will be UNCHANGED in this instance.
											   is.seekg(savedPos);
											   return;
										   }
										   try
										   {
											   is >> f.valuePtr();  //deserialize into whatever it is. See overloads of >> operator
											   bReadOK = true;		//we read something OK - tell the caller
											   ReadNextCharIs(is, COMMA);	//read the next char if it's a comma otherwise don't read it - it might be the end of the object
										   }
										   catch (std::runtime_error &ref)
										   {	//write a better error that includes the field name we were processing
											   std::string betterError = "Error processing field: " + membername + std::string(" : ") + ref.what();
											   throw std::runtime_error(betterError);
										   }
                                   }
                           private:
                                   JSONReaderVisitor(JSONReaderVisitor &) {};   //no copies.
                           };

                           //======================overloads of the >> operator======================
                                                   //string handling
                                                   ///our stream at this point will be expected to be a string at front.
                                                   //It might be "hello","World"],"member2":4,"member3":{.......
                                                   //when expecting a string, we need to read the stream until the first non-escaped " character.
                                                   //we make this inline because we need to it to be used in multiple compilation units
                                                   inline istream& operator >> (istream &in, std::string *p)
                                                   {
                                                       ReadString(in, *p);
                                                       return in;
                                                   }
                                                   
                                                   //bool handling
                                                   //our stream at this point might be '0','1' or even 'TRUE' or 'True' or perhaps Y/N or T/F
                                                   //however it won't be surrounded by quotes in the JSON obviously
                                                   //we need to sensibly deal with these combinations
                                                   //these aren't standard JSON but it's always a good idea to intelligently handle other people's mistakes.

                                                   //NOTE. There is an existing issue with 'char' members and compilation issues
                                                   //partially caused by this
                                                   inline istream& operator >> (istream &in, bool *p)
                                                   {
                                                       std::string theValue;
                                                       ReadBoolean(in,theValue);
                                                       //now examine our boolean value from the JSON
                                                       
                                                       //first convert to lowercase
                                                       
                                                       //C++ 11 only
                                                       std::transform(theValue.begin(), theValue.end(), theValue.begin(),
                                                       [](unsigned char c){ return std::tolower(c); });
                                                       
                                                       //and now examine it and try and make sense of it
                                                       bool assignvalue=false;
                                                       
                                                       if (theValue.length()==1)
                                                       {
                                                           if (theValue=="1" || theValue == "Y" || theValue == "T") 
                                                               assignvalue=true;
                                                           else if (theValue=="0" || theValue == "N" || theValue == "F") 
                                                                assignvalue=false;
                                                           else 
                                                           {
                                                               std::string err="JSON problem. Boolean value field contains unrecognized character: "+theValue;
                                                               throw std::runtime_error(err);
                                                           }
                                                       }
                                                       else
                                                       { 
                                                           if (theValue=="true") 
                                                              assignvalue=true;
                                                           else if (theValue=="false") 
                                                               assignvalue=false;
                                                           else 
                                                           {
                                                               std::string err="JSON problem. Boolean value field contains unrecognized word: "+theValue;
                                                               throw std::runtime_error(err);
                                                           }
                                                       }
                                                       *p=assignvalue; 
                                                       return in;
                                                   }

                                                   //general pointer to something. Because when deserializing we use pointers that means that regular object members
                                                   //native types, strings and so forth, come through here. 
                                                   template <typename U> istream& operator >> (istream &in, U* p)
                                                   {
                                                      //Logger::Instance()<<"field..";
                                                       if (!p) throw std::runtime_error("Unable to deserialize into a null pointer");
                                                       in >> (*p);
                                                       return in;
                                                   }
        
                                                   //support for pointers to pointers things (we use pointer to pointer for this)
                                                   //this is used for member pointers to pointers to objects.
                                                   //it is NOT used for pointers within containers. That is handled within the XReflectionJSONExtension.h code.
                                                   //we do it this way so we can dynamically create an object and influence our callers object space
                                                   //(we need a pointer to a pointer for that)
                                                   //see the use of f.ValuePtr() to return the pointer to the member and not a reference to it.
                                                   //in the case of a member of type MyClass * we will actually receive MyClass ** see?
                                                   template <typename U> istream& operator >> (istream &in, U** p)
                                                   {
                                                            U* ptr = *p;            //get the pointer to the member (from the pointer to the pointer)
                                                            //First we check if the pointer is null (ie the field is 0 ) since we expect {some object} 
                                                            if (IsNull(in))
                                                            {      //the json object is NULL (ie the sending object pointer was null)
                                                                   //if the JSON has a null pointer but our object has a pre-existing pointer in it
                                                                   //then rather than leave it unchanged, or overwrite it, we throw an exception.
                                                                   if (ptr) throw std::runtime_error("Received nullptr but object already has pointer in it. Memory leak warning. I will not deserialize a nullptr into an object that already has a populated pointer");
                                                            }
                                                            else
                                                            { 
                                                                //Not null. We can go ahead and open and read the object
                                                                //Logger::Instance()<<"Calling create..\n";
                                                                *p = XReflection::internalcreatehelper<U>(in);       //make object and store in the pointer to pointer
                                                                //Logger::Instance()<<"Deserialse into object\n";
                                                                in >> *p;                                 //and deserialize into object pointed to by the pointer.
                                                                //Logger::Instance()<<"Done object creation and population\n";
                                                          }
                                                           return in;
                                                   } //end template
                                   } //end _impl namespace

                                   //interface
                                    template<typename MainClass> void fromJSON(MainClass& obj, istream &is)
                                    {
                                        _impl::JSONReaderVisitor tmp;
                                        _impl::visitAllJsonRead(obj, tmp, is);
                                    }

} //end XReflection namespace
