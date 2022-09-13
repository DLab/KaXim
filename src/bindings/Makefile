
all:
	g++ -O3 -shared -fPIC `python3 -m pybind11 --includes` -I . wrapper.cpp -o KaXim`python3-config --extension-suffix` -L/usr/lib -lKaXim -Wl,-rpath,. -I../../eigen3/
	g++ -O0 -g3 -DDEBUG -fopenmp -shared -fPIC `python3 -m pybind11 --includes` -I . wrapper.cpp -o KaXimDebug`python3-config --extension-suffix` -L/usr/lib -lKaXimDebug -Wl,-rpath,. -I../../eigen3/
#	sudo cp KaXim.cpython-38-x86_64-linux-gnu.so /usr/lib/
