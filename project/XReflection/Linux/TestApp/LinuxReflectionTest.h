#pragma once

//test application header file
#include "XReflection.h"		//needed to make it work.

//example class and test application for C++ reflection demo. 
#include <boost/shared_ptr.hpp>	//needed for example sharedptr test
//-------------------------EXAMPLE CLASS-----------------------------------
struct Super
{
	virtual ~Super() {}
	XREFLECT
	(
		(int)SuperInt,
		(bool)SuperBool
	)
		SERIALIZE_JSON(Super)
};
REFLECTION_REGISTER(Super)

struct SimpleItem
{
	SimpleItem() : ItemInt(0),ItemString("") {}
	virtual ~SimpleItem() {}
	XREFLECT
	(
		(int)ItemInt,
		(std::string)ItemString
	)
		SERIALIZE_JSON(SimpleItem)
};
REFLECTION_REGISTER(SimpleItem)

struct MyBase : public Super
{
	virtual ~MyBase() {}
	XREFLECT
	(
		(long)MyBaseLong,
		(double)MyBaseDouble
	)
	SERIALIZE_JSON_DERIVED(MyBase,Super)
};
REFLECTION_REGISTER(MyBase)

//TO DO - If we want to break it, Add a 'char' member.
//Chars are a pain and cause all sorts of issues
//with our solution. It's the only native type i'm having trouble with.
struct MyClass : public MyBase
{
	virtual ~MyClass() {}
	MyClass() : x(0), y(0)
	{
	}
	MyClass(int a, int b) : x(a), y(b) {}
	XREFLECT
	(
		(int) x,
		(int) y,
		(std::string) z,
		(std::vector<SimpleItem>) listOfItems
	)
	SERIALIZE_JSON_DERIVED(MyClass, MyBase)
};
REFLECTION_REGISTER(MyClass)  //must be outside the class declaration

class base
{
public:
	virtual ~base(){}
	XREFLECT
	(
		(double) baseOne,
		(std::string) m_baseTwo,
		(MyClass) mBaseObjThree
	)
	SERIALIZE_JSON(base)
};
REFLECTION_REGISTER(base)

//here is a test class for the sky AS System Config File
//currently in this format.
/*
{
    "lowPowerTimerEnabled": true,
    "useThunderForHdmiControl": false,
    "cecWakeSupported": true,
    "motionDetectionSupported": true,
    "ffvWakeSupported": true,
    "hdmiInputSupported": true
}
*/

class SkyASSystemConfigFileTest
{
	public:
	SkyASSystemConfigFileTest() : lowPowerTimerEnabled(false),useThunderForHdmiControl(false),cecWakeSupported(false),
							  motionDetectionSupported(false),ffvWakeSupported(false),hdmiInputSupported(false)
	{
	}
	
	bool islowPowerTimerEnabled(void) const 
	{
		return lowPowerTimerEnabled;
	}	//blah blah just for illustration



	private:
	XREFLECT
	(
		(bool) lowPowerTimerEnabled,
    	(bool) useThunderForHdmiControl,
    	(bool) cecWakeSupported,
   		(bool) motionDetectionSupported,
    	(bool) ffvWakeSupported,
    	(bool) hdmiInputSupported
	)
	SERIALIZE_JSON(SkyASSystemConfigFileTest)
};
REFLECTION_REGISTER(SkyASSystemConfigFileTest)

//so. Here we see that to make a class reflectable we must define our member
//variables in a slightly different way to usual. 
//don't worry. When it compiles, the members will be just the same as usual. 
//you can see this in this example where we are accessing the members in the 'ctor just like we normally would.
class TestClass : public base
{
public:
	TestClass() :  ObjectPtrMember(NULL),NullObjectPtrMember(NULL)
	{

	};

