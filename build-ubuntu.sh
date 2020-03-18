#!/bin/sh

DEBUG='-f Makefile.Debug'
RELEASE='-f Makefile.Release'

make_function()
{
    qmake
    make $1 distclean
    qmake 
    make $1 clean
    make $1 -j 2
    make $1 install
}

echo Usage: "build-xxx.sh release/debug/all/clean/\"\" "
echo "      release|\"\" : Make version release        "
echo "      debug      : Make version debug            "
echo "      all        : Make version debug and release"
echo "      clean      : Make clean                    "

if [ "$1" = "" ]; then 
    make_function "$RELEASE"
fi
if [ "$1" = "release" ]; then 
    make_function "$RELEASE"
fi
if [ "$1" = "debug" ]; then 
    make_function "$DEBUG"
fi
if [ "$1" = "all" ]; then 
    make_function "$RELEASE"
    make_function "$DEBUG"
fi
if [ "$1" = "clean" ]; then 
    qmake
    make distclean
fi

