#!/bin/bash

# Ucieszony Chat Client
# Copyright (C) 2013-2015 Paweł Ostrowski
#
# This file is part of Ucieszony Chat Client.
#
# Ucieszony Chat Client is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# Ucieszony Chat Client is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Ucieszony Chat Client (in the file LICENSE); if not,
# see <http://www.gnu.org/licenses/gpl-2.0.html>.

if [ ! -d src/ ]; then
    echo -e "Nie znaleziono src/"
    exit 1
fi

mkdir -p obj/Release/src/

echo -e "cd src/"
cd src/

CPP_ALL=$(ls *.cpp 2>/dev/null | xargs)

HOW=$(echo -e ${CPP_ALL} | wc -w)

if [ ${HOW} = "0" ]; then
    echo -e "Nie znaleziono żadnych plików *.cpp"
    exit 2
fi

for FILE in ${CPP_ALL}; do
    COMPILER="g++ -std=c++11 -Wall -O2 -c ${FILE} -o ../obj/Release/src/${FILE}.o"
    echo -e ${COMPILER}
    $(${COMPILER}) || exit 3
done

echo -e "cd ../"
cd ../

mkdir -p bin/Release/

if [ `uname -o` = "Cygwin" ]; then
    LINKER="g++ -o bin/Release/ucc $(ls obj/Release/src/*.o | xargs) -lncursesw -lssl -liconv -s"
else
    LINKER="g++ -o bin/Release/ucc $(ls obj/Release/src/*.o | xargs) -lncursesw -lssl -s"
fi

echo -e ${LINKER}
$(${LINKER})
