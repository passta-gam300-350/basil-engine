# gam300
repo for gam300 engine

## prerequisites
- Visual Studio 2022
- CMake 3.20++

## setup guide
#### using Visual Studio CMake
&nbsp;&nbsp;open the top directory(root) with visual studio's build in CMake support
#### using cmake Visual Studio solution generator
&nbsp;&nbsp;open command prompt and set directory to top directory (root) and run the following command
```bash
cmake -G "Visual Studio 17 2022" -B build
```
&nbsp;&nbsp;locate the visual studio sln file at the build folder
```bash
start "" "build/gam300.sln"
```

## cleanup guide
#### using Visual Studio CMake
&nbsp;&nbsp;delete visual studio generated folders
#### using cmake Visual Studio solution generator
&nbsp;&nbsp;delete build folder

## ~~working directory (debugger)~~
~~&nbsp;&nbsp;the working directory is located in the bin folder at the top directory level~~
