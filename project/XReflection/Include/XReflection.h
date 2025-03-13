#pragma once

//XReflect
//(c) John Nessworthy 2019.  Email ideaboxconsult@gmail.com
//C++ reflection and JSON serialization Library

//This software does not come with any warranty express or implied and is to be used at your own risk.
//The author does not accept any responsbility for an losses due to it's use, however they are incurred.
//You are free to compile and use this code in your own application but please give me credit and do
//not claim that the work is your own.

//these header files have been tested and compile on windows studio 2015 and gcc on Red Hat Linux
//they have been tested against boost_1.69  and boost 1_59 but you will require a small change for boost 1_59
//as mentioned in the code (it relates to the change from _1  to boost::placeholders::_1 in those boost versions)

//---------------------------------------------
//I am leaving these in here because in case of a real build problem
//it's very useful to comment these in. They won't be used normally in code
//#define STRINGIFY(s) XSTRINGIFY(s)
//#define XSTRINGIFY(s) #s
//#pragma message ("BOOST_PP_VARIADICS=" STRINGIFY(BOOST_PP_VARIADICS))
//#pragma message( "Compiling: " __FILE__ )
//-----------------------------------------------

//boost preprocessor
#include <boost/preprocessor/variadic/size.hpp>         //BOOST_PP_VARIADIC_SIZE
#include <boost/preprocessor/variadic/to_seq.hpp>       //BOOST_PP_VARIADIC_TO_SEQ
#include <boost/preprocessor/seq/for_each_i.hpp>        //BOOST_PP_SEQ_FOR_EACH_I
#include <boost/type_traits.hpp>                        //essential/

#include <typeinfo>                                     //typeID for factory so it can search on typeID if needed

//our JSON reflection fmwk classes
#include "XReflectionImpl.h"                         //implementation for member functions
#include "XReflectionJSONImpl.h"                     //implementation for JSON
#include "XReflectionJSONextensions.h"				//implementation for STL containers in JSON - container extensions add here
#include "XReflectionClassFactory.h"				//class factory to handle pointers of arbitrary objects

//extension type classes
#include "XReflectionTypes.h"						//extend the framework by adding support for additional types in here.

//Note. If you are using an older gcc compiler you will need -std=c++0x
#if (BOOST_PP_VARIADICS == 0)
#pragma message ("WARNING. BOOST_PP_VARIADICS is not defined or zero. This will result in build errors in reflection. Something is wrong with the compiler settings. Check that you are using -std=c++0x if using gcc")
#endif 


//include this file in any class file header that you want reflected.
//you will need to change the member variable declarations though in your class
//to follow this form below
//---------------------------------------------------------
//EXAMPLE

//so. Here we see that to make a class reflectable we must define our member
//variables in a slightly different way to usual.
//don't worry. When it compiles, the members will be just the same as usual.
//you can see this in this example where we are accessing the members in the 'ctor just like we normally would.
//please also see the test applications.

/*
Basic syntax for base classes
class Foo
{
    SERIALIZE_JSON(Foo)  <--this tells the fmwk to manufacture the fromJSON and toJSON methods for the class.
    XREFLECT
    (
        (double) DoubleMember,  <---remember to put a space after the bracket and a comma not a semicolon
        (std::string) StringMember,
        (MyClass) ObjectMember,
        (MyClass *) NullObjectPtrMember  <----but the last member doesn't need a comma or a semicolon. Looks like this
    )
}
REFLECTION_REGISTER(Foo)  <------ this is needed to register the class with the factory so that it can be created using XReflect::create 

For derived classes, replace SERIALIZE_JSON(Foo) with SERIALIZE_JSON_DERIVED(DerivedName,BaseName) see examples
*/

//when the class is generated and compiled it will have regular member variables just the same as usual
//you will be able to access them in just the usual way as always
//however, they will also be 'reflected' which means that we can dynamically access them.
//using a generated member function FieldByName.

//if we also use the Macro SERIALIZE_JSON we will able to to serialize/de-serialize the object to/from JSON
//using generated functions toJSON and fromJSON

//take a look in the test applications for how this works.


