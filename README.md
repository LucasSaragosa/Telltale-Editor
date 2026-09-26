# Telltale Editor

![Telltale Editor](https://github.com/Telltale-Modding-Group/Telltale-Editor/actions/workflows/cmake-multi-platform.yml/badge.svg)

![editor_example](Screenshots/EditorMain_TX100.png)

This project is an all in one modding application for all games made in the Telltale Tool by Telltale Games. For information about how it works, what you can do with it and more, see the [wiki](https://github.com/Telltale-Modding-Group/Telltale-Editor/wiki).
This can be built for Windows, MacOS and Linux.

### How to build

This project uses CMake and to build everything you can use the build scripts, at Scripts/Builders. Pass in two arguments, the config debug/release and the second argument
needs to be the installation location of your Qt installation (read below):

This project (is in the process of integrating so it) uses Qt for UI. If you want to build from source, please build these yourself. For building on windows do the following:

- Install Ninja (eg with winget)
- Open x64 Native Tools Powershell for Visual Studio (in search bar, MUST be x64) and take it to the Qt directory
- Run the following CMake inside the Qt repo folder: cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release -DQT_BUILD_TESTS=OFF -DQT_BUILD_EXAMPLES=OFF -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe
- Run command 'cmake --build build-ninja --parallel' and wait.
- Build files are in build-nina/bin (DLLs) and build-ninja/lib (import libs)
- Run command 'cmake --install build-ninja' so TTE can find Qt on your system

Example and default Windows Qt installation for release: cmake -B build -S . -DCMAKE_PREFIX_PATH="C:/Qt/Qt-6.13.0" -DCMAKE_BUILD_TYPE=Release

### For Linux Users

Note that this project is mainly tested on Windows and MacOS. Linux is supported but may have bugs so please report them.
Zenity must be installed before using Telltale Editor on Linux! If not file dialogs won't open!

## Authors

This project was made possible by lots of work done by various people. 

All C++ and C implementation, and Lua Classes:
#### [Lucas Saragosa](https://github.com/LucasSaragosa)

Lua classes and CMake build system as well as UI development:
#### [Ivan ('DarkShadow')](https://github.com/iMrShadow)

Initial testing help and github workflows
#### [Asil ('Proton')](https://github.com/asilz)

Support and future help
#### [David M.](https://github.com/frostbone25)
