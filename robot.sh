#!/bin/bash

# prepare header files for libreadline
cd readline
bash install

# compile lua
cd lua-mirror
make clean
make linux
cd ..

# compile boost library
cd boost_1_55_0
chmod +x bootstrap.sh
chmod +x tools/build/v2/engine/build.sh  
./bootstrap.sh --with-libraries=filesystem,regex,thread --prefix=/usr/local
chmod +x ./b2
./b2 install
ldconfig
cd ..

# compile horos(tcpgo)
make
