source /home/mory/softwares/rk3576/setup-rk3576.sh
rm -rf build
mkdir build
cd build
cmake CMAKE_BUILD_TYPE=Release .. 
make