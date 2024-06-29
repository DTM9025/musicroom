# Preservation notes

[`src/`](src/) corresponds to the source archive, [`dist/`](dist/) to the binary
archive. [`libs/`](libs/) contains all used libraries as Git submodules, at the
versions that were originally linked.

The .bgm files and the `valuespit` tool are part of the [bgmlib
repository](libs/bgmlib/).

## Prerequisites

* Clone the repo. It is suggested you do a recursive clone as follows:

  ```
  $ git clone --recursive https://github.com/DTM9025/musicroom.git
  ```

* Make sure you got all submodules:

  ```
  $ git submodule init
  $ git submodule update
  ```

* It is also recommended to target the same Windows SDK Version and the same
  Platform Toolset for each project for consistency whenever it asks.

## Building a Release with VS Community 2017 or Later

These instructions assumes you have installed VS Community with the `Desktop development with C++` workload (or related options). Development personally is using Visual Studio Community 2017, but I think later version should still work.

### Building and Placing the Needed Libraries

**NOTE:** Make sure to consistently set the same *C/C++ → Code Generation → Runtime Library* option in all projects being built, because not all of them will come with the same by default. I would suggest MultiThreadedDLL for Release builds and MultiThreadedDebugDLL for Debug builds.

**NOTE:** VS Community may ask you to do so open opening a project or solution. If they do, be sure to check all the projects and make the Windows SDK Version and Platform Toolset the same for all projects and solutions. If it doesn't and you are getting build erros, try manually retargeting the project by right clicking the project in the Solution Explorer and select Retarget.

* Open `libs/bgmlib/libs/ogg/win32/VS2010/libogg_dynamic.vcxproj` with VS Community, change Debug to Release in the dropdown menu, and then build the `libogg` project by right clicking `libogg` in the Solution Explorer and selecting *Build*.

* Navigate into the `Win32/Release` directory and copy `libogg.lib` and `libogg.dll` to the `libs/` folder in the repo root.

* Open `libs/bgmlib/libs/vorbis/win32/VS2010/vorbis_dynamic.sln` with VS Community, change Debug to Release in the dropdown menu, and then build the `libvorbis` project by right clicking `libvorbis` in the Solution Explorer and selecting *Build*.

* Open `libs/bgmlib/libs/vorbis/win32/VS2010/vorbis_static.sln` with VS Community, change Debug to Release in the dropdown menu, and then build the `libvorbisfile` project by right clicking `libvorbisfile` in the Solution Explorer and selection *Build*.

* Navigate into the `Win32/Release` directory and copy `libvorbis.lib`, `libvorbis.dll`, and `libvorbisfile_static.lib` to the `libs/` folder in the repo root.

* Open `libs/bgmlib/libs/fox/windows/vcpp/reswrap/reswrap.vcxproj` with VS Community, change Debug to Release in the dropdown menu, and then build the `reswrap` project by right clicking `reswrap` in the Solution Explorer and selection *Build*.

* Open `libs/bgmlib/libs/fox/windows/vcpp/fox/fox.vcxproj` with VS Community, change Debug to Release in the dropdown menu, and then build the `fox` project by right clicking `fox` in the Solution Explorer and selection *Build*.

* Navigate into the `Release` directory and copy `fox.lib` to the `libs/` folder in the repo root.

* Open the Developer Command Prompt for VS2017 (substituting in your own VS version). This should be installed with VS Community. You can search for it in the Start Menu.

* In that command prompt, navigate to `libs/curl/winbuild` and run the following command:

  ```
  $ nmake /f Makefile.vc mode=static
  ```

* Navigate to the `../builds/libcurl-vc-x86-release-static-ipv6-sspi-winssl/lib` directory and copy `libcurl_a.lib` to the `libs/` folder in the repo root.

### Building Touhou Music Room for Release

**NOTE:** Like above, be sure to have the same  *C/C++ → Code Generation → Runtime
  Library* option for all projects in the solution as the ones you used for the libraries.

**NOTE:** Be sure to target the same Windows SDK and Platform Toolset as before.

* Open `src/musicroom.sln` with VS Community

* Change Debug to Release in the dropdown menu.

* Build the solution by going to *Build -> Build Solution*. A `musicroom.exe` executable will be generated in the repo root.

* Place `musicroom.exe` into the `dist/` directory, since it absolutely
  needs `musicroom.cfg` to run. Make sure to also copy the `libvorbis.dll` and
  `libogg.dll` files from `libs/` to the `dist/` directory as well.
  Also, copy bgmlib's `bgminfo/` subdirectory there.

* If you also want to have the program be able to output `mp3` and `flac` formats include [`lame.exe`](https://lame.sourceforge.io/) and [`flac.exe`](https://xiph.org/flac/) encoders into the `dist/` directory. You can just use the same ones as I do in the releases of Touhou Music Room

* ZIP it up and distribute!
