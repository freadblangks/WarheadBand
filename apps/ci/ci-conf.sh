#!/bin/bash

set -e

cat >>conf/config.sh <<CONFIG_SH
MTHREADS=$(($(grep -c ^processor /proc/cpuinfo) + 2))
CWARNINGS=ON
CDEBUG=OFF
CTYPE=Release
CSCRIPTS=static
CMODULES=$2
CBUILD_TESTING=ON
CSERVERS=ON
CTOOLS=ON
CSCRIPTPCH=$1
CCOREPCH=$1
CCUSTOMOPTIONS='-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_FLAGS="-Werror" -DCMAKE_CXX_FLAGS="-Werror"'
DB_CHARACTERS_CONF="MYSQL_USER='root'; MYSQL_PASS='root'; MYSQL_HOST='localhost';"
DB_AUTH_CONF="MYSQL_USER='root'; MYSQL_PASS='root'; MYSQL_HOST='localhost';"
DB_WORLD_CONF="MYSQL_USER='root'; MYSQL_PASS='root'; MYSQL_HOST='localhost';"
CONFIG_SH

if [[ $ENABLE_CPP_20 -eq 1 ]]; then
  echo "CUSE_CPP_20=ON" >> ./conf/config.sh
fi

# Start mysql
sudo systemctl start mysql

case $COMPILER in

  # this is in order to use the "default" clang version of the OS, without forcing a specific version
  "clang" )
    time sudo apt-get install -y clang
    echo "CCOMPILERC=\"clang\"" >> ./conf/config.sh
    echo "CCOMPILERCXX=\"clang++\"" >> ./conf/config.sh
    ;;

  "clang11" )
    time sudo apt-get install -y clang-11
    echo "CCOMPILERC=\"clang-11\"" >> ./conf/config.sh
    echo "CCOMPILERCXX=\"clang++-11\"" >> ./conf/config.sh
    ;;

  "clang12" )
    time sudo apt-get install -y clang-12
    echo "CCOMPILERC=\"clang-12\"" >> ./conf/config.sh
    echo "CCOMPILERCXX=\"clang++-12\"" >> ./conf/config.sh
    ;;

  * )
    echo "Unknown compiler $COMPILER"
    exit 1
    ;;
esac
