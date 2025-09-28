#include <iostream>
#include <Windows.h>
#include <filesystem>
#include "ThirdParty/SimpleIni.h"

// Basic config struct for holding settings values
typedef struct LLConfig
{
    std::string ProcessName;
    std::string LibraryName;
	int RunDelay;
};

// Various error codes
enum class LLError {
	ConfigNotFound,
	LibraryNotFound,
	ProcessNotFound,
	MissingLibraryPath,
	MissingProcessPath,
	InvalidDelayValue,
	CouldntSpawnProcess,
	CouldntInjectLibrary
};

// Function to load up all of our settings values into our struct
/* Proper config syntax -> llconfig.ini
[settings]
ProcessName=Process.exe
LibraryName=Library.dll
RunDelay=1
*/
LLConfig LoadConfig(const std::string& ConfigName)
{
	CSimpleIniA ini;
	ini.SetUnicode();
	SI_Error rc = ini.LoadFile(ConfigName.c_str());
	if (rc < 0)
		throw LLError::ConfigNotFound;

	LLConfig config;
	config.ProcessName = ini.GetValue("settings", "ProcessName", "null");
	config.LibraryName = ini.GetValue("settings", "LibraryName", "null");
	try
	{
		config.RunDelay = std::stoi(ini.GetValue("settings", "RunDelay", "0"));
	}
	catch (std::invalid_argument)
	{
		throw LLError::InvalidDelayValue;
	}
	if (config.LibraryName == "null")
		throw LLError::MissingLibraryPath;
	if (config.ProcessName == "null")
		throw LLError::MissingProcessPath;
	if (config.RunDelay == 0)
	if (!std::filesystem::exists(config.LibraryName))
		throw LLError::LibraryNotFound;
	if (!std::filesystem::exists(config.ProcessName))
		throw LLError::ProcessNotFound;

	return config;
}

// Function to spawn a process with the CREATE_SUSPENDED and CREATE_NEW_CONSOLE flag so execution is paused and it doesn't reuse our existing window
PROCESS_INFORMATION SpawnChildProcess(const std::string& ProcessPath)
{
	STARTUPINFOA si = { sizeof(STARTUPINFOA) };
	PROCESS_INFORMATION pi;

	if (CreateProcessA(
		ProcessPath.c_str(),
		NULL,
		NULL,
		NULL,
		FALSE,
		CREATE_SUSPENDED | CREATE_NEW_CONSOLE,
		NULL,
		NULL,
		&si,
		&pi
	))
	{
		return pi;
	}
	else { throw LLError::CouldntSpawnProcess; }
}

// Function that injects a library into the process that is specified through the PROCESS_INFORMATION parameter
void LoadRemoteLibrary(const std::string& LibraryPath, PROCESS_INFORMATION pi)
{
	const char* LibPath = LibraryPath.c_str(); // Convert to a c-style string for our wpm call

	LPVOID LibraryPathPtr = VirtualAllocEx(pi.hProcess, 0, strlen(LibPath) + 1, MEM_COMMIT, PAGE_READWRITE); // Allocate some memory to write to
	if (LibraryPathPtr == nullptr)
		throw LLError::CouldntInjectLibrary;

	WriteProcessMemory(pi.hProcess, LibraryPathPtr, (LPVOID)LibPath, strlen(LibPath) + 1, 0); // Writes the path to our library into the spawned process

	// Create a thread within the spawned process that calls LoadLibraryA, passing the pointer to the library path string we wrote previously
	HANDLE LoadLibThread = CreateRemoteThread(pi.hProcess, 0, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "LoadLibraryA"), LibraryPathPtr, 0, 0);
	if (LoadLibThread == INVALID_HANDLE_VALUE)
		throw LLError::CouldntInjectLibrary;

	WaitForSingleObject(LoadLibThread, INFINITE); // Pauses local execution until the remote thread returns

	VirtualFreeEx(pi.hProcess, LibraryPathPtr, strlen(LibPath) + 1, MEM_RELEASE); // Free that memory we allocated for our library path string
}

int main()
{
	std::cout << "LaunchLoader v1.0 from CaesarAtmos [(https://github.com/CaesarAtmos/LaunchLoader)]\n";
	try // Handles all of our code, any exceptions are thrown from functions
	{
		auto config = LoadConfig("llconfig.ini");
		std::cout << "[*] Config loaded: " << config.ProcessName << " - " << config.LibraryName << std::endl;

		auto process = SpawnChildProcess(config.ProcessName);
		std::cout << "[*] Process created with process ID: " << process.dwProcessId << std::endl;

		LoadRemoteLibrary(config.LibraryName, process);

		std::cout << "[*] Library injected. Starting in " << config.RunDelay << " second(s)\n";
		Sleep(config.RunDelay * 1000); // 1 second = 1000ms; janky, maybe clarify better in the future

		ResumeThread(process.hThread);
		CloseHandle(process.hThread);
		CloseHandle(process.hProcess);
		return 0;
	}
	catch (LLError e) // Catch any errors that we specified in our error enum
	{
		switch (e)
		{
		case LLError::ConfigNotFound: std::cerr << "[!] Fatal error while loading config. (Does it exist?)\n"; break;
		case LLError::LibraryNotFound: std::cerr << "[!] Fatal error while loading config. (LibraryNotFound)\n"; break;
		case LLError::ProcessNotFound: std::cerr << "[!] Fatal error while loading config. (ProcessNotFound)\n"; break;
		case LLError::MissingLibraryPath: std::cerr << "[!] Fatal error while loading config. (MissingLibraryPath)\n"; break;
		case LLError::MissingProcessPath: std::cerr << "[!] Fatal error while loading config. (MissingProcessPath)\n"; break;
		case LLError::InvalidDelayValue: std::cerr << "[!] Fatal error while loading config. (InvalidDelayValue)\n"; break;
		case LLError::CouldntSpawnProcess: std::cerr << "[!] Fatal error while spawning process. (Try running as UAC admin.)\n"; break;
		case LLError::CouldntInjectLibrary: std::cerr << "[!] Fatal error while injecting library. (Try running as UAC admin.)\n"; break;
		}
		std::cout << "[?] Fatal error encountered. Press any key to close LaunchLoader." << std::endl;
		std::cin.get();
		return 1;
	}
}
