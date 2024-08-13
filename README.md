# Touhou Music Room

The Touhou Music Room is program for playing, extracting, and tagging the music from the Touhou games. It currently supports all official games released on Windows.

The music can be encoded into multiple formats (Ogg Vorbis native, FLAC and MP3 via bundled external encoders), while adding correct tags and comments in either Japanese or English. Translations are automatically downloaded and updated from the [Touhou Wiki](http://en.touhouwiki.net). You can mass update tags to your music files with this tool as well.

You can customize the loop count, fade duration and automatic filenaming.

It also can be used as a masstagger for the PC-98 soundtracks from Akyu's Untouched Score and every single one of ZUN's music CDs.

Any future mainline games can be extracted with this tool by providing an appropriate bgm file in the `bgminfo` folder. To generate a bgm file for a mainline game, simply invoke the provided `valuespit_thXX.exe` with the `thbgm.fmt` file extracted from the game and edit the generated file as per the template in the `bgminfo` folder.

This program was originally created and developed by [Nmlgc](https://github.com/nmlgc). Development is now being done by [DTM](https://github.com/DTM9025).

## Installation

Simply navigate to the [Release page on GitHub](https://github.com/DTM9025/musicroom/releases) and download the latest ZIP package (should be something like `Touhou.Music.Room.X.Y.Z.zip`). Then extract it to any directory and run `musicroom.exe` in the `Touhou Music Room` folder and you should be good to go!

## Building a Release with VS Community 2017 or Later

**NOTE:** If you just want to use the program, you don't need to do this or go down any further, just install it with the instructions above. Below is only for development purposes.

### Building Prerequisites

[`src/`](src/) corresponds to the source archive, [`dist/`](dist/) to the binary archive. [`libs/`](libs/) contains all used libraries as Git submodules, at the versions that were originally linked.

The .bgm files and the `valuespit` tool are part of the [bgmlib
repository](libs/bgmlib/).

* Clone the repo. It is suggested you do a recursive clone as follows:

  ```
  $ git clone --recursive https://github.com/DTM9025/musicroom.git
  ```

* Make sure you got all submodules:

  ```
  $ git submodule update --init --recursive
  ```

* Building assumes you have installed Visual Studio Community with the `Desktop development with C++` workload (or related options). Development personally is using Visual Studio Community 2017, but I think later versions should still work.

* In addition, building the `curl-impersonate-win` library from source requires MSYS2 and associated packages to be installed. See instructions below on building curl for details.

### Getting the Needed Libraries

This only needs to be done once (unless you are modifying the libraries which should be very unlikely). As soon as the libraries are built and placed in the correct places, you shouldn't need to touch them again.

For this you have two options.  You can either use the prebuilt library binaries provided or you can build them from source yourself. Instructions for both are provided below.

#### Option 1: Using the Prebuilt Libraries

If you don't care about building these libraries from source, you can just download my prebuilt libraries [here](https://github.com/DTM9025/musicroom/releases/download/start/prebuilt_libs.zip). Simply extract its contents into the `libs/` folder and move on to the **Building Touhou Music Room for Release** section.

For posterity, these libraries are built targeting Windows SDK version 10.0.22621.0 with v141 of the Platform Toolset, and using MultiThreadedDLL for the Runtime Library.

#### Option 2: Building the Libraries from Source

**NOTE:** Make sure to consistently set the same *C/C++ → Code Generation → Runtime Library* option in all projects being built, because not all of them will come with the same by default. I would suggest MultiThreadedDLL for Release builds and MultiThreadedDebugDLL for Debug builds.

**NOTE:** VS Community may ask you to retarget the Windows SDK/Platform Toolset upon open opening a project or solution. If they do, be sure to check all the projects and make the Windows SDK Version and Platform Toolset the same for all projects and solutions. If it doesn't and you are getting build errors, try manually retargeting the project by right clicking the project in the Solution Explorer and select Retarget Project.

**NOTE:** Make sure to build the 32 bit Release versions of the libraries. The instructions should ensure that you do that, but be mindful when doing the builds that Release is selected in the dropdown menu and that either Win32 or x86 is selected in the other dropdown menu. 

* Open `libs/bgmlib/libs/ogg/win32/VS2010/libogg_dynamic.sln` with VS Community, change Debug to Release and x64 to Win32 in the dropdown menus, and then build the `libogg` project by right clicking `libogg` in the Solution Explorer and selecting *Build*.

* Navigate into the resulting `Win32/Release/` directory and copy `libogg.lib` and `libogg.dll` to the `libs/` folder in the repo root.

* Open `libs/bgmlib/libs/vorbis/win32/VS2010/vorbis_dynamic.sln` with VS Community, change Debug to Release and x64 to Win32 in the dropdown menus, and then build the `libvorbis` project by right clicking `libvorbis` in the Solution Explorer and selecting *Build*.

* Open `libs/bgmlib/libs/vorbis/win32/VS2010/vorbis_static.sln` with VS Community, change Debug to Release and x64 to Win32 in the dropdown menus, and then build the `libvorbisfile` project by right clicking `libvorbisfile` in the Solution Explorer and selecting *Build*.

* Navigate into the resulting `Win32/Release/` directory and copy `libvorbis.lib`, `libvorbis.dll`, and `libvorbisfile_static.lib` to the `libs/` folder in the repo root.

* Open `libs/bgmlib/libs/fox/windows/vcpp/reswrap/reswrap.vcxproj` with VS Community, change Debug to Release in the dropdown menu, and then build the `reswrap` project by right clicking `reswrap` in the Solution Explorer and selection *Build*.

* Open `libs/bgmlib/libs/fox/windows/vcpp/fox/fox.vcxproj` with VS Community, change Debug to Release in the dropdown menu, and then build the `fox` project by right clicking `fox` in the Solution Explorer and selecting *Build*.

* Navigate into the resulting `Release/` directory and copy `fox.lib` to the `libs/` folder in the repo root.

* Open `libs/bgmlib/libs/MIRACL/miracl.vcxproj` with VS Community, change Debug to Release and x64 to x86 in the dropdown menus, and then build the `miracl` project by right clicking `miracl` in the Solution Explorer and selecting *Build*.

* Navigate into the resulting `Release/` directory and copy `miracl.lib` to the `libs/` folder in the repo root.

* Go to `libs/curl-impersonate-win/` and follow the instructions in the [README](https://github.com/DTM9025/curl-impersonate-win/tree/musicroom#curl-impersonate-win) to build, specifically the sections **Environment** and **Build** (you can obviously skip the step on cloning the repo recursively). Note that this is the only part of the build that requires something other than VS Community, specifically it requires MSYS2. The README should have instructions on how to set it up.

* Navigate to the resulting `curl/bin/` directory and copy `libcurl.dll` and `libcurl.dll.a` to the `libs/` folder in the repo root.

### Building Touhou Music Room for Release

**NOTE:** Like above, be sure to have the same  *C/C++ → Code Generation → Runtime Library* option for all projects in the solution as the ones you used for the libraries. If you used the prebuilt libraries the defaults should be okay.

**NOTE:** Be sure to target the same Windows SDK and Platform Toolset as before.

* Open `src/musicroom.sln` with VS Community

* Change Debug to Release in the dropdown menu.

* Build the solution by going to *Build -> Build Solution*. This will first build the `Release THVC` build of `bgmlib`, then the `Release` build of `th_tool_shared`, and finally the `Release` build of `musicroom`. A `musicroom.exe` executable will be generated in the repo root.

* Place `musicroom.exe` into the `dist/` directory, since it absolutely   needs `musicroom.cfg` to run. Make sure to also copy the `libvorbis.dll`, `libogg.dll`, and `libcurl.dll` files from `libs/` to the `dist/` directory as well. Also, copy bgmlib's `bgminfo/` subdirectory there.

* If you also want to have the program be able to output `mp3` and `flac` formats, include the [`lame.exe`](https://lame.sourceforge.io/) and [`flac.exe`](https://xiph.org/flac/) encoders into the `dist/` directory. You can just use the same ones as I do in the releases of Touhou Music Room.

* ZIP it up and distribute!

## License

This program was originally developed by [Nmlgc](https://github.com/nmlgc/), who released the source code with the following note:

    As for the source code, well, do with it whatever you like.

Development is now being done by [DTM9025](https://github.com/DTM9025) and the program, including bgmlib and th_tool_shared, are now licensed under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program uses various third-party libraries, which have their own licenses. All but one (BoringSSL) are compatible with this program's license. An additional permission is provided to allow the program to link and combine BoringSSL with this program and convey the resulting work.

The full license terms are detailed in [`LICENSE.md`](https://github.com/DTM9025/musicroom/blob/master/LICENSE.md).
