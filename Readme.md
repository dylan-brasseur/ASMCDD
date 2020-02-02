# Accurate Synthesis of Multi Class Disks Distribution
Implementation of `P. Ecormier‐Nocca, P. Memari, J. Gain, et M. Cani, « Accurate Synthesis of Multi‐Class Disk Distributions », Computer Graphics Forum, vol. 38, nᵒ 2, p. 157‑168, may 2019` in C++ and OpenGL.


## How to build ?
This repository has only been tested on Linux and requires cmake 3.0+

- Download the repository and switch to it
- Then type these commands :
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
 ```

## How to run ?
To run the program, go in the build directory and type
```
./DisksProject config_file
```
where ```config_file``` is the config file necessary to run the project on an example. This file is mandatory, and if it's not well formatted or doesn't correspond to the example, the program may crash.

The options are the following :
```
./DisksProject example_config_file [domain_length [error_delta [sigma [step [limit [max_iter [threshold isDistance] ]]]]]]
```

## Available examples :
All available in the configs directory

- forest.txt : 3 classes, 5 pcf, represents a toy forest, same as the first picture of the paper
- forest_no_interaction.txt : 3 classes, 3 pcf, represents a toy forest but with no interaction between the classes
- constrained.txt : 2 classes, 3 pcf, toy forest with trees and mushroom in a very constrained manner
- constrained_overlap.txt : 2 classes, 3 pcf, toy forest with trees and grass similar to the previous one but with overlapping disks
- praise_the_sun.txt : 2 classes, 3 pcf, Solaires of Astora and bronze statues, similar to the previous one but with overlapping disks outside the other class disks
- zerg_rush.txt : 1 class, 1 pcf, Mutalisks in the air following a simple 1 class distribution.
