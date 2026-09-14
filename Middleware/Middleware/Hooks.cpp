#include "Global.h"

static bool CopyAndHashName(const char* source, char* buffer, const size_t bufferSize)
{
    if (!source || !buffer || bufferSize == 0)
        return false;

    const size_t len = strlen(source);
    if (len + 1 > bufferSize)
    {
        Console::Print("Buffer too small for name: %s", source);
        return false;
    }

    memcpy(buffer, source, len + 1);
    Hash::Run(buffer, len);
    return true;
}

EXTERN_C DLL_EXPORT PVOID il2cpp_class_from_name(const PVOID image, const char* namespaze, const char* name)
{
    static PVOID(*target_func)(const PVOID, const char*, const char*) = nullptr;
    if (!target_func)
        target_func = reinterpret_cast<PVOID(*)(const PVOID, const char*, const char*)>(
            GetProcAddress(Global::GameAssembly, "il2cpp_class_from_name"));

    if (!target_func)
    {
        Console::Print("Failed to resolve il2cpp_class_from_name()");
        return nullptr;
    }

    const char* safeNamespace = namespaze ? namespaze : "";
    const char* safeName = name ? name : "";

    Console::Print("il2cpp_class_from_name(): %s.%s", safeNamespace, safeName);

    PVOID result = target_func(image, safeNamespace, safeName);
    if (result)
        return result;

    char hashedNamespace[256];
    char hashedName[256];

    const size_t nsLen = strlen(safeNamespace);
    if (nsLen + 1 > sizeof(hashedNamespace))
    {
        Console::Print("Namespace buffer too small");
        return result;
    }

    memcpy(hashedNamespace, safeNamespace, nsLen + 1);

    if (!CopyAndHashName(safeName, hashedName, sizeof(hashedName)))
        return result;

    // Occulto metadata parser currently hashes simple type names only, not namespaces.
    // Keep namespace unchanged and hash the class name.
    // Hash::Run(hashedNamespace, nsLen);

    Console::Print("-> hashed class lookup: %s.%s", hashedNamespace, hashedName);

    result = target_func(image, hashedNamespace, hashedName);

    Console::Print("-> 0x%p", result);

    return result;
}

EXTERN_C DLL_EXPORT PVOID il2cpp_class_get_property_from_name(PVOID klass, const char* name)
{
    static PVOID(*target_func)(PVOID klass, const char* name) = nullptr;
    if (!target_func)
        target_func = reinterpret_cast<PVOID(*)(PVOID klass, const char* name)>(
            GetProcAddress(Global::GameAssembly, "il2cpp_class_get_property_from_name"));

    if (!target_func)
    {
        Console::Print("Failed to resolve il2cpp_class_get_property_from_name()");
        return nullptr;
    }

    if (!name)
        return target_func(klass, name);

    Console::Print("il2cpp_class_get_property_from_name(): %s", name);

    PVOID result = target_func(klass, name);
    if (result)
        return result;

    char hashedName[256];
    if (!CopyAndHashName(name, hashedName, sizeof(hashedName)))
        return result;

    Console::Print("-> hashed property lookup: %s", hashedName);

    result = target_func(klass, hashedName);

    Console::Print("-> 0x%p", result);

    return result;
}

EXTERN_C DLL_EXPORT PVOID il2cpp_class_get_field_from_name(PVOID klass, const char* name)
{
    static PVOID(*target_func)(PVOID klass, const char* name) = nullptr;
    if (!target_func)
        target_func = reinterpret_cast<PVOID(*)(PVOID klass, const char* name)>(
            GetProcAddress(Global::GameAssembly, "il2cpp_class_get_field_from_name"));

    if (!target_func)
    {
        Console::Print("Failed to resolve il2cpp_class_get_field_from_name()");
        return nullptr;
    }

    if (!name)
        return target_func(klass, name);

    Console::Print("il2cpp_class_get_field_from_name(): %s", name);

    PVOID result = target_func(klass, name);
    if (result)
        return result;

    char hashedName[256];
    if (!CopyAndHashName(name, hashedName, sizeof(hashedName)))
        return result;

    Console::Print("-> hashed field lookup: %s", hashedName);

    result = target_func(klass, hashedName);

    Console::Print("-> 0x%p", result);

    return result;
}

EXTERN_C DLL_EXPORT PVOID il2cpp_class_get_method_from_name(PVOID klass, const char* name, int argsCount)
{
    static PVOID(*target_func)(PVOID klass, const char* name, int argsCount) = nullptr;
    if (!target_func)
        target_func = reinterpret_cast<PVOID(*)(PVOID klass, const char* name, int argsCount)>(
            GetProcAddress(Global::GameAssembly, "il2cpp_class_get_method_from_name"));

    if (!target_func)
    {
        Console::Print("Failed to resolve il2cpp_class_get_method_from_name()");
        return nullptr;
    }

    if (!name)
        return target_func(klass, name, argsCount);

    Console::Print("il2cpp_class_get_method_from_name(): %s/%d", name, argsCount);

    PVOID result = target_func(klass, name, argsCount);
    if (result)
        return result;

    char hashedName[256];
    if (!CopyAndHashName(name, hashedName, sizeof(hashedName)))
        return result;

    Console::Print("-> hashed method lookup: %s/%d", hashedName, argsCount);

    result = target_func(klass, hashedName, argsCount);

    Console::Print("-> 0x%p", result);

    return result;
}
