
//XReflect
//(c) John Nessworthy 2019.  Email ideaboxconsult@gmail.com
//C++ reflection and JSON serialization Library

//This software does not come with any warranty express or implied and is to be used at your own risk.
//The author does not accept any responsbility for an losses due to it's use, however they are incurred.
//You are free to compile and use this code in your own application but please give me credit and do
//not claim that the work is your own.
//----------------------

#include <boost/mpl/range_c.hpp>                        //boost::mpl
#include <boost/mpl/for_each.hpp>
#include <boost/bind/bind.hpp>                          //boost::bind
#include <boost/bind/placeholders.hpp>          //for the placeholders used in boost::bind
#include <typeinfo>                             //typeID
#include <ostream>                              //ostream support for streaming
#include <istream>                              //istream support for streaming
#include <sstream>                              //stringstream support
#include <stdexcept>

using namespace std;

namespace XReflection
{
        //This friend helper allows us to access the field data and field count within the class without the compiler barfing.
        //we have to put the methods in a struct because we can't friend a namespace.. so in they go.
        //we can avoid instantiating it each time by declaring it a non-template struct that happens to contain static template functions which should help our class size/speed slightly.
        class XReflHelper
        {
        public:
                //This effectively returns a dependent type to whatever member_metadata templated structure is at the specific index number 'Index'
                //Now. Because each template is effectively called  member_metadata<N,Whatever the main class Is>
                //it's easy and convenient to use the index number in the template.
                template<int index, typename MainClass>
                static typename MainClass::template member_metadata<index, MainClass> getFieldByIndex(MainClass& x)
                {
                        return typename MainClass::template member_metadata<index, MainClass>(x);
                }

                //This inner class helper (XReflHelper::members<typeName> gives us the field count from the member variable in the main class.
                //that member variable is a static const int which is set to the number of member variables that appear in our initial class declaration (inside XREFLECT)
                //This trick allows us to use ensure that it evaluates to a constant expression which is needed to make it work with boost::mpl
                template<typename MainClass>
                struct members
                {
                        static const int count = MainClass::NumberOfMembers;
                };
        };

        namespace _impl //implementation namespace
        {
                //=================================================
                //methods used fetching a named field (used for dynamic access to member variables)
                class fetchFieldVisitor
                {
                public:
                        fetchFieldVisitor() {};
                        template<typename MainClass, typename Visitor, typename I>
                        void operator()(MainClass& theClass, Visitor &myvisitor, I)
                        {
                                myvisitor(XReflHelper::getFieldByIndex<I::value>(theClass));
                        }
                };

                template<typename DesiredFieldType>
                class fetchVisitor
                {
                public:
                        fetchVisitor(const std::string &name) : m_ptr(NULL),m_name(name) {};   //pass in the field name that we are looking for.
                        template<typename FieldData> void operator()(FieldData f) //will populate the member ptr with the pointer to the member we need if it matches what we want, in name and in type.
                        {
                                if (m_ptr) return;                                                       //found it already. quicker than doing string compare below.
                                if (m_name == std::string(f.name()))                                    //if the name matches what we are looking for...
                                {
                                        DesiredFieldType *dummyPtr = NULL;                                      //create a dummy pointer to our type. We create a pointer and not an object so we don't invoke the 'ctor.
                                        std::string actualType = typeid(&f.value()).name();             //type of a pointer to our actual type
                                        std::string desiredType = typeid(dummyPtr).name();      //type of a pointer to our desired type
                                        #ifdef _WIN32
                                            (dummyPtr);                   //remove compiler warning
                                        #endif
                                        if (actualType == desiredType)          //if the type we want in the call equals the type we actually have, then set our void* pointer to point to it.
                                                m_ptr = (void*)&f.value();     //store a pointer to our member in the visitor. Since we are passing a reference to the visitor around and using boost::ref in the mpl we can then 'see' it from the caller.
                                        else
                                                throw std::runtime_error("Field located but unexpected type");  //let the caller know
                                }
                        }
                        void * m_ptr;           //pointer to void * pointer that receives our data. Due to the templated nature of this we can't use a typed pointer. It won't work (because we don't have the = operator defined for all our possible combinations of conversions)
                private:
                        fetchVisitor(const fetchVisitor &ref) : m_name(ref.m_name), m_ptr(ref.m_ptr) { };       //no copies - prevents problems with boost::mpl if we screw up the boost::bind
                        const std::string &m_name; //name of the field we are looking for
                };
        }//end _impl namespace
} //end namespace

//---------------------(optional) FREE FUNCTION INTERFACE FOR CLIENTS TO USE-------------------------------------------------
//This is used by the generated member functions above so that we can avoid code duplication in the same compilation unit.
//it is not essential to use these free functions. You can use the generated member functions above which share the same name.
//note that implementation methods are held inside XReflection::_impl::
//wherease interface methods are merely in XReflection::

namespace XReflection
{
        //implementation
        namespace _impl
        {
                //visit each member of the class with the passed in visitor
                template<typename MainClass, typename Visitor>
                void visitAllAndFetch(MainClass & c, Visitor &v)
                {
                        //iterate through all the fields in the class and for each member, we will call our visitor
                        typedef boost::mpl::range_c<int, 0, XReflHelper::members<MainClass>::count> members;
                        //note that we need to use boost::ref for a reference type so that we don't create a copy accidentally and note that we need a public copy c'tor in our visitor
                        boost::mpl::for_each<members>(boost::bind<void>(fetchFieldVisitor(), boost::ref(c), boost::ref(v),boost::placeholders::_1));
                }//note boost 1_59 is _1 and 1_64 boost::placeholders::_1
        } //end namespace
 
       //interface
       //fetch a named field of an object passing in a reference to the desired type we think it is.
        template<typename DesiredFieldType, typename MainClass> DesiredFieldType& fieldByName(MainClass& obj, const std::string &name)  //allow the compiler to deduce the type of the Mainclass from the passed in parameter
        {
                _impl::fetchVisitor<DesiredFieldType> theVisitor(name);
                _impl::visitAllAndFetch(obj, theVisitor);     //this will throw if the field is found BUT wrong type.
                if (!theVisitor.m_ptr) throw std::runtime_error("Field name not found");        //null pointer means simply field name not located.
                DesiredFieldType *pointer = (DesiredFieldType*)theVisitor.m_ptr;        //C style cast will work because has been type checked and will work for native types.
                return *pointer;                //return reference to it
        }
}
