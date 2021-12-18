# Qt GH DLL Injector
- Rebuild Guided Hacking DLL Injector
- This Version works with MSVC2019/MSVC2022 and QT 5.15.2

# Modified version of this repo by multikill
https://github.com/multikill/GH_Injector_MSVC_2019_QT_5_15_0

## Picture
![alt_text](https://i.gyazo.com/1a0bbc65dba4e5e0f8ca4c6a66f7f986.png)

## How to build
1. Visual Studio 2019
	1. Download https://visualstudio.microsoft.com/vs/
2. Qt
	1. Download https://www.qt.io/download-qt-installer
	1. Install Qt 5.15.2 -> MSVC 2019 32-bit
	2. Install Qt 5.15.2 -> MSVC 2019 64-bit
3. Qt VS Tools for Visual Studio 2019
	1. Download https://marketplace.visualstudio.com/items?itemName=TheQtCompany.QtVisualStudioTools2022
4. Static Qt 5.15.2
	1. Download https://github.com/martinrotter/qt5-minimalistic-builds/releases
	2. Extract to "C:\Qt\5.15.2\qt-5.15.2-static-msvc2019-x86_64"
5. Setup MSVC
	1. Toolbar -> Qt VS Tools -> Qt Options -> Add ->
		1. "C:\Qt\5.15.2\msvc2019"
		2. "C:\Qt\5.15.2\msvc2019_64"
		3. "C:\Qt\5.15.2\qt-5.15.2-static-msvc2019-x86_64"
	2. Toolbar -> Project -> Properties -> Qt Project Settings -> Qt Installtion -> 
		1. x86 -> msvc2019
		2. x64 -> msvc2019_64
		3. x64_static -> qt-5.15.2-static-msvc2019-x86_64
	3. Restart MSVC to repair intellisense
	4. Build project
6. GH Injector-Library
	1. Download https://github.com/Broihon/GH-Injector-Library
	2. Change C++ Language to std:c++20
	3. Build and Copy to Project Folder

## Features:
- Intelligent drag and drop that bypasses UIPI
- Commandline interface
- Shortcut generater
- Auto injection

## Credits:
- https://guidedhacking.com/resources/guided-hacking-dll-injector.4/
- https://github.com/Jorgen-VikingGod/Qt-Frameless-Window-DarkStyle
- https://github.com/fpoussin/Qt5-MSVC-Static

## License
All original licenses of all used components Qt are respected with the additional exception that compiling, linking or using is allowed. Go to Qt website and check for License.
