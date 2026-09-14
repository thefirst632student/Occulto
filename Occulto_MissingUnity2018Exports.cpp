// Occulto_MissingUnity2018Exports.cpp
//
// Add this file to Occulto Middleware project to satisfy Unity 2018 GameAssembly exports
// that exist in the original ReZeroDeathKiss GameAssembly.dll but are missing from
// the stock Occulto middleware.
//
// This is a compatibility shim. It dynamically forwards the missing exports to
// GameAssembly.Real.dll. On Windows x64, extra pointer-sized arguments are harmless
// when the real callee expects fewer arguments.
//
// Put this file into:
//   D:\tools\Occulto-main-member104\Middleware\
//
// Then in Visual Studio:
//   Middleware project -> Add -> Existing Item -> Occulto_MissingUnity2018Exports.cpp
//   Build Release | x64
//
// Required missing exports from your log:
//   CloseZStream
//   CreateZStream
//   Flush
//   ReadZStream
//   WriteZStream
//   UnityPalGetLocalTimeZoneData
//   UnityPalGetTimeZoneDataForID
//   UnityPalTimeZoneInfoGetTimeZoneIDs
//   UseUnityPalForTimeZoneInformation
//   il2cpp_unity_liveness_calculation_begin
//   il2cpp_unity_liveness_calculation_end

#include <windows.h>
#include <stdint.h>

static HMODULE GetRealGameAssembly()
{
    static HMODULE h = nullptr;
    if (h)
        return h;

    h = GetModuleHandleA("GameAssembly.Real.dll");
    if (!h)
        h = LoadLibraryA("GameAssembly.Real.dll");

    return h;
}

static FARPROC GetRealExport(const char* name)
{
    HMODULE h = GetRealGameAssembly();
    if (!h)
        return nullptr;
    return GetProcAddress(h, name);
}

// Generic x64 forwarding helper.
// We use a broad 6-argument pointer-sized signature. This keeps RCX/RDX/R8/R9
// and stack arguments usable for the known Unity exports in this compatibility layer.
typedef uintptr_t (__cdecl *Fn6)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);

static uintptr_t Forward6(const char* name,
                          uintptr_t a1 = 0,
                          uintptr_t a2 = 0,
                          uintptr_t a3 = 0,
                          uintptr_t a4 = 0,
                          uintptr_t a5 = 0,
                          uintptr_t a6 = 0)
{
    FARPROC p = GetRealExport(name);
    if (!p)
        return 0;

    return ((Fn6)p)(a1, a2, a3, a4, a5, a6);
}

extern "C" {

// ZStream exports from original Unity GameAssembly.
__declspec(dllexport) uintptr_t __cdecl CreateZStream(uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6)
{
    return Forward6("CreateZStream", a1, a2, a3, a4, a5, a6);
}

__declspec(dllexport) uintptr_t __cdecl CloseZStream(uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6)
{
    return Forward6("CloseZStream", a1, a2, a3, a4, a5, a6);
}

__declspec(dllexport) uintptr_t __cdecl Flush(uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6)
{
    return Forward6("Flush", a1, a2, a3, a4, a5, a6);
}

__declspec(dllexport) uintptr_t __cdecl ReadZStream(uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6)
{
    return Forward6("ReadZStream", a1, a2, a3, a4, a5, a6);
}

__declspec(dllexport) uintptr_t __cdecl WriteZStream(uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6)
{
    return Forward6("WriteZStream", a1, a2, a3, a4, a5, a6);
}

// UnityPal timezone exports.
__declspec(dllexport) uintptr_t __cdecl UnityPalGetLocalTimeZoneData(uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6)
{
    return Forward6("UnityPalGetLocalTimeZoneData", a1, a2, a3, a4, a5, a6);
}

__declspec(dllexport) uintptr_t __cdecl UnityPalGetTimeZoneDataForID(uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6)
{
    return Forward6("UnityPalGetTimeZoneDataForID", a1, a2, a3, a4, a5, a6);
}

__declspec(dllexport) uintptr_t __cdecl UnityPalTimeZoneInfoGetTimeZoneIDs(uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6)
{
    return Forward6("UnityPalTimeZoneInfoGetTimeZoneIDs", a1, a2, a3, a4, a5, a6);
}

__declspec(dllexport) uintptr_t __cdecl UseUnityPalForTimeZoneInformation(uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6)
{
    return Forward6("UseUnityPalForTimeZoneInformation", a1, a2, a3, a4, a5, a6);
}

// Unity liveness exports.
__declspec(dllexport) uintptr_t __cdecl il2cpp_unity_liveness_calculation_begin(uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6)
{
    return Forward6("il2cpp_unity_liveness_calculation_begin", a1, a2, a3, a4, a5, a6);
}

__declspec(dllexport) uintptr_t __cdecl il2cpp_unity_liveness_calculation_end(uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6)
{
    return Forward6("il2cpp_unity_liveness_calculation_end", a1, a2, a3, a4, a5, a6);
}

} // extern "C"
