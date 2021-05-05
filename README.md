# KaXim
Kappa for Expanded Simulations

Install instructions:

``` [bash]
$ git clone https://github.com/DLab/KaXim.git
$ cd KaXim/src/grammar
$ make
$ cd -
$ cd KaXim/Release
$ make all
$ sudo cp KaXim /usr/bin
```
Dependencies:
- flex
- bison
- libboost_program_options
- libboost_system
- libboost_filesystem

PS:
For testing is useful to install debug version:
``` [bash]
$ cd KaXim/Debug
$ make all
$ sudo cp KaXim /usr/bin/KaXim-Debug
```
