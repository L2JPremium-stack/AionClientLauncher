#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <shellapi.h>

#include <string>
#include <vector>

#ifndef IMAGE_FILE_MACHINE_ARM64
#define IMAGE_FILE_MACHINE_ARM64 0xAA64
#endif

#ifndef PROCESSOR_ARCHITECTURE_ARM64
#define PROCESSOR_ARCHITECTURE_ARM64 12
#endif

namespace {

std::wstring CombinePath(const std::wstring& left, const std::wstring& right) {
    if (left.empty()) {
        return right;
    }

    wchar_t last = left[left.size() - 1];
    if (last == L'\\' || last == L'/') {
        return left + right;
    }

    return left + L"\\" + right;
}

std::wstring GetDirectoryName(const std::wstring& path) {
    size_t separator = path.find_last_of(L"\\/");
    if (separator == std::wstring::npos) {
        return L".";
    }

    return path.substr(0, separator);
}

bool FileExists(const std::wstring& path) {
    DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool LooksLikeClientRoot(const std::wstring& path) {
    return FileExists(CombinePath(path, L"bin32\\aion.exe")) ||
        FileExists(CombinePath(path, L"bin64\\aion.exe"));
}

std::wstring GetModulePath() {
    std::vector<wchar_t> buffer(MAX_PATH);

    for (;;) {
        DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return L"";
        }

        if (length < buffer.size() - 1) {
            return std::wstring(buffer.data(), length);
        }

        buffer.resize(buffer.size() * 2);
    }
}

std::wstring GetCurrentDirectoryPath() {
    DWORD required = GetCurrentDirectoryW(0, nullptr);
    if (required == 0) {
        return L"";
    }

    std::vector<wchar_t> buffer(required + 1);
    DWORD length = GetCurrentDirectoryW(static_cast<DWORD>(buffer.size()), buffer.data());
    if (length == 0) {
        return L"";
    }

    return std::wstring(buffer.data(), length);
}

std::wstring FindClientRoot() {
    std::wstring modulePath = GetModulePath();
    std::wstring moduleDirectory = modulePath.empty() ? L"" : GetDirectoryName(modulePath);
    if (!moduleDirectory.empty() && LooksLikeClientRoot(moduleDirectory)) {
        return moduleDirectory;
    }

    std::wstring currentDirectory = GetCurrentDirectoryPath();
    if (!currentDirectory.empty() && LooksLikeClientRoot(currentDirectory)) {
        return currentDirectory;
    }

    return moduleDirectory;
}

bool IsNative64BitWindows() {
#if defined(_WIN64)
    return true;
#else
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    if (kernel32) {
        typedef BOOL(WINAPI* IsWow64Process2Ptr)(HANDLE, USHORT*, USHORT*);
        IsWow64Process2Ptr isWow64Process2 =
            reinterpret_cast<IsWow64Process2Ptr>(GetProcAddress(kernel32, "IsWow64Process2"));

        if (isWow64Process2) {
            USHORT processMachine = IMAGE_FILE_MACHINE_UNKNOWN;
            USHORT nativeMachine = IMAGE_FILE_MACHINE_UNKNOWN;
            if (isWow64Process2(GetCurrentProcess(), &processMachine, &nativeMachine)) {
                return nativeMachine == IMAGE_FILE_MACHINE_AMD64 ||
                    nativeMachine == IMAGE_FILE_MACHINE_ARM64 ||
                    nativeMachine == IMAGE_FILE_MACHINE_IA64;
            }
        }

        typedef BOOL(WINAPI* IsWow64ProcessPtr)(HANDLE, PBOOL);
        IsWow64ProcessPtr isWow64Process =
            reinterpret_cast<IsWow64ProcessPtr>(GetProcAddress(kernel32, "IsWow64Process"));

        if (isWow64Process) {
            BOOL isWow64 = FALSE;
            if (isWow64Process(GetCurrentProcess(), &isWow64)) {
                return isWow64 != FALSE;
            }
        }
    }

    SYSTEM_INFO nativeSystemInfo = {};
    GetNativeSystemInfo(&nativeSystemInfo);
    return nativeSystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
        nativeSystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64 ||
        nativeSystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64;
#endif
}

std::wstring QuoteArgument(const std::wstring& argument) {
    if (argument.empty()) {
        return L"\"\"";
    }

    if (argument.find_first_of(L" \t\n\v\"") == std::wstring::npos) {
        return argument;
    }

    std::wstring quoted;
    quoted.push_back(L'"');

    size_t backslashCount = 0;
    for (wchar_t ch : argument) {
        if (ch == L'\\') {
            backslashCount++;
            continue;
        }

        if (ch == L'"') {
            quoted.append(backslashCount * 2 + 1, L'\\');
            quoted.push_back(ch);
            backslashCount = 0;
            continue;
        }

        quoted.append(backslashCount, L'\\');
        backslashCount = 0;
        quoted.push_back(ch);
    }

    quoted.append(backslashCount * 2, L'\\');
    quoted.push_back(L'"');
    return quoted;
}

std::wstring GetForwardedArguments() {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        return L"";
    }

