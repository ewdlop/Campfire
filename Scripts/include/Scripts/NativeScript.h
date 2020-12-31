#pragma once

#ifdef NATIVE_SCRIPT_EXPORTS
#define NATIVE_SCRIPT_API __declspec(dllexport)
#else
#define NATIVE_SCRIPT_API __declspec(dllexport)
#endif

struct NativeScript
{
    virtual ~NativeScript() {}
    virtual void Start() {}
    virtual void Update() {}
};

extern "C"
{
    NATIVE_SCRIPT_API NativeScript* _cdecl CreateNativeScript();
};

typedef NativeScript* (*CREATE_SCRIPT) ();