#pragma once

#ifdef NATIVE_SCRIPT_EXPORTS
#define NATIVE_SCRIPT_API __declspec(dllexport)
#else
#define NATIVE_SCRIPT_API __declspec(dllexport)
#endif

extern "C" NATIVE_SCRIPT_API void Start();
extern "C" NATIVE_SCRIPT_API void Update();
//extern "C" NATIVE_SCRIPT_API_API void Update(float dt);
//extern "C" NATIVE_SCRIPT_API_API void OnTriggerEnter();
//extern "C" NATIVE_SCRIPT_API_API void OnTriggerExit();
//extern "C" NATIVE_SCRIPT_API_API void OnTriggerStay();
