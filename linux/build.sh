set -e

cd /PRMQ/linux
rm -r ./tmp | true
mkdir tmp
cd tmp
if [[ "$1" == "" ]]; then
    echo build Debug
    cmake -DCMAKE_BUILD_TYPE=Debug ..
else
    echo build Release $2 $1
    cmake -DCMAKE_BUILD_TYPE=Release -DVERSION=$1 -DNAME_POSTFIX=$2 ..
fi
cmake --build .
RMQ_HOST=rabbitmq ctest -V
