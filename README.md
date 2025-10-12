# LaunchLoader

A project for launching Windows binaries and injecting a specified DLL using CreateRemoteThread.
## Features

- Multi-architectural. Build for 64 or 32 bit out of the box.
- Configuration on the fly using an easy to edit .ini file.
- Small codebase, (< 200 lines in the main file)!
- Minimal dependencies, includes the lightweight single header "SimpleIni" library from [here](https://github.com/brofield/simpleini)


## Usage

Simply clone the repository and compile from source using Visual Studio. You'll need to edit the configuration file "llconfig.ini" before use.

The format is as follows:
```ini
[settings]
ProcessName=C:\Some\Path\Program.exe ; Process to spawn
LibraryName=ExampleLib.dll           ; Library to inject
RunDelay=5                           ; Run delay in seconds
ShowConsoleWindow=true               ; Show LaunchLoaders window
```
Note: ShowConsoleWindow does not hide the console on Windows 11 due to the default terminal no longer being conhost.exe.

After editing the configuration file, double click LaunchLoader.exe or run LaunchLoader from a terminal. LaunchLoader will attempt to spawn the process in a suspended state, inject the dll, and will wait the specified number of seconds you set in RunDelay before unsuspending the process.
## Notes

### Security Note
For ethical use only. This project contains code that could execute potentially unsafe code in another process. This is not designed to be secure. Use at your own risk and do not distribute to users that don't understand this warning and the potential security ramifications.
### Personal Note
This project may not be the cleanest or use the best practices, however I believe in the interest of educational and personal reasons it's a great public release. This PoC could be tremendously helpful for beginning game modders and security enthusiasts to help understand code injection and Windows process manipulation.
## Libraries Used

 - [SimpleIni (MIT Licensed)](https://github.com/brofield/simpleini)

