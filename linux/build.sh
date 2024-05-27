set -e

cd /PRMQ/linux
rm -r ./tmp | true
mkdir tmp 
cd tmp
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
ctest -V 