    std::wstring arguments;
    for (int i = 1; i < argc; i++) {
        if (!arguments.empty()) {
            arguments.push_back(L' ');
        }
        arguments += QuoteArgument(argv[i]);
    }

    LocalFree(argv);
    return arguments;
}

std::wstring FormatWin32Error(DWORD error) {
    LPWSTR messageBuffer = nullptr;
    DWORD length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        0,
        reinterpret_cast<LPWSTR>(&messageBuffer),
        0,
        nullptr);

    std::wstring message;
    if (length > 0 && messageBuffer) {
        message.assign(messageBuffer, length);
        LocalFree(messageBuffer);
    }

    if (message.empty()) {
        message = L"Erro do Windows: " + std::to_wstring(error);
    }

    return message;
}

bool LaunchGame(const std::wstring& gamePath, const std::wstring& extraArguments, DWORD* errorCode) {
    std::wstring commandLine = QuoteArgument(gamePath);
    if (!extraArguments.empty()) {
        commandLine.push_back(L' ');
        commandLine += extraArguments;
    }

    std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
    mutableCommandLine.push_back(L'\0');

    STARTUPINFOW startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInfo = {};
    std::wstring workingDirectory = GetDirectoryName(gamePath);

    BOOL created = CreateProcessW(
        gamePath.c_str(),
        mutableCommandLine.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        workingDirectory.c_str(),
        &startupInfo,
        &processInfo);

    if (!created) {
        if (errorCode) {
            *errorCode = GetLastError();
        }
        return false;
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return true;
}

void ShowError(const std::wstring& message) {
    MessageBoxW(nullptr, message.c_str(), L"Aion Launcher", MB_OK | MB_ICONERROR);
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    std::wstring clientRoot = FindClientRoot();
    std::wstring x86Path = CombinePath(clientRoot, L"bin32\\aion.exe");
    std::wstring x64Path = CombinePath(clientRoot, L"bin64\\aion.exe");

    bool hasX86 = FileExists(x86Path);
    bool hasX64 = FileExists(x64Path);

    if (!hasX86 && !hasX64) {
        ShowError(
            L"Nao encontrei bin32\\aion.exe nem bin64\\aion.exe.\n\n"
            L"Coloque o AionLauncher.exe na raiz do cliente, ao lado das pastas bin32 e bin64.");
        return 1;
    }

    bool preferX64 = IsNative64BitWindows() && hasX64;
    std::wstring primaryPath = preferX64 ? x64Path : x86Path;
    std::wstring fallbackPath = preferX64 ? x86Path : x64Path;
    bool hasFallback = preferX64 ? hasX86 : hasX64;

    std::wstring forwardedArguments = GetForwardedArguments();
    DWORD errorCode = ERROR_SUCCESS;

    if (LaunchGame(primaryPath, forwardedArguments, &errorCode)) {
        return 0;
    }

    if (hasFallback && errorCode == ERROR_BAD_EXE_FORMAT &&
        LaunchGame(fallbackPath, forwardedArguments, &errorCode)) {
        return 0;
    }

    ShowError(
        L"Nao consegui iniciar:\n" + primaryPath + L"\n\n" +
        FormatWin32Error(errorCode));
    return 1;
}