//Other Considerations
//----------------------
//1.
//fieldbyName allows you to dynamically get and set private members of the class
//it's not easily possible to stop that from happening.
//so this means that if you have a 'reflectable' class you are able to access its
//private members using this reflection functionality.

//2.
//Any classes included in your reflected class by instantiation as direct members or pointers
//need to EITHER:
//1. Be reflected themselves and include the SERIALIZE_JSON macro
//OR
//2. Implement the operator << and >> and serialize themselves to JSON using their own techniques.
//see The notes in  "XReflectionJSONextensions.h"which tell you how to add new types.
//this technique is used to allow stl containers to be serialized.

//3. Reference member variables in objects are not currently supported.
//because basically it will be too much of a pain to make it work.

//4. Multiple inheritance will not work.
//the macro provides for classes to inherit from ONE class.
//however multiple levels of inheritance are supported because it's not worth the aggravation.
//for example super -> base- > derived -> object will work just fine. 

//5. There is a problem with 'char' member variables. These are quite unncommon but they won't compile (yet).

//6. Classes need a default constructor defined (in addition to whatever other constructors you have)

//--------------------End of introduction comments---------------------

using namespace std;

//=========================================================
//generation of the reflected class
//=========================================================

//---helper macros---
//unfortunately we need to use preprocessor macros

//in all the below macros, the 'expression' will be a single 'argument' from our XREFLECTION argument list
//for example (int *) m_Member
//or (vector<int>) m_memberTwo
//These macros allow us to turn these arguments
//into useful statements like declarations, extract the type, and extract the name of the member.

//insert the member variable correctly without parentheses as a declaration from the 'expression'
//XREFL_INSRTMEMBR( (int *) membername
//evaluates to  XREFL_RMVE (int *) membername
//evaluates to int * membername
#define XREFL_INSRTMEMBR(x) XREFL_RMVE x

//Remove the type (in brackets) from the 'expression' - leaving just the intended member name behind
//This relies on the type having a space after it so that
//the below line XREFL_RMBRACKET( (int *)member ) evaluates to:
//XREFL_DSCRD (int*) member which evaluates to:  member
//(because the XREFL_DSCRD just discards its arguments)
#define XREFL_RMBRACKET(x) XREFL_DSCRD x

//shared variadic macro to discard the parameter(s) passed in.
#define XREFL_DSCRD(...)

//----------
//implement XREFL_PPTYPEOF (preprocessor type of)
//this is used to extract the type of our argument from the 'expression'.
#define XREFL_PPTYPEOF(x) XREFL_PPTYPEOF_DETAIL(XREFL_TYPEINF_CHECK x,)
#define XREFL_TYPEINF_CHECK(...) (__VA_ARGS__),
//note that windows and linux require a different definition.
#ifdef _WIN32
#define XREFL_PPTYPEOF_DETAIL(...) XREFL_TYPEINF_VS((__VA_ARGS__))
#define XREFL_TYPEINF_VS(args) XREFL_TYPEINF_HEAD args
#else
#define XREFL_PPTYPEOF_DETAIL(...) XREFL_TYPEINF_HEAD(__VA_ARGS__)
#endif
#define XREFL_TYPEINF_HEAD(x, ...) XREFL_RMVE x

//shared variadic macro to remove the () from around something
#define XREFL_RMVE(...) __VA_ARGS__

//---end of helper macros---

//----------main macros---------

//this is the repetition macro - this is the code that gets inserted into the header.
//i is the rep counter .. 0..1..2..3 etc
//x is the stuff in the sequence. In this case, our desired member list

//note that the methods below value() and name() are the accessor functions for our members
//note that ValueC() gives a const reference.
//whilst value() gives a non-const reference.
//the name() method generated below gives us the name of the member

//How it works
//----------------
//every member variable will be inserted first via XREFL_INSRTMEMBR
//and then a template inner class will be generated for that member
//specializing for that particular member.

