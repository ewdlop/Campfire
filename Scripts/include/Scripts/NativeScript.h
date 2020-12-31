#pragma once

#ifdef MAKE_DLL
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllexport)
#endif

struct NativeScript
{
    virtual ~NativeScript() {}
    virtual void Start() {}
    virtual void Update() {}
};

extern "C"
{
    EXPORT NativeScript* _cdecl CreateNativeScript();
};

typedef NativeScript* (*CREATE_SCRIPT) ();
