#pragma once

//XReflect
//(c) John Nessworthy 2019.  Email ideaboxconsult@gmail.com
//C++ reflection and JSON serialization Library

//This software does not come with any warranty express or implied and is to be used at your own risk.
//The author does not accept any responsbility for an losses due to it's use, however they are incurred.
//You are free to compile and use this code in your own application but please give me credit and do
//not claim that the work is your own.
//----------------------


#include <ostream>      //ostream support -  to JSON
#include <istream>      //istream support - from JSON
#include <iomanip>      //std::quoted

//support for smart pointer types - add headers here as needed
#include <boost/shared_ptr.hpp>

//support for STL containers - add headers here as needed
#include <vector>
#include <map>
#include <stdexcept>
//----------------------------

using namespace std;
//This file contains extensions to support  Common STL Containers, Boost smart pointers
//and new  types, new template containers etc that we need to support.

//to add a new simple type
//1. The type must support << and >>
//2. If it doesn't you will need to add an declaration of
//friend istream& XReflection::_impl::operator >> (istream &in, YourObject* obj); NOTE - pointer 
//friend ostream& XReflection::_impl::operator<< (ostream &out, YourObject& obj); NOTE - Reference
//in the below Macro AND an implementation in this file.

//to add a new template type.
//1. Implement the << and >> operators for it below.
//2. Declare as friends under the SERIALIZE_JSON_OUTPUT_EXT and  SERIALIZE_JSON_INPUT_EXT macro below
//3. Note that for the << operator you must use references but for the >> operator you must use pointers.
//4. When de-serializing into a container of things using >> take care to use the pointer overload for the referenced object.
//5. For containers like maps that don't allow in-place edits, you must create tempories. See the Deserializer template & map implementation below.
//6. Ensure that your << operators always take const references as below.
//7. Because of the techniques used below, you can use one << and one >> for containers of 'things' whether they are objects, pointers or a mixture.

//JSON output of a class. Add friend declaration below.
#define SERIALIZE_JSON_OUTPUT_EXT(classnam)\
template<typename U> \
friend ostream& XReflection::_impl::operator << (ostream &out, boost::shared_ptr<U> const & p);\
template <typename U> \
friend ostream& XReflection::_impl::operator << (ostream &out, std::vector<U> const & set);\
template <typename U,typename V> \
friend ostream& XReflection::_impl::operator << (ostream &out, std::map<U,V> const & map);\

//JSON input of a class. Add friend declaration below.
#define SERIALIZE_JSON_INPUT_EXT(classnam)\
template <typename U> \
friend istream& XReflection::_impl::operator >> (istream &in, boost::shared_ptr<U>* p);\
template <typename U> \
friend istream& XReflection::_impl::operator >> (istream &in, std::vector<U>* set);\
template <typename U,typename V> \
friend istream& XReflection::_impl::operator >> (istream &in, std::map<U,V>* map);\
//======================JSON output of a class=========================

namespace XReflection
{
        namespace LITERALS
        {
                //additional literals used in this file only but also see the literals in XReflectionJSONImpl.h
                static const char STARTARRAY = '[';
                static const char ENDARRAY = ']';
				static const std::string MAPKEYFIELD = "key";
				static const std::string MAPVALUEFIELD = "val";
        }

        using namespace XReflection::LITERALS;

        namespace _impl
        {

//===================JSON output from an object (serialize) =======================

//support for boost::shared_ptr
                template<typename U> ostream& operator << (ostream &out,boost::shared_ptr<U> const &p)
                {
                        out << p.get();         //treat our smart pointer as a normal pointer.
                        return out;
                }

//support for std::vectors of things (as JSON arrays)
                template <typename U> ostream& operator<< (ostream &out, std::vector<U>const & set)
                {
                        out << STARTARRAY;
                        if (set.size()) //only serialize if the vector is empty.
                        {
                                typename std::vector<U>::const_iterator iter = set.begin();
                                do
                                {
                                        out << (*iter); //call the << operator on the 'thing'
                                        if (++iter != set.end()) out << COMMA;  //comma if not last one
                                } while (iter != set.end());
                        }
                        out << ENDARRAY;
                        return out;
                }

//support for std::maps
//we expect every item in the map to be
//[ {"key" : {object for first} , "val" : {object for second} } , { ......}  ]  
//and NOT [{key,value} , {key,value} ]

                template <typename U, typename V> ostream& operator << (ostream &out, std::map<U,V>const & map)
                {
                        out << STARTARRAY;
                        if (map.size()) //only serialize if the map is not empty.
                        {
                                typename std::map<U, V>::const_iterator iter = map.begin();
                                do
                                {       //note. It's important for the overloaded << operator for our type within the map to work on a const object
                                        out << STARTOBJECT << MAPKEYFIELD << COLON << ((*iter).first) << COMMA << MAPVALUEFIELD << COLON << ((*iter).second) << ENDOBJECT;
                                        if (++iter != map.end()) out << COMMA;  //comma if not last one
                                } while (iter != map.end());
                        }
                        out << ENDARRAY;
                        return out;
                }
        } //end _impl namespace
//=================================Helper functions for deserialization=========================
//also see the other helper functions in XReflectionJSONImpl.h

        namespace
        {
                //-------helper function for our STL types---------
                //expects the stream to be pointing at [ character
                //will return true if the stream is [] (empty array) & false if the stream is [ (full array)
                //will throw an exception if the stream isn't pointing to [ at all.
                //will advance the stream over either the [ or both the [] chars if they exist otherwise unchanged.