//Particular note
//XREFL_PPTYPEOF macro gives us the type of our member variable
//XREFL_RMBRACKET is used to remove the ( ) declaration from a parameter  - ie if you have (int *)m_x then it will give you m_x (the name of the member)
//XREFL_INSRTMEMBR is used to insert a declaration of our member variable (without the brackets around it).
//These three macros are important - because they allow us to access the actual members from our thisClassRef reference variable
//and also to work out the type that they are. Hence this is why the value() method is  declared in this manner.
//we get the type from XREFL_PPTYPEOF(x) , we get the name of the member that we want from XREFL_RMBRACKET(x)
//you will note that we declare the c'tor for our member metadata here so that we can set the reference variable via the initialisation list.

#define PROCESS_MEMBER(r, NotUsed, i, elem) \
        XREFL_INSRTMEMBR(elem); \
        template<typename ThisClass> struct member_metadata<i, ThisClass> \
        { \
                ThisClass & m_thisClassRef; \
                member_metadata(ThisClass & ref) : m_thisClassRef(ref) {} \
                virtual ~member_metadata() {} \
                virtual const std::string& name() const \
                { \
                    static const std::string mynam=BOOST_PP_STRINGIZE(XREFL_RMBRACKET(elem)); \
                    return mynam;\
                }\
                virtual const std::string& type() const \
                {\
                    static const std::string myTyp=BOOST_PP_STRINGIZE(XREFL_PPTYPEOF(elem));\
                    return myTyp;\
                }\
                XREFL_PPTYPEOF(elem) &value() const \
                { \
                        return m_thisClassRef.XREFL_RMBRACKET(elem); \
                }\
                XREFL_PPTYPEOF(elem)* valuePtr() const \
                { \
                        return &(m_thisClassRef.XREFL_RMBRACKET(elem)); \
                }\
        }; \

//-------------
//Main Reflection Macro
//this takes the list of member variables as a parameter and then effectively generates our class body for us.
//The macro works by inserting the code below and then BOOS_PP_SEQ_FOR_EACH will insert the repetition macro a number of times.
//the type member_metadata is defined above and we have a number of instances of it for each member variable.   (see PROCESS_MEMBER above)
//you will note that we add methods in here to implement allfieldsToStream and fieldByName into our class
//although they then will use the correct templated version of the code in XReflection.
//you will note that we have to define out number of members as a static const int so that it can be compile-time evaluated

//you will also note that because we want to incorporate public methods we are forced to change the access modifier at the end
//which means that the last methods will be public.
//which could wind up with making the stuff below the macro public unintentionally which won't break it, but is not really ideal.
//have to think about how to overcome that.

#define XREFLECT(...) \
        friend  class XReflection::XReflHelper; \
        template<typename U> friend struct XReflection::XReflHelper::members; \
        static const int NumberOfMembers = BOOST_PP_VARIADIC_SIZE(__VA_ARGS__); \
        template<int index, typename ThisClass> struct member_metadata {}; \
        BOOST_PP_SEQ_FOR_EACH_I(PROCESS_MEMBER,~, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) \
    public:\
        template<typename DesiredFieldType> DesiredFieldType& fieldByName(const std::string &name)\
        {\
                return XReflection::fieldByName<DesiredFieldType>(*this, name);\
        }\
//note that BOOST_PP_SEQ_FOR_EACH_I takes arguments Macro, Data, Seq.  where the Macro is a macro with the form (r,data,i,elem) see boost documentation

//-----------------------------------
//JSON Macros

//JSON input and output for actual base classes
#define SERIALIZE_JSON(classnam)\
SERIALIZE_JSON_OUTPUT(classnam)\
SERIALIZE_JSON_INPUT_CREATE(classnam)\
SERIALISE_JSON_INPUT(classnam)\

//abstract base classes
#define SERIALIZE_JSON_ABSTRACT(classnam)\
SERIALIZE_JSON_OUTPUT(classnam)\
SERIALIZE_JSON_INPUT_CREATE_ABSTRACT(classnam)\
SERIALISE_JSON_INPUT(classnam)\

//JSON input and output for derived classes
#define SERIALIZE_JSON_DERIVED(classnam,parentnam)\
SERIALIZE_JSON_OUTPUT_DERIVED(classnam,parentnam)\
SERIALISE_JSON_INPUT_DERIVED(classnam,parentnam)\
//Notes. This implementation requires the class writing the JSON to be EXACTLY THE SAME
//as the class that reads it. If extra members are added, your old JSON will break in the new class.
//if members are removed, it will break.
//if the order is different, it will break.
//I have written this for speed
//not for backward compatibility or niceness. be aware.

