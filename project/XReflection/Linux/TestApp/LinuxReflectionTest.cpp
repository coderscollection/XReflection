//
//November 2018 - John Nessworthy

#include "LinuxReflectionTest.h"

#include <fstream>
//stl
#include <string>
#include <iostream>
//-----------TEST APPLICATION-----------------------------------

//just for linux debugging - rather than logging to a stringstream and then outputting at the end
//we need to log as we go - because if it crashes, we won't see what went wrong.
class debugstream
{
public:
	debugstream(){};
	template<typename U> debugstream & operator << (U in)
	{
	   cout << in;
	   //m_os << in;
	  return *this;	
	}
	//std::string str()
	//{
	//   return m_os.str();
	//}		
private:
	//stringstream m_os;
};


//This small test program tests and demonstrates how to manipulate our class
//class dynamically using reflection in C++
int main()
{
	bool bPassed = true;
	debugstream dbg;								//open our test stream
	dbg << "---Start of Test---\n";
	try
	{
		//----------------------------------------
		dbg << "\n----------------------\n";
		dbg << "Case 1.0 - Very simple initial object creation and JSON construction \n";
		stringstream ahs;
		MyClass foo;
		foo.x=6;
		foo.y=7;
		foo.z=8;
		foo.MyBaseDouble=5;
		foo.MyBaseLong=19;
		foo.SuperBool=true;
		foo.SuperInt=666;
		foo.listOfItems.resize(3);	//stick 3 blank items in our object
		foo.toJSON(ahs);		//output it
		
		dbg << "JSON Created.\n";
		std::string simplesourceJSON = ahs.str();	
		dbg << simplesourceJSON << "\n";	//and output the JSON
		
		//--------------------------------------
		//test case 1.1b
		dbg << "\n----------------------\n";
		dbg << "Test Case 1.1b\n";
		dbg << "Test  JSON object serialize and then deserialize into an empty object and comparison check\n";
	
		stringstream ajs;
		foo.toJSON(ajs);	
		dbg << "..serialized to JSON. Now deserializing into target object...\n";
		MyClass bar;
		bar.fromJSON(ajs);

		dbg << "Deserialized into object.\n";
		std::stringstream aks;
		bar.toJSON(aks);
		dbg << "Now examining target JSON from that object...";
		std::string simpletargetJSON = aks.str();
		
		dbg << "\n";
		if (simplesourceJSON == simpletargetJSON)
			dbg << "\nSource and target JSON same hence objects assumed same.  Test passed\n";
		else
		{
			dbg << "\nTARGET JSON\n" << simpletargetJSON << "\n";
			dbg << "\nSource and target JSON not same. Target JSON above. Test FAILED\n";
			bPassed = false;
		}

		//-------------------------------
		dbg << "\n-------\n";

		dbg << "Case 1.2 - complex initial object state and JSON construction..\n";
		//case 0 - initial object state - use special characters in our string
		//to test handling of quotes in the string: " 
		//set object by construction
		stringstream hs;
		TestClass p(3.141, "initial value special chars in text \"><{( - create the JSON ");	
		p.toJSON(hs);		//output it
		
		dbg << "JSON Created.\n";
		std::string sourceJSON = hs.str();	
		dbg << sourceJSON << "\n";	//and output the JSON
		
		//--------------------------------------
		//test case 1.3
		dbg << "\n----------------------\n";
		dbg << "Test Case 1.3\n";
		dbg << "Test the above Complex JSON object serialize and deserialize into an empty object and comparison check\n";
	
		stringstream js;
		p.toJSON(js);	
		dbg << "..serialized to JSON stream. Now deserializing into target object...\n";
		TestClass q;
		q.fromJSON(js);

		dbg << "Deserialized OK. \n";
		std::stringstream ks;
		q.toJSON(ks);
		std::string targetJSON = ks.str();
		
		dbg << "\n";
		if (sourceJSON == targetJSON)
			dbg << "\nSource and target JSON same hence objects assumed same.  Test PASSED\n";
		else
		{
			dbg << "\nTARGET JSON\n" << targetJSON << "\n";
			dbg << "\nSource and target JSON not same. Target JSON above. Test FAILED\n";
			bPassed = false;
		}

		//--------------------------------------
		//test case 2
		//de-serialize into populated objects - we expect it to refuse to work and throw.
		dbg << "\n----------------------\n";
		dbg << "\nTest Case 2\n";
		dbg << "Deserialize into an already populated object\n";
		
		TestClass populated(6.66, "another value");
		try
		{
			stringstream ls;
			ls << sourceJSON;
			populated.fromJSON(ls);
			dbg << " Already populated test FAILED. An exception should be thrown.\n";
			bPassed = false;
		}
		catch (std::runtime_error &ref)
		{
			dbg << ref.what() << "\n";
			dbg << "This is expected. Already populated test PASSED\n";
		}
		//---------------------------------------------
		//test case 3
		//set the double member using the member function.
		std::string doublefieldName = "DoubleMember";		//name of our double member
		dbg << "\n----------------------\n";
		dbg << "\nTest Case 3\n";
		dbg << "Write the double member: " << doublefieldName << " dynamically to a value of 199.8 \n";

		stringstream ms;
		//p.DoubleMember=199.8;
		p.fieldByName<double>(doublefieldName) = 199.8;   //will throw if not found
		//The above line is effectively identical to p.DoubleMember = 199.8 but whilst this is compile-time the above line is run-time.

		p.toJSON(ms);		//output all member variables to the stream (again)
		
		dbg << " The JSON for the object is shown below. Check the field has changed.\n";
		dbg << ms.str() << "\n";
		//--------------------------------------

		//test case 4
		//write the double member dynamically
		stringstream ns;

		dbg << "\n----------------------\n";
		dbg << "\nTest Case 4\n";
		double testvalue = 22.2;
		dbg << "Write the double member: " << doublefieldName << " dynamically to a value of "<< testvalue<<" \n";
		
		p.fieldByName<double>(doublefieldName) = testvalue;		//will throw if not found
		p.toJSON(ns);		//output all member variables to the stream (again)
		
	    dbg << " The JSON for the object is shown below. Check the field has changed.\n";
    	dbg << ns.str() << "\n";


		//--------------------------------------
		//test case 5
		//read the double member dynamically & check it
		dbg << "\n----------------------\n";
		dbg << "\nTest Case 5\n";
		dbg << "Read the double member: " << doublefieldName << " dynamically\n";
		double c = p.fieldByName<double>(doublefieldName);		//will throw if not found
		dbg << "read double value: " << c << "\n";
		//check it's the same
		if (c == testvalue)
		{
			dbg << "Dynamic read/write double test PASSED";
		}
		else
		{
			dbg << "Dynamic read/write double test FAILED";
			bPassed = false;
		}
		//--------------------------------------
		stringstream pt;
		//test case 6
		//write the string member dynamically
		std::string stringFieldName = "StringMember";		//name of our string member
		std::string stestvalue = "test case four with quote \" ";
		dbg << "\n----------------------\n";
		dbg << "\nTest Case 6\n";
		dbg << "Write the string member: " << stringFieldName << " dynamically to a value of "<< stestvalue << " and output object using free function\n";

		p.fieldByName<std::string>(stringFieldName) = stestvalue;
		XReflection::toJSON(p,pt);		//output all member variables to the stream (again) but using free function this time.
		
		dbg << " The JSON for the object is shown below. Check the field has changed.\n";
                dbg << pt.str() << "\n";

		//--------------------------------------
		//test case 7
		//read the string member dynamically using the free function classes & check it
		dbg << "\n----------------------\n";
		dbg << "\nTest Case 7\n";
		dbg << "Read the string member: " << stringFieldName << " dynamically using free function\n";
		std::string e = XReflection::fieldByName<std::string>(p,stringFieldName);			//read the member will throw if not found
		
		dbg << "Read string value: " << e << "\n";

		//check it's the same
		if (e == stestvalue)
		{
			dbg << "Dynamic read/write string test PASSED";
		}
		else
		{
			dbg << "Dynamic read/write string test FAILED";
			bPassed = false;
		}

		//----------------missing data cases--------------
		//to do - once I have made the code support a 'missing data' option. 


		//------------------------------error cases-----------------------
		dbg << "\n---Error Cases---\n";

		//----------------
		dbg << "\n----------------------\n";
		dbg << "Here we are going to dyanmically access a string member variable as a double. We expect a run-time error.\n";
		//access the string member as a double using a free function
		try
		{
			double error = XReflection::fieldByName<double>(p, stringFieldName);			//we expect this to throw with an error 'field found but unepected type'
			dbg << "Wrong Type Test FAILED value on double is: "<<error<< "\n";
			bPassed = false;
		}
		catch (std::runtime_error &ref)
		{
			dbg << ref.what() << "\n Wrong Type Test PASSED\n";
		}

		dbg << "\n----------------------\n";
		dbg << "Here we are going to dyanmically access a  member variable that doesn't exist.  We expect a run-time error.\n";
		//access a member that doesn't exist using a member function
		std::string badFieldName = "m_NotThere";		//bad name of member
		try
		{
			double errorTwo = p.fieldByName<double>(badFieldName);			////we expect this to throw with 'field name not found' 
			dbg << "Non Existent Member Test FAILED. Value returned was: "<<errorTwo<<" \n";
			bPassed = false;
		}
		catch (std::runtime_error &ref)
		{
			dbg << ref.what()<<"\n Non Existent Member Test PASSED\n";
		}

		//----end of tests----
	}
	catch (std::runtime_error &ref)
	{
		dbg << "Error: Caught Exception in test program:  " << ref.what() << "\n";
		bPassed = false;
	}
	catch (...)
	{
		dbg << "Error Unknown Exception caught in test program";
		bPassed = false;
	}

	if (bPassed)
		dbg << "\n ****Great! All tests PASSED!****\n";
	else
		dbg << "\n at least one test FAILED";
	//----------------------------------
//the output of the entire test pack has also been output to os
	//std::cout<< "The entire test results can be written to file here by accessing them from dbg.str() ";
	//if you want write it
	//
	std::cout << "\n Type any character and hit enter to continue (like y)\n";
	char c;
	std::cin >> c;
    return !bPassed; //0 for ok
}


