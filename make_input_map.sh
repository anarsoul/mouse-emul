#!/bin/sh

if [ ! -e $1 ]; then
	echo "$1 is missing!"
	exit 1
fi
echo "/* This file is autogenerated, DO NOT EDIT! */"
echo "#ifndef INPUT_MAP_H"
echo "#define INPUT_MAP_H"
echo ""
echo "/*"
echo " *  Copyright (C) 2012 Vasily Khoruzhick (anarsoul@gmail.com)"
echo " *"
echo " *  This program is free software; you can redistribute it and/or modify"
echo " *  it under the terms of the GNU General Public License as published by"
echo " *  the Free Software Foundation; either version 2 of the License, or"
echo " *  (at your option) any later version."
echo " */"
echo ""
echo "struct uint_str_tuple linux_input_map[] = {"
grep BTN_ $1 | awk '{ print "{\""$2"\"" ", " $3 "},"}'
grep KEY_ $1 | awk '{ print "{\""$2"\"" ", " $3 "},"}'
grep SW_ $1 | awk '{ print "{\""$2"\"" ", " $3 "},"}'
echo "};"
echo ""
echo "#endif"