//--------------base classes---------------------
//JSON output only
//rather than go to all the effort of changing the visitors to be totally const correct
//I have had to put a const_cast in here. Makes it simpler in the end.
//I was forced to use a default parameter to handle the recursion here. 
#define SERIALIZE_JSON_OUTPUT(classnam)\
    public:\
    virtual std::string getCppClassName(void)\
    {\
        return #classnam;\
    }\
    virtual std::string getCppBaseClassName(void)\
    {\
        return #classnam;\
    }\
    virtual void toJSON(ostream &os,bool bStart=true)\
            {\
                if (bStart) XReflection::writeOpenObject(*this, os);\
                XReflection::toJSON(*this, os);\
                if (bStart) XReflection::writeCloseObject(*this, os);\
            }\
            friend ostream& operator<< (ostream &out, const classnam & obj)\
    {\
    const_cast<classnam &>(obj).toJSON(out);\
    return out;\
    }\
    friend ostream& XReflection::_impl::operator<< (ostream &out, const std::string& obj);\
	friend ostream& XReflection::_impl::operator<< (ostream &out, const DteDateTime& obj);\
    template <typename U> friend ostream& XReflection::_impl::operator<< (ostream &out, U* p);\
    SERIALIZE_JSON_OUTPUT_EXT(classnam)\
// Note that to provide extensions for new types please see XReflectionJSONExtensions.h

//JSON input only
//note we had to add the pointer overload here because of the trick we are doing with pointer types in the extensions code.

//creation method for real base
#define SERIALIZE_JSON_INPUT_CREATE(classnam)\
public:\
    classnam *create()\
    {\
		return new classnam();\
    }\

//creation method for abstract base
//it can not create an instance of an abstract base so will return a nullptr
//if someone weirdly tries to create an instance of an abstract base.
#define SERIALIZE_JSON_INPUT_CREATE_ABSTRACT(classnam)\
public:\
    classnam *create()\
    {\
	    return NULL;\
    }\

//remainder of JSON input methods
#define SERIALISE_JSON_INPUT(classnam)\
    virtual void fromJSON(istream &is)\
    {\
        XReflection::fromJSON(*this, is);\
        XReflection::readCloseObject(*this, is);\
    }\
    virtual void fromJSONinternal(istream &is)\
    {\
        XReflection::fromJSON(*this, is);\
    }\
    friend istream& operator >> (istream &in, classnam & obj)\
    {\
        obj.fromJSON(in);\
        return in;\
    }\
    friend istream& operator >> (istream &in, classnam * obj)\
    {\
        obj->fromJSON(in);\
        return in;\
    }\
    friend istream& XReflection::_impl::operator >> (istream &in, std::string* obj);\
	friend istream& XReflection::_impl::operator >> (istream &in, DteDateTime* obj);\
    template <typename U> friend istream& XReflection::_impl::operator >> (istream &in, U* p);\
    template <typename U> friend istream& XReflection::_impl::operator >> (istream &in, U** p);\
    SERIALIZE_JSON_INPUT_EXT(classnam)\
// Note that to provide extensions for new types please see XReflectionJSONExtensions.h

//------------derived classes-----------------

//JSON output only
//rather than go to all the effort of changing the visitors to be totally const correct
//I have had to put a const_cast in here. Makes it simpler in the end.
//note I was forced to add this default parameter to handle the recursion.
//note that member function calls to toJSON will recurse but
//called to XReflection::ToJSON will not.
//you will note that we specifically call the base class implementation of the virtual function here.
//which has the effect of walking up the heirarchy of classes until it reaches the very top. 
#define SERIALIZE_JSON_OUTPUT_DERIVED(classnam,parentnam)\
    public:\
    virtual std::string getCppClassName(void)\
    {\
        return #classnam;\
    }\
    virtual void toJSON(ostream &os,bool bStart=true)\
    {\
        if (bStart) XReflection::writeOpenObject(*this, os);\
        parentnam *mybase = static_cast<parentnam *>(this);\
        mybase->parentnam::toJSON(os,false);\
        XReflection::writeBaseDerivedSeparator(*this,os);\
        XReflection::toJSON(*this, os);\
        if (bStart) XReflection::writeCloseObject(*this, os);\
    }\
    friend ostream& operator<< (ostream &out, const classnam & obj)\
    {\
        const_cast<classnam &>(obj).toJSON(out);\
        return out;\
    }\
    friend ostream& XReflection::_impl::operator<< (ostream &out, const std::string& obj);\
	friend ostream& XReflection::_impl::operator<< (ostream &out, const DteDateTime& obj);\
    template <typename U> friend ostream& XReflection::_impl::operator<< (ostream &out, U* p);\
    SERIALIZE_JSON_OUTPUT_EXT(classnam)\
