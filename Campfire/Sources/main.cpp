#ifdef Lua
#include "Scripting/ScriptEngine.h"
#ifdef  XXX
#include<Windows.h>
int main(int argc, char* argv[])
{
    //To Do: use STDIN/STDOUT
    //ShellExecuteA(0, "open", "cmd.exe", "/C C:\\Users\\Ray\\Desktop\\Blazor\\TenetEngine\\Campfire\\Build\\Campfire\\Lua\\test.bat", 0, SW_SHOWDEFAULT);
    STARTUPINFO sInfo;
    PROCESS_INFORMATION pInfo;

    ZeroMemory(&sInfo, sizeof(sInfo));
    ZeroMemory(&pInfo, sizeof(pInfo));
    sInfo.cb = sizeof(STARTUPINFOA);
    //sInfo.lpReserved = NULL;
    //sInfo.lpReserved2 = NULL;
    //sInfo.cbReserved2 = 0;
    //sInfo.lpDesktop = NULL;
    //sInfo.lpTitle = NULL;
    //sInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    //sInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    //sInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    //sInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    //sInfo.wShowWindow = SW_SHOWDEFAULT;
    //wchar_t cmdline[] = L"cmd.exe /C C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\Common\\Tools\\VsDevCmd.bat";
    
    if (!CreateProcessA(0,   // No module name (use command line)
        "cmd.exe /K C:\\Users\\Ray\\Desktop\\Blazor\\TenetEngine\\Campfire\\Build\\Campfire\\Lua\\test.bat",        // Command line
        0,           // Process handle not inheritable
        0,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        0,           // Use parent's environment block
        0,           // Use parent's starting directory 
        &sInfo,            // Pointer to STARTUPINFO structure
        &pInfo)           // Pointer to PROCESS_INFORMATION structure
        )
    {
        printf("CreateProcess failed (%d).\n", GetLastError());
        return 0;
    }

    WaitForSingleObject(pInfo.hProcess, INFINITE);

    // Close process and thread handles. 
    CloseHandle(pInfo.hProcess);
    CloseHandle(pInfo.hThread);
    return 0;
}
#else
int main(int argc, char* argv[])
{
    ScriptEngine engine;
    return 0;
}
#endif
#else
#include "Core/Application.h"
int main(int argc, char* argv[])
{
    Application app;
    app.Run();
    return 0;
}
#endif //Lua
