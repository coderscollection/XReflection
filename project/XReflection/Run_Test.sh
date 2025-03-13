#!/bin/bash
pushd .
printf "+++++++++Running Host Unit Test+++++++++++\n\n"
printf "+++++++++Removing old build \n\n"
rm -rf build
mkdir build
cp testfile.json build/
printf "+++++++++Creating Debug Makefiles\n\n"
cd build
cmake DCMAKE_BUILD_TYPE=Debug ../
printf "\n\n+++++++++Building debug test code\n\n"
make
printf "\n\n+++++++++Running Test App\n\n"
./ReflectionTest
printf "\n\n All done. Look in 'build' folder for binary and any core files.\n\n"
popd

