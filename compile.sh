#!/bin/bash

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
    COMPILER="g++ -Wall -std=gnu++11 -O2 -c ${FILE} -o ../obj/Release/src/${FILE}.o"
    echo -e ${COMPILER}
    $(${COMPILER}) || exit 3
done

echo -e "cd ../"
cd ../

mkdir -p bin/Release/

LINKER="g++ -o bin/Release/ucc $(ls obj/Release/src/*.o | xargs) -lncursesw -lssl -lcrypto -s"
echo -e ${LINKER}
$(${LINKER})