                bool IsEmptyArray(istream &in)
                {
                        char discard;
                        if (!ReadNextCharIs(in, STARTARRAY)) throw std::runtime_error("Expected [ start of array");
                        if (in.peek() != ENDARRAY) return false;        //was not empty array
                        in >> discard;          //throw away ] character
                        return true;            //was empty array
                }
        }

        namespace _impl //implementation namespace
        {
//=====================JSON input to an object (deserialize) ======================

                //support for boost::shared_ptr  
                template <typename U> istream& operator >> (istream &in, boost::shared_ptr<U>* p)
                {
                    //if the JSON value is null then don't attempt to manipulate the shared_ptr
                    if (!IsNull(in))                //internal helper function in JSonImpl.h file
                    {
                        //otherwise we  need to create a new object. Serialize into it and then assign it to our shared_ptr
                        U* tmp = XReflection::internalcreatehelper<U>(in); //read and create it
                        in >> tmp;              //deserialize fields into it
                        *p = boost::shared_ptr<U>(tmp); //and assign to our member pointer that's pointed to by our incoming pointer
                    }
                    return in;
                }

                //support for std::vectors of things (as JSON arrays)
                template <typename U> istream& operator >> (istream &in, std::vector<U>* set)
                {
                    if (set->size()) throw std::runtime_error("Unable to deserialize JSON into an existing vector. Ensure vector is empty");
                    //technically we could serialize into an existing vector but have disallowed it to stay consistent with map and because it's not that useful anyways.
                    if (IsEmptyArray(in)) return in;        //if empty, then return (our object already empty)
                    char nextchar;
                    bool bContinue = true;
                    //Now create items for all elements in our JSON array until our array ends with a ]
                    while (bContinue)
                    {
                        set->resize(set->size() + 1);   //add one item and call default 'ctor for it
                        U& ref = set->back();   //reference to object
                        in >> &ref;				//use pointer overload of >>                         
                        in >> nextchar;         //next character - can be , or ] 
                        if (nextchar == ENDARRAY) bContinue = false;     //if at the end of the JSON then we are done.
                        else if (nextchar != COMMA)throw std::runtime_error("expected , in array of objects JSON");
                    }
                    return in;
                }
                
                //support for std::maps of things (as JSON arrays)
                template <typename U, typename V> istream& operator >> (istream &in, std::map<U, V>* map)
                {
                        if (map->size()) throw std::runtime_error("Unable to deserialize JSON into an existing map. Ensure map is empty");
                        //You can't change std::map entries in place.
                        if (IsEmptyArray(in)) return in;                //if this is an empty array then don't proceed (our object already empty)
                                                                        //this will already skip the [ character
                        U firstItem;    //note. U,V  might be a reference or a pointer.
                        V secondItem;   //we use a trick with the Deserializer template to differentiate between pointers and references
                        //so that we can get away with creating a temporary here. We have to create temporaries because we
                        //can't create the object 'in place' like a vector and then de-serialize into it.
                        //We have to actually create a 'thing' (which could be a pointer OR an object) and then work with it.
                        //the trick is - how do we know if it's a pointer or an object? Well. This is how you do it when it matters.
                        char nextchar;
                        bool bContinue = true;
                        do
                        {
				//we expect every item in the map to be
				//[ {"key" : {object for first} , "val" : {object for second} } , { ......}  ]  
				//and NOT [{key,value} , {key,value} ]

                                if (!ReadNextCharIs(in,STARTOBJECT) ) throw std::runtime_error("expected opening { in map array JSON");
				std::string keyFieldName, valueFieldName;
				ReadString(in, keyFieldName);//read the key field name
				if (keyFieldName!= MAPKEYFIELD) throw std::runtime_error("unexpected key field name in map");
				if (!ReadNextCharIs(in, COLON)) throw std::runtime_error("Expected : between map key field name and it's value");
                    		Deserializer<U>::in(&firstItem, in);    //conditionally deserialize 'key field' value (object or pointer)
                                if (!ReadNextCharIs(in, COMMA)) throw std::runtime_error("expected , in map array JSON");
				ReadString(in, valueFieldName);//read the value field name
				if (valueFieldName!= MAPVALUEFIELD) throw std::runtime_error("unexpected value field name in map");
				if (!ReadNextCharIs(in, COLON)) throw std::runtime_error("Expected : between map value field name and it's value");
                                Deserializer<V>::in(&secondItem, in);   //conditionally deserialize the 'value field' value (object or pointer)
                                std::pair<typename std::map<U,V>::iterator, bool> ret = map->insert(typename std::map<U, V>::value_type(firstItem, secondItem)); //insert our item/pointer
                                if (!ret.second) throw std::runtime_error("JSON contained duplicate entries in map. Could not insert");
                                if (!ReadNextCharIs(in, ENDOBJECT)) throw std::runtime_error("missing closing } in map array JSON for an entry");
                                in >> nextchar;
                                if (nextchar == ENDARRAY) bContinue = false;     //if at the end ofthe JSON then we are done.
                                else if (nextchar != COMMA) throw std::runtime_error("expected , in map array JSON between entries");
                        } while (bContinue);
                        return in;
                }
        } //end _impl namespace
} //end XReflection namespace

