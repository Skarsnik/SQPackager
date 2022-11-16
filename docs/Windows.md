# Windows builds

This document explains how SQPackager builds the Windows releases

# Prerequisite

SQPackager only builds for MSVC and does not provide static build. Static builds require building Qt from sources
and since that is a costly operation it's not easy to integrate into a CLI.

The build process assumes you are building from an `AMD64` architecture.

# Thing you should add to your project

You need a RC file that qmake will use to set your application icon and adding
`RC_FILE = myfile.rc` to your .pro file. You also need a `.ico` file.

An installer icon file

# Detection

SQPackager tries to find the following :

- All you Microsoft Visual Studio Installation
- All the MSVC versions inside these Visual Studio installations
- All Qt version

Then it tries to match the highest Qt version with the highest compatible MSVC version for each architecture

It also tries to find :
- 7-zip installation to produce 7zip archive for standalone releases
- InnoSetup V6 to generate an installer
- jom to replace nmake (allow to use all your cpu core)

# Build phase

A pair of builds is done for every architecture found, a standalone build and an installer build.

The build process inherits SQPackager environment.

The build can be summarized to

```
VsDevCmd.bat /clean_env
vcvarsall.bat -vcvars_ver=<vctoolversion> <arch>
qmake -spec win32-msvc CONFIG+="release no_batch" DEFINES+=SQPROJECT_WIN32_STANDALONE 1
nmake (or jom)
nmake install
```

Note that the qmake call receives a `DEFINES+=SQPROJECT_WIN32_STANDALONE 1` or `DEFINES+=SQPROJECT_WIN32_INSTALL 1` argument to separate a standalone release and installed release.

# Release phase

If you did not provide an install target in your .pro, SQPackager will copy the Exe found in the build directory, then the Readme and Licence file to the deploy directory.

windeployqt is then run like this:

```
windeployqt.exe mainexe.exe --no-translations --no-system-d3d-compiler --no-opengl --no-webkit --no-webkit2 --release
```

SQPackager then removes the following file

```
opengl32sw.dll
libEGL.dll
libGLESV2.dll
vc_redist.x64.exe
vc_redist.x86.exe
```

A zip and 7-zip files are generated for a standalone release. Then if available
an installer file is generated.

# Options

These are the Windows specifics commands line arguments you can give to sqpackager

- `--gen-windows` : Check and generate Windows-related stuff
- `--windows-build-path` `<path>` : Set the base directory where compilation takes place
- `--windows-deploy-path` `<path>` : Set the base directory where deployment takes place
