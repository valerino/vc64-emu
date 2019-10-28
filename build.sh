#!/usr/bin/env sh
# usage: ./build.sh [debug=build with debug symbols|clean=cleanup build|update=git pull]
BUILD_TYPE=RelWithDebInfo
if [ "$1" = "debug" ]; then
    # debug build
    echo ". building debug version"
    BUILD_TYPE=Debug
elif [ "$1" = "clean" ]; then
    # cleanup everything
    echo ". cleaning up"
    cd emushared
    rm -rf build
    cd ../v65xx
    rm -rf build
    cd ..
    rm -rf build
    exit 0
elif [ "$1" = "update" ]; then
    # cleanup everything
    echo ". git update"
    cd emushared
    git pull
    cd ../v65xx
    git pull
    cd ..
    git pull origin dev
    exit 0
else
    # default, release build
    echo ". building release version"
fi

# setup environment vars
export EMUSHARED_LIB_PATH=$(pwd)/emushared/build
export EMUSHARED_INCLUDE_PATH=$(pwd)/emushared
export V65XX_LIB_PATH=$(pwd)/v65xx/build
export V65XX_INCLUDE_PATH=$(pwd)/v65xx

# build emushared
cd emushared
mkdir build 
cd build
cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE
make
if [ $? -ne 0 ]; then
    exit $?
fi

# build v65xx
cd ../../v65xx
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE
make
if [ $? -ne 0 ]; then
    exit $?
fi

# build emu
cd ../../
echo $(pwd)
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE
make



