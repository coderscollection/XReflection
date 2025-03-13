#!/bin/bash
#this script is run from inside our build container
#Basic build script - (c) John Nessworthy all rights reserved.

pushd .

#----get rid of the old build----
printf "+++++++++Removing old build \n\n"
rm -rf build
mkdir build

#-----run cmake to get the makefiles-------
printf "++++++++creating debug makefiles\n\n"
cd build
cmake DCMAKE_BUILD_TYPE=Debug ../

#-----build it------------
printf "\n\n+++++++++Building debug test code\n\n"
make

popd
printf "\n Done. Binary is in 'build' folder"