	TestClass(double d, const std::string &s) : DoubleMember(d), StringMember(s), ObjectPtrMember(NULL),NullObjectPtrMember(NULL)
	{
		
		//above initialisation covers
		//double, string, null pointer
		
		//base
		baseOne = 55;
		m_baseTwo = "base";
		mBaseObjThree.x = 11;
		mBaseObjThree.y = 12;

	    //pointer to object
		ObjectPtrMember = new MyClass(401, 402);

		//object member is already initialized.
		ObjectMember.x = 101;
		ObjectMember.y = 102;

		//list of objects
		ObjectVector.resize(3);	//add three elements and call default c'tr for each
		ObjectVector[0].x = 201;
		ObjectVector[0].y = 202;
		ObjectVector[1].x = 203;
		ObjectVector[1].y = 204;
		ObjectVector[2].x = 205;
		ObjectVector[2].y = 206;

		//list of integers
		IntVector.push_back(301);
		IntVector.push_back(302);
		IntVector.push_back(303);
		IntVector.push_back(304);

		//list of strings
		StringVector.push_back("hello");
		StringVector.push_back("world");

		//pointer to object
		ObjectPtrMember = new MyClass(401,402);

		//list of pointers to objects
		//with one NULL pointer in the list
		ObjectPtrVector.push_back(new MyClass);
		ObjectPtrVector.push_back(NULL);
		ObjectPtrVector.push_back(new MyClass);
		ObjectPtrVector[0]->x = 501;
		ObjectPtrVector[0]->y = 502;
		//ObjectPtrVector[1] is null.
		ObjectPtrVector[2]->x = 503;
		ObjectPtrVector[2]->y = 504;

		//smart pointer
		//SharedPtrMember = boost::shared_ptr<MyClass>(new MyClass);
		//SharedPtrMember->x = 601;
		//SharedPtrMember->y = 602;

		//null smart pointer - already initialized

		//vectors of smart pointers
		//SharedPtrVector.push_back(boost::shared_ptr<MyClass>(new MyClass));
		//SharedPtrVector[0]->x = 701;
		//SharedPtrVector[0]->y = 702;
		//SharedPtrVector[1]->x = 703;
		//SharedPtrVector[1]->y = 704;

		//vector of vectors
		vectorOfVectors.resize(3);	//3 vectors of size 0.
		vectorOfVectors[0].resize(2);	//first vector now size 2
		vectorOfVectors[1].resize(3);	//first vector now size 3
		vectorOfVectors[2].resize(4);	//first vector now size 4
		//now populate the items in the vectors
		vectorOfVectors[0].operator[](0).x = 801;
		vectorOfVectors[0].operator[](0).y = 802;
		vectorOfVectors[0].operator[](1).x = 801;
		vectorOfVectors[0].operator[](1).y = 802;

		vectorOfVectors[1].operator[](0).x = 803;
		vectorOfVectors[1].operator[](0).y = 804;
		vectorOfVectors[1].operator[](1).x = 805;
		vectorOfVectors[1].operator[](1).y = 806;
		vectorOfVectors[1].operator[](2).x = 807;
		vectorOfVectors[1].operator[](2).y = 808;

		vectorOfVectors[2].operator[](0).x = 809;
		vectorOfVectors[2].operator[](0).y = 810;
		vectorOfVectors[2].operator[](1).x = 811;
		vectorOfVectors[2].operator[](1).y = 812;
		vectorOfVectors[2].operator[](2).x = 813;
		vectorOfVectors[2].operator[](2).y = 814;
		vectorOfVectors[2].operator[](3).x = 815;
		vectorOfVectors[2].operator[](3).y = 816;
		
		//string int map
		stringIntmap.insert(std::make_pair<std::string, int>("First", 1));
		stringIntmap.insert(std::make_pair<std::string, int>("Second", 2));

		//string object maps
		StringObjmap.insert(std::make_pair<std::string, MyClass>("First Object",MyClass() ));
		StringObjmap.insert(std::make_pair<std::string, MyClass >("Second Object", MyClass() ));

		//string object pointer map
		StringObjPtrmap.insert(std::make_pair<std::string, MyClass*>("One",new MyClass() ));
		StringObjPtrmap.insert(std::make_pair<std::string, MyClass*>("Two", new MyClass() ));

	}

	virtual ~TestClass()
	{
		//std::cout<<"TestClass destructor...in\n";
		//clean up our raw pointers
		delete ObjectPtrMember;	

		//delete our array of object pointers
		for (std::vector<MyClass*>::iterator iter = ObjectPtrVector.begin();
			iter != ObjectPtrVector.end(); ++iter)
		{
			delete *iter;
			*iter = NULL;
		}

		//delete our map of object pointers
		//std::cout<<"Testclass destructor OUT\n";
	};

private:	
 	TestClass(const TestClass&rhs);	//no copies

	//this means that we define our members a bit differently using this syntax. 
	//if we have a pre-generated class then we replace the semicolons with commas (apart from the last one)
	//and wrap it in the GCSSREFLECT macro and wrap the types with ( ) as per the below
	//when the class expands, it will have members just the same as usual with no difference.
	//this is so that we can use the C++ preprocessor to generate our metadata for us about this class.

	//The reason that we wrap the type in parentheses is so that we can easily separate the type from the variable name
	//even when the type is two symbols - for example (int *) p_Ptr would otherwise be confusing for the parser.
	
	//to use JSON serialization you will need this macro
	//if you don't need it, then don't put this in.
private:
	    XREFLECT
		(
			(double) DoubleMember,
			(std::string) StringMember,
			(MyClass) ObjectMember,
			(std::vector<MyClass>) ObjectVector,
			(std::vector<int>) IntVector,
			(std::vector<std::string>) StringVector,
			(MyBase *) ObjectPtrMember,
			(MyClass*) NullObjectPtrMember,
			(std::vector<MyClass *>) ObjectPtrVector,
			(std::vector<std::vector<MyClass> >) vectorOfVectors,
			(std::map<std::string, MyClass*>) StringObjPtrmap,
			(std::map<std::string,int>) stringIntmap,
			(std::map<std::string, MyClass>) StringObjmap,
			(std::vector<MyClass>) EmptyVector,
			(std::map<std::string,MyClass>) EmptyMap
		)//END GCSSREFLECT
		SERIALIZE_JSON_DERIVED(TestClass,base)
		
/* Fix smart pointer support for std::shared_ptr instead
		(boost::shared_ptr<MyClass>) SharedPtrMember,
		(boost::shared_ptr<MyClass>) nullSmartPtr,
		(std::vector<boost::shared_ptr<MyClass> >) SharedPtrVector,
*/


//if you get a compilation error like this:
//: warning: not enough actual parameters for macro 'BOOST_PP_SEQ_SIZE_16' 
//then that means that you accidentally put a comma or a semicolon at the end of the last member
//look carefully above.

	//Q. why does our test class have these members? 
	//A. they exist to satisfy the following tests: 
	//simple double 
	//simple string
	//Included Object
	//Null pointer
	//pointer to valid object
	//vector of objects
	//vector of ints
	//vector of strings
	//smart pointer 
	//null smart pointer
	//vector of pointers to objects with one null entry in the vector
	//vector of smart pointers
	//vector of vectors
	//string int map
	//string object map
	//string object ptr map
	//empty vector
	//empty map

};
REFLECTION_REGISTER(TestClass)

