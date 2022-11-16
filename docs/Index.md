# SQPackager

This project aims to simplify and automate the deployment of a basic Qt Application built with `qmake`.

## Usage

You describe your project in a `sqproject.json` file and its characteristics and that should be enough to be able
to deploy for all the supported platforms.

The idea is to run a command like `sqpackager --build-platform` in your project directory and `SQPackager` will set up
the build and the deployment according to the platform.

Each supported platform has its own specific part/requirement and you should run
`--gen-platform` or `--check-platform` before doing the `--build-platform`

## Example

This is from the ExampleApp sqproject.json file
```
{
    "name" : "SQExampleApp",
    "pro-file" : "ExampleApp.pro",
    "org" : "fr.nyo",
    "short-description" : "An example of SQProject",
    "description" : "SQExample app is a dummy application to test various feature of a SQProject",
    "icon" : "icon.png"
}
```

## Fields

- name : the Name of the Application
- short-description : describs your application in a short way
- description : the full description of your application
- pro-file : if you need to specify your .pro file
- org : This is needed by flatpak
- icon : Used by a .desktop file
- version : specify your application version. If not set: default to using git (tag or current commit) then the date. You can manually set it to "git" or "date"
- qt-major-version : ether qt5 or qt6

## Qt Version

If you don't specify your required Qt version, sqpackager will try to use the highest Qt version installed or kits, depending
on the specified platform. This could be Qt6. Use the `qt-major-version` if you need to enforce Qt5 or Qt6


# Platform

You will have to look at each platform's documentation to learn more about how things are built or if there are additionals options








