XReflection library and test.
(c) John Nessworthy 2020. 
No rights to copy, reproducem  alter or use this code in any form please.  For information only.
---------------


Instructions
------------

git clone this repo

run ./start.sh to create the docker container and will drop into an interactive shell as the build user.

cd ~

run ./build.sh to create the makefiles and build the binaries.
(the binary and all other object files are present in the created 'build' folder)

execute the binary 'ReflectionTest' in the build folder.


More notes on the reflection code if you are interested
-------------------------------------------------------
This code is a work in progress. I expect to rewrite it some day. I share it so give an illustration of an interesting piece of work I designed.

This can be found in the XReflection directory. The code for the framework is in 'Include' and the test programs are in 'Linux'. There is also a windows test program that I didn't share - I wrote the headers to work on both platforms.   

I wanted to write a header-only library that would allow me to represent any c++ object as a JSON message, and automatically serialize a JSON stream to an appropriately structured object.  This is useful because it allows us to easily load and save a jSON configuration file, for example, or easily represent a set of incoming JSON messages as C++ objects.

The framework supports complex classes, single inheritance and containers. It also supports abstract base classes. 

This is useful - because if have (say) 10 different sorts of JSON message that can arrive, if they all inherit from the same base class then the framework will interpret the message, create the correct object, and give the developer the object via a base class pointer. That means the code that uses the framework doesn't even need to know what messages are coming in. Just deal with objects.

There are a few limitations. It won't truly support multiple inheritance for example - and it won't support reference member variables because there is no way to create an empty object and then (re)initialize one of those from some JSON.  It also currently writes "" for empty string variables rather than skipping them. 

The code uses boost MPL, templates and the preprocessor to automatically create fromJSON and toJSON methods for any c++ class. The only necessity, is that the member variables of the class (which can be any type - including container types) need to be defined using the correct syntax. 

There are lengthy instructions in XReflection/instructions.txt.