// Note that to provide extensions for new types please see XReflectionJSONExtensions.h

//JSON input only
//note we had to add the pointer overload here because of the trick we are doing with pointer types in the extensions code.
//note also that bIsFirst=true is only true for a direct call to FromJSON
//(from the client)
//internal calls to fromJSON (from the >> operator) have this FALSE.
//so that the read of the opening object is not performed here
//but rather inside the operator itself so that it can determine what
//type of object to create. 

//the other thing to note is that when reading a derived class
//we basically need to read the whole stream from the beginning again
//in case the variables in the JSON are in the wrong order.
//this hurts efficiency so we have a choice. We either support
//variables in the wrong order and 'extra' variables and be less efficient
//or we don't support ANY of that... and be more efficient.

//TO DO
//need to add a switch in here to turn on/off this 'enhancement feature' so we can tune the code in terms of efficiency.
#define SERIALISE_JSON_INPUT_DERIVED(classnam,parentnam)\
    public:\
    parentnam *create()\
    {\
        return new classnam();\
    }\
    virtual void fromJSON(istream &is)\
    {\
        fromJSONinternal(is);\
        XReflection::readCloseObject(*this, is);\
    }\
    virtual void fromJSONinternal(istream &is)\
    {\
        iostream::pos_type fromJSONpos = is.tellg();\
        parentnam *mybase = static_cast<parentnam *>(this);\
        mybase->parentnam::fromJSONinternal(is);\
        is.clear();\
        is.seekg(fromJSONpos);\
        XReflection::fromJSON(*this, is);\
    }\
    friend istream& operator>> (istream &in, classnam & obj)\
    {\
        obj.fromJSON(in);\
        return in;\
    }\
    friend istream& operator>> (istream &in, classnam * obj)\
    {\
        obj->fromJSON(in);\
        return in;\
    }\
    friend istream& XReflection::_impl::operator>> (istream &in, std::string* obj);\
	friend istream& XReflection::_impl::operator >> (istream &in, DteDateTime* obj);\
    template <typename U> friend istream& XReflection::_impl::operator>> (istream &in,U* p);\
    template <typename U> friend istream& XReflection::_impl::operator>> (istream &in, U** p);\
    SERIALIZE_JSON_INPUT_EXT(classnam)\
// Note that to provide extensions for new types please see XReflectionJSONExtensions.h

//------------------------------------------------------------------------------------------

//------------------factory extensions to provide support for base pointers to classes of derived objects-------

//macro to register and create a factory for arbitrary objects - used by reflection fmwk when processing arrays of pointers
//this is the macro that the developer must place in their code after the class they wish to register.
//syntax: REFLECTION_REGISTER(classname)

//the global instance of the individual factory will be instantiated on program startup and will register itself with the registry. 
//every factory has a 'create' function that creates the object that it's associated with by calling the 'create' function
//this is known as the 'virtual constructor' pattern in C++.
#define REFLECTION_REGISTER(classnam)\
class classnam##_factory : public XReflection::iFactory\
{\
    public:\
    classnam##_factory(void)\
    {\
        XReflection::ClassRegistry::Instance().registerclass(#classnam,this);\
    }\
    std::string getTypeID()\
    {\
        return typeid(m_item).name();\
    }\
    void* create()\
    {\
        return (void*)m_item.create();\
    }\
private:\
    classnam m_item;\
};\
static classnam##_factory classnam##_global_factory; //global instance of the individual factory called classnam_global_factory

//--------------end of main macros--------------

