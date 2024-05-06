# Qt GH DLL Injector GUI Interface
- This is the GUI Interface for the [Guided Hacking DLL Injector](https://guidedhacking.com/resources/guided-hacking-dll-injector.4/)
- This Version works with MSVC2019/MSVC2022 and QT 5.15.2

## Initial Qt GUI Interace by Kage/Multikill

The original GH Injector used AutoIt as a GUI frontend.  [Kage](https://guidedhacking.com/members/kage.109622/) / [Multikill](https://github.com/multikill) developed the initial version of Qt front end in 2020, which Broihon has continued to improve over the past 5 years.

## Picture
![image](https://github.com/guided-hacking/GH-Injector-GUI/assets/15186628/81f934c8-b319-4058-928c-69d8c971672a)

<h3>Official Guided Hacking Courses</h3>
<ul>
	<li><a href="https://guidedhacking.com/ghb" target="_blank">The Game Hacking Bible</a>&nbsp;- a massive 70 chapter Game Hacking Course</li>
	<li><a href="https://guidedhacking.com/threads/squally-cs420-game-hacking-course.14191/" target="_blank">Computer Science 420</a>&nbsp;- an eight chapter lecture on CS, Data Types &amp; Assembly</li>
	<li><a href="https://guidedhacking.com/forums/binary-exploit-development-course.551/" target="_blank">Binary Exploit Development</a>&nbsp;- a 9 chapter series on exploit dev&nbsp;from a certified OSED</li>
	<li><a href="https://guidedhacking.com/forums/game-hacking-shenanigans/" target="_blank">Game Hacking Shenanigans</a>&nbsp;- a twenty lesson Cheat Engine hacking course</li>
	<li><a href="https://guidedhacking.com/threads/python-game-hacking-tutorial-1-1-introduction.18695/" target="_blank">Python Game Hacking Course</a>&nbsp;- 7 chapter external &amp; internal python hack lesson</li>
	<li><a href="https://guidedhacking.com/threads/python-game-hacking-tutorial-2-1-introduction.19199/" target="_blank">Python App Reverse Engineering</a>&nbsp;- Learn to reverse python apps in 5 lessons</li>
	<li><a href="https://guidedhacking.com/threads/web-browser-game-hacking-intro-part-1.17726/" target="_blank">Web Browser Game Hacking</a>&nbsp;- Hack javascript games with this 4 chapter course</li>
	<li><a href="https://guidedhacking.com/forums/roblox-exploit-scripting-course-res100.521/" target="_blank">Roblox Exploiting Course</a>&nbsp;- 7 Premium Lessons on Hacking Roblox</li>
	<li><a href="https://guidedhacking.com/forums/java-reverse-engineering-course-jre100.538/" target="_blank">Java Reverse Engineering Course</a>&nbsp;- 5 chapter beginner guide</li>
	<li><a href="https://guidedhacking.com/forums/java-game-hacking-course-jgh100.553/" target="_blank">Java Game Hacking Course</a>&nbsp;- 6 Chapter Beginner Guide</li>
</ul>

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
	2. Toolbar -> Project -> Properties -> Qt Project Settings -> Qt Installation -> 
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
- Shortcut generator
- Auto injection

## Credits:
- https://guidedhacking.com/resources/guided-hacking-dll-injector.4/
- https://github.com/Jorgen-VikingGod/Qt-Frameless-Window-DarkStyle
- https://github.com/fpoussin/Qt5-MSVC-Static

## License
All original licenses of all used components Qt are respected with the additional exception that compiling, linking or using is allowed. Go to Qt website and check for License.


