# Basil Engine
Passta Basil Engine Source

## prerequisites
- Visual Studio 2022
- CMake 3.20++

## setup guide
### setting up the mono environment
run root/dep/vendor/install.bat to install the mono runtime environment
### using Visual Studio CMake
open the top directory(root) with visual studio's build in CMake support
### using cmake Visual Studio solution generator
run root/Batch Scripts/RunCMAKEGenerator.bat. press 1 when prompted. open the solution folder in root/build/gam300.sln

## building the editor
set the editor.exe as the build target(**visual studio cmake**) or startup project(**visual studio solution generator**)

## opening the editor
1. select open project button
2. locate the unity_levelv2/.basil
3. press open

## building the game
1. select file->build project in the toolsbar
2. check the build configuration dropdown
3. [**OPTIONAL**] fill in the build configuration options
4. press build and wait for build success
5. locate the game in the build output folder (**DEFAULT: project_folder/build**)

## cleanup guide
#### using Visual Studio CMake
delete visual studio generated folders (/.vs, /out)
#### using cmake Visual Studio solution generator
delete root/build folder
