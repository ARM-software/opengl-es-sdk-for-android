#!/bin/bash

VERSTRING=`cat VERSION`
VERSION=$VERSTRING
export VERSION
echo $VERSION

cd doxygen
exec doxygen
