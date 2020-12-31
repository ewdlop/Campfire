#pragma once

//Do this temporarily so linx build can still build
#ifdef WIN32
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <Windows.h>

#include "Scripts/NativeScript.h"

struct ReloadableCpp
{
    std::wstring DLLPath;
    std::vector<std::string> ProcsToLoad;

    HMODULE Module = NULL;
    uint64_t LastWrite = 0;
    std::map<std::string, void*> Procs;
};

// Checks if the DLL has been changed, and if so, reloads the functions from it.
bool Poll(ReloadableCpp& rcpp)
{
    // Try opening the DLL's file
    HANDLE hDLLFile = CreateFileW(rcpp.DLLPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDLLFile == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    // Check if the DLL has changed since we last loaded it.
    uint64_t lastWriteTime;
    if (GetFileTime(hDLLFile, NULL, NULL, (FILETIME*)&lastWriteTime) &&
        rcpp.LastWrite < lastWriteTime)
    {
        // Remove all references to the old module
        if (rcpp.Module)
        {
            FreeLibrary((HMODULE)rcpp.Module);
            rcpp.Module = NULL;
            rcpp.Procs.clear();
        }

        // Copy the DLL so we don't hold a lock on the original file.
        std::wstring tmpname = rcpp.DLLPath + L".rcpp.dll";
        if (CopyFileW(rcpp.DLLPath.c_str(), tmpname.c_str(), FALSE))
        {
            // Load the copied DLL and get the functions from it.
            rcpp.Module = LoadLibraryW(tmpname.c_str());
            if (rcpp.Module)
            {
                for (const auto& p : rcpp.ProcsToLoad)
                {
                    rcpp.Procs[p] = GetProcAddress((HMODULE)rcpp.Module, p.c_str());
                }
                rcpp.LastWrite = lastWriteTime;
            }
            CloseHandle(hDLLFile);
            return true;
        }
    }
    // Boy Scout Rule!
    CloseHandle(hDLLFile);
    return false;
}

int wmain(int argc, wchar_t* argv[])//tchar for unicode
{
    // Get the name of the directory the .exe is in.
    // We are assuming that the .dll to live-reload is in the same directory.
    std::wstring directory(argv[0], argv[0] + wcslen(argv[0]));//wsclen for wchar
    directory.erase(directory.find_last_of(L'\\') + 1);

    // Specify the path to the DLL and the functions we want to load from it.
    ReloadableCpp rcpp;
    rcpp.DLLPath = directory + L"../../lib/Debug/Scriptsd.dll";
    rcpp.ProcsToLoad = { "CreateNativeScript" };


    if (Poll(rcpp))
    {
        while (true)
        {
            // Poll the DLL and update any live-reloaded function from it.
            CREATE_SCRIPT pEntryPoint = (CREATE_SCRIPT)rcpp.Procs["CreateNativeScript"];
            NativeScript* player = pEntryPoint();
            if (player)
            {
                player->Start();
            }
            while (true)
            {
                player->Update();
                if (Poll(rcpp)) break;
            }

            // Get the pointer to the live-reloaded function,
            // and cast it to a function pointer type with the right signature.
            //auto startPtr = (decltype(Start)*)rcpp.Procs["Start"];

            // If loading the function pointer worked properly, we can call it.
            //Sleep(1000);
        }
    }
    //what should we do if first hotload fail?


    return 0;
}

#else
#include "Core/Application.h"
#include "Core/ResourceManager.h"
#include "Scripts/Script.h"

int main()
{
    extern Application* CreateApplication();
    Application* app = CreateApplication();
    app->Run();
    return 0;
}
#endif //  WIN32