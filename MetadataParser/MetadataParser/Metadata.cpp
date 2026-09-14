#include "Global.h"

namespace
{
    constexpr uint32_t NO_INDEX = 0xFFFFFFFFu;

    Metadata::Header* g_header = nullptr;
    std::vector<uint8_t>* g_buffer = nullptr;
    uint8_t* g_base = nullptr;
    size_t g_size = 0;
    bool g_typeOnly = false;
    bool g_verbose = false;

    struct TypeLayout
    {
        const char* Name;
        uint32_t RecordSize;
        uint32_t NameIndex;
        uint32_t FieldStart;
        uint32_t MethodStart;
        uint32_t PropertyStart;
        uint32_t MethodCount;
        uint32_t PropertyCount;
        uint32_t FieldCount;
        uint32_t MethodRecordSize;
        uint32_t ParameterStartInMethod;
        uint32_t ParameterCountInMethod;
        bool HashParameters;
    };

    // Some metadata v24 sub-variants differ.
    const TypeLayout LAYOUT_88 = { "v24_88", 88, 0, 32, 36, 44, 64, 66, 68, sizeof(Metadata::MethodDefinition), 16, 34, true };
    const TypeLayout LAYOUT_92 = { "v24_92_byref", 92, 0, 36, 40, 48, 68, 70, 72, sizeof(Metadata::MethodDefinition), 16, 34, true };

    // ReZeroDeathKiss / Unity 2018 metadata v24 layout discovered by metadata_member_probe3.py.
    // TypeDefinitionsSize=458536, recordSize=104, typeCount=4409.
    // FIELD:    fieldStart @ 48, fieldCount @ 84, table count 25593.
    // METHOD:   methodStart @ 52, methodCount @ 80, method record size 32, table count 52568.
    // PROPERTY: propertyStart @ 60, propertyCount @ 82, table count 7429.
    // Parameter offsets inside the 32-byte method record are not trusted here, so parameter hashing is disabled.
    const TypeLayout LAYOUT_104_REZERO = { "v24_104_rezero_probe3", 104, 0, 48, 52, 60, 80, 82, 84, 32, 0, 0, false };

    struct Stats
    {
        uint32_t MatchedTypes = 0;
        uint32_t ModifiedTypes = 0;
        uint32_t ModifiedFields = 0;
        uint32_t ModifiedMethods = 0;
        uint32_t ModifiedParams = 0;
        uint32_t ModifiedProperties = 0;
        uint32_t SkippedInvalid = 0;
        uint32_t SkippedInternal = 0;
        uint32_t SkippedDuplicateString = 0;
    };

    std::unordered_set<size_t> g_modifiedStringOffsets;

    bool InRange(size_t offset, size_t size)
    {
        return offset <= g_size && size <= g_size - offset;
    }

    bool SectionValid(uint32_t offset, uint32_t size)
    {
        return InRange(offset, size);
    }

    uint32_t SectionCount(uint32_t byteSize, size_t recordSize)
    {
        if (recordSize == 0)
            return 0;
        return static_cast<uint32_t>(byteSize / recordSize);
    }

    bool ReadU32(size_t offset, uint32_t& value)
    {
        if (!InRange(offset, sizeof(uint32_t)))
            return false;
        memcpy(&value, g_base + offset, sizeof(uint32_t));
        return true;
    }

    bool ReadU16(size_t offset, uint16_t& value)
    {
        if (!InRange(offset, sizeof(uint16_t)))
            return false;
        memcpy(&value, g_base + offset, sizeof(uint16_t));
        return true;
    }

    bool GetStringInfo(uint32_t index, char*& ptr, size_t& len, std::string& text)
    {
        ptr = nullptr;
        len = 0;
        text.clear();

        if (!g_header)
            return false;

        if (index >= g_header->StringSize)
            return false;

        const size_t offset = static_cast<size_t>(g_header->StringOffset) + index;
        const size_t stringEnd = static_cast<size_t>(g_header->StringOffset) + g_header->StringSize;

        if (!InRange(offset, 1) || stringEnd > g_size || offset >= stringEnd)
            return false;

        size_t end = offset;
        while (end < stringEnd && end < g_size && g_base[end] != 0)
            ++end;

        if (end >= stringEnd || end >= g_size)
            return false;

        ptr = reinterpret_cast<char*>(g_base + offset);
        len = end - offset;
        text.assign(ptr, len);
        return true;
    }

    bool ReadType(const TypeLayout& layout, uint32_t index, uint32_t& nameIndex,
        uint32_t& fieldStart, uint32_t& methodStart, uint32_t& propertyStart,
        uint16_t& methodCount, uint16_t& propertyCount, uint16_t& fieldCount)
    {
        const size_t off = static_cast<size_t>(g_header->TypeDefinitionsOffset) + static_cast<size_t>(index) * layout.RecordSize;
        if (!InRange(off, layout.RecordSize))
            return false;

        return ReadU32(off + layout.NameIndex, nameIndex)
            && ReadU32(off + layout.FieldStart, fieldStart)
            && ReadU32(off + layout.MethodStart, methodStart)
            && ReadU32(off + layout.PropertyStart, propertyStart)
            && ReadU16(off + layout.MethodCount, methodCount)
            && ReadU16(off + layout.PropertyCount, propertyCount)
            && ReadU16(off + layout.FieldCount, fieldCount);
    }

    bool ValidIndexRange(uint32_t start, uint32_t count, uint32_t total)
    {
        if (count == 0)
            return true;
        if (start == NO_INDEX)
            return false;
        if (start > total)
            return false;
        if (count > total - start)
            return false;
        return true;
    }

    bool GetFieldNameIndex(uint32_t fieldIndex, uint32_t& nameIndex)
    {
        const uint32_t total = SectionCount(g_header->FieldsSize, sizeof(Metadata::FieldDefinition));
        if (fieldIndex >= total)
            return false;
        return ReadU32(static_cast<size_t>(g_header->FieldsOffset) + static_cast<size_t>(fieldIndex) * sizeof(Metadata::FieldDefinition), nameIndex);
    }

    bool GetPropertyNameIndex(uint32_t propertyIndex, uint32_t& nameIndex)
    {
        const uint32_t total = SectionCount(g_header->PropertiesSize, sizeof(Metadata::PropertyDefinition));
        if (propertyIndex >= total)
            return false;
        return ReadU32(static_cast<size_t>(g_header->PropertiesOffset) + static_cast<size_t>(propertyIndex) * sizeof(Metadata::PropertyDefinition), nameIndex);
    }

    bool GetParameterNameIndex(uint32_t parameterIndex, uint32_t& nameIndex)
    {
        const uint32_t total = SectionCount(g_header->ParametersSize, sizeof(Metadata::ParameterDefinition));
        if (parameterIndex >= total)
            return false;
        return ReadU32(static_cast<size_t>(g_header->ParametersOffset) + static_cast<size_t>(parameterIndex) * sizeof(Metadata::ParameterDefinition), nameIndex);
    }

    bool ReadMethod(const TypeLayout& layout, uint32_t methodIndex, uint32_t& nameIndex, uint32_t& parameterStart, uint16_t& parameterCount)
    {
        const uint32_t total = SectionCount(g_header->MethodsSize, layout.MethodRecordSize);
        if (methodIndex >= total)
            return false;

        const size_t off = static_cast<size_t>(g_header->MethodsOffset) + static_cast<size_t>(methodIndex) * layout.MethodRecordSize;
        if (!ReadU32(off + 0, nameIndex))
            return false;

        parameterStart = 0;
        parameterCount = 0;
        if (layout.HashParameters)
        {
            if (!ReadU32(off + layout.ParameterStartInMethod, parameterStart)
                || !ReadU16(off + layout.ParameterCountInMethod, parameterCount))
                return false;
        }

        return true;
    }

    bool HashNameByIndex(uint32_t nameIndex, const char* category, const std::string& owner, Stats& stats, bool skipDots = false)
    {
        char* ptr = nullptr;
        size_t len = 0;
        std::string original;
        if (!GetStringInfo(nameIndex, ptr, len, original))
        {
            ++stats.SkippedInvalid;
            return false;
        }

        if (original.empty())
        {
            ++stats.SkippedInvalid;
            return false;
        }

        if (Metadata::IsInternalType(original))
        {
            ++stats.SkippedInternal;
            return false;
        }

        if (skipDots && original.find('.') != std::string::npos)
            return false;

        const size_t absoluteOffset = static_cast<size_t>(reinterpret_cast<uint8_t*>(ptr) - g_base);
        if (g_modifiedStringOffsets.find(absoluteOffset) != g_modifiedStringOffsets.end())
        {
            ++stats.SkippedDuplicateString;
            return false;
        }
        g_modifiedStringOffsets.insert(absoluteOffset);

        Hash::Run(ptr, len);

        if (g_verbose)
            printf(" - [%s] %s.%s => %s\n", category, owner.c_str(), original.c_str(), ptr);

        return true;
    }

    int ScoreLayout(const TypeLayout& layout)
    {
        if (!SectionValid(g_header->TypeDefinitionsOffset, g_header->TypeDefinitionsSize))
            return -1000000;
        if (g_header->TypeDefinitionsSize < layout.RecordSize)
            return -1000000;

        const uint32_t typeCount = SectionCount(g_header->TypeDefinitionsSize, layout.RecordSize);
        const uint32_t fieldTotal = SectionCount(g_header->FieldsSize, sizeof(Metadata::FieldDefinition));
        const uint32_t methodTotal = SectionCount(g_header->MethodsSize, layout.MethodRecordSize);
        const uint32_t propertyTotal = SectionCount(g_header->PropertiesSize, sizeof(Metadata::PropertyDefinition));

        int score = 0;
        if (g_header->TypeDefinitionsSize % layout.RecordSize == 0)
            score += 1000;

        const uint32_t sampleCount = min<uint32_t>(typeCount, 256);
        for (uint32_t i = 0; i < sampleCount; ++i)
        {
            uint32_t nameIndex = 0, fieldStart = 0, methodStart = 0, propertyStart = 0;
            uint16_t methodCount = 0, propertyCount = 0, fieldCount = 0;
            if (!ReadType(layout, i, nameIndex, fieldStart, methodStart, propertyStart, methodCount, propertyCount, fieldCount))
            {
                score -= 10;
                continue;
            }

            char* ptr = nullptr;
            size_t len = 0;
            std::string name;
            if (GetStringInfo(nameIndex, ptr, len, name) && !name.empty())
                score += 3;
            else
                score -= 10;

            if (ValidIndexRange(fieldStart, fieldCount, fieldTotal)) score += 2; else score -= 6;
            if (ValidIndexRange(methodStart, methodCount, methodTotal)) score += 2; else score -= 6;
            if (ValidIndexRange(propertyStart, propertyCount, propertyTotal)) score += 2; else score -= 6;
        }

        return score;
    }

    const TypeLayout& ChooseLayout()
    {
        const int score88 = ScoreLayout(LAYOUT_88);
        const int score92 = ScoreLayout(LAYOUT_92);
        const int score104 = ScoreLayout(LAYOUT_104_REZERO);
        printf("Layout score: %s=%d, %s=%d, %s=%d\n",
            LAYOUT_88.Name, score88, LAYOUT_92.Name, score92, LAYOUT_104_REZERO.Name, score104);

        const TypeLayout* best = &LAYOUT_88;
        int bestScore = score88;
        if (score92 > bestScore)
        {
            best = &LAYOUT_92;
            bestScore = score92;
        }
        if (score104 > bestScore)
        {
            best = &LAYOUT_104_REZERO;
            bestScore = score104;
        }
        return *best;
    }

    bool ProcessType(const TypeLayout& layout, uint32_t index, Stats& stats)
    {
        uint32_t nameIndex = 0, fieldStart = 0, methodStart = 0, propertyStart = 0;
        uint16_t methodCount = 0, propertyCount = 0, fieldCount = 0;
        if (!ReadType(layout, index, nameIndex, fieldStart, methodStart, propertyStart, methodCount, propertyCount, fieldCount))
        {
            ++stats.SkippedInvalid;
            return false;
        }

        char* typeNamePtr = nullptr;
        size_t typeNameLen = 0;
        std::string typeName;
        if (!GetStringInfo(nameIndex, typeNamePtr, typeNameLen, typeName))
        {
            ++stats.SkippedInvalid;
            return false;
        }

        if (Metadata::IsInternalType(typeName))
            return false;

        if (!Config::ShouldProtect(typeName))
            return false;

        ++stats.MatchedTypes;
        printf("[type:%u] %s", index, typeName.c_str());

        const size_t typeStringOffset = static_cast<size_t>(reinterpret_cast<uint8_t*>(typeNamePtr) - g_base);
        if (g_modifiedStringOffsets.find(typeStringOffset) == g_modifiedStringOffsets.end())
        {
            g_modifiedStringOffsets.insert(typeStringOffset);
            Hash::Run(typeNamePtr, typeNameLen);
            ++stats.ModifiedTypes;
            printf(" => %s\n", typeNamePtr);
        }
        else
        {
            printf(" => already-modified-string\n");
        }

        if (g_typeOnly)
            return true;

        const uint32_t fieldTotal = SectionCount(g_header->FieldsSize, sizeof(Metadata::FieldDefinition));
        const uint32_t methodTotal = SectionCount(g_header->MethodsSize, layout.MethodRecordSize);
        const uint32_t parameterTotal = SectionCount(g_header->ParametersSize, sizeof(Metadata::ParameterDefinition));
        const uint32_t propertyTotal = SectionCount(g_header->PropertiesSize, sizeof(Metadata::PropertyDefinition));

        if (ValidIndexRange(fieldStart, fieldCount, fieldTotal))
        {
            for (uint32_t i = 0; i < fieldCount; ++i)
            {
                uint32_t fieldNameIndex = 0;
                if (GetFieldNameIndex(fieldStart + i, fieldNameIndex) && HashNameByIndex(fieldNameIndex, "field", typeName, stats))
                    ++stats.ModifiedFields;
            }
        }
        else
        {
            printf("  [warn] invalid field range start=%u count=%u total=%u\n", fieldStart, fieldCount, fieldTotal);
            ++stats.SkippedInvalid;
        }

        if (ValidIndexRange(methodStart, methodCount, methodTotal))
        {
            for (uint32_t i = 0; i < methodCount; ++i)
            {
                uint32_t methodNameIndex = 0, parameterStart = 0;
                uint16_t parameterCount = 0;
                if (!ReadMethod(layout, methodStart + i, methodNameIndex, parameterStart, parameterCount))
                {
                    ++stats.SkippedInvalid;
                    continue;
                }

                if (HashNameByIndex(methodNameIndex, "method", typeName, stats, true))
                    ++stats.ModifiedMethods;

                if (layout.HashParameters)
                {
                    if (ValidIndexRange(parameterStart, parameterCount, parameterTotal))
                    {
                        for (uint32_t p = 0; p < parameterCount; ++p)
                        {
                            uint32_t paramNameIndex = 0;
                            if (GetParameterNameIndex(parameterStart + p, paramNameIndex) && HashNameByIndex(paramNameIndex, "param", typeName, stats))
                                ++stats.ModifiedParams;
                        }
                    }
                    else
                    {
                        ++stats.SkippedInvalid;
                    }
                }
            }
        }
        else
        {
            printf("  [warn] invalid method range start=%u count=%u total=%u\n", methodStart, methodCount, methodTotal);
            ++stats.SkippedInvalid;
        }

        if (ValidIndexRange(propertyStart, propertyCount, propertyTotal))
        {
            for (uint32_t i = 0; i < propertyCount; ++i)
            {
                uint32_t propertyNameIndex = 0;
                if (GetPropertyNameIndex(propertyStart + i, propertyNameIndex) && HashNameByIndex(propertyNameIndex, "prop", typeName, stats))
                    ++stats.ModifiedProperties;
            }
        }
        else
        {
            printf("  [warn] invalid property range start=%u count=%u total=%u\n", propertyStart, propertyCount, propertyTotal);
            ++stats.SkippedInvalid;
        }

        return true;
    }
}

void Metadata::SetTypeOnly(bool value)
{
    g_typeOnly = value;
}

void Metadata::SetVerbose(bool value)
{
    g_verbose = value;
}

bool Metadata::IsInternalType(const std::string& name)
{
    static std::unordered_set<std::string> list =
    {
        "Object", "Void", "Boolean", "Byte", "SByte", "Int16", "UInt16", "Int32", "UInt32", "UIntPtr",
        "IntPtr", "Int64", "UInt64", "Single", "Double", "Char", "String", "Enum", "Array", "ValueType",
        "Delegate", "MulticastDelegate", "AsyncResult", "MonoAsyncCall", "ManualResetEvent", "Type",
        "MonoType", "Thread", "InternalThread", "RuntimeType", "AppDomain", "AppDomainSetup", "MemberInfo",
        "FieldInfo", "MethodInfo", "PropertyInfo", "EventInfo", "StringBuilder", "StackFrame", "StackTrace",
        "TypedReference", "IList`1", "ICollection`1", "IEnumerable`1", "IReadOnlyList`1", "IReadOnlyCollection`1",
        "Nullable`1", "Version", "CultureInfo", "RuntimeAssembly", "AssemblyName", "RuntimeParameterInfo",
        "RuntimeModule", "Exception", "SystemException", "ArgumentException", "MarshalByRefObject", "__Il2CppComObject",
        "SafeHandle", "SortKey", "DBNull", "ErrorWrapper", "Missing", "Attribute", "CustomAttributeData",
        "CustomAttributeTypedArgument", "CustomAttributeNamedArgument", "KeyValuePair`2", "Guid", "_ThreadPoolWaitCallback",
        "MonoMethodMessage", "SByteEnum", "Int16Enum", "Int32Enum", "Int64Enum", "ByteEnum", "UInt16Enum",
        "UInt32Enum", "UInt64Enum", "__Il2CppFullySharedGenericType", "__Il2CppFullySharedGenericStructType", "GetHashCode",
        "Finalize", "Empty", "AudioSource", "Awake", "Start", "OnEnable", "OnDisable", "Update", "FixedUpdate",
        "LateUpdate", "OnDestroy", "OnApplicationQuit", "OnApplicationPause", "OnApplicationFocus", "OnGUI",
        "OnRenderObject", "OnWillRenderObject", "OnPreCull", "OnPreRender", "OnPostRender", "OnBecameVisible",
        "OnBecameInvisible", "OnCollisionEnter", "OnCollisionStay", "OnCollisionExit", "OnTriggerEnter", "OnTriggerStay",
        "OnTriggerExit", "OnMouseDown", "OnMouseUp", "OnMouseDrag", "OnMouseEnter", "OnMouseOver", "OnMouseExit",
        "OnLevelWasLoaded", "OnSceneLoaded", "OnSceneUnloaded", "OnAudioFilterRead", "OnAnimatorMove", "OnAnimatorIK",
        "OnControllerColliderHit", "Reset", "OnValidate", "OnServerInitialized", "OnConnectedToServer", "OnDisconnectedFromServer"
    };

    return list.find(name) != list.end();
}

bool Metadata::Process(std::vector<uint8_t>& buffer)
{
    if (buffer.size() < sizeof(Header))
        return false;

    g_buffer = &buffer;
    g_base = buffer.data();
    g_size = buffer.size();
    g_header = reinterpret_cast<Header*>(buffer.data());
    g_modifiedStringOffsets.clear();

    if (g_header->Sanity != 0xFAB11BAF)
    {
        printf("Invalid metadata sanity: 0x%08X\n", g_header->Sanity);
        return false;
    }

    if (!SectionValid(g_header->StringOffset, g_header->StringSize)
        || !SectionValid(g_header->TypeDefinitionsOffset, g_header->TypeDefinitionsSize)
        || !SectionValid(g_header->FieldsOffset, g_header->FieldsSize)
        || !SectionValid(g_header->MethodsOffset, g_header->MethodsSize)
        || !SectionValid(g_header->ParametersOffset, g_header->ParametersSize)
        || !SectionValid(g_header->PropertiesOffset, g_header->PropertiesSize))
    {
        printf("Invalid metadata section range. File size: %zu\n", g_size);
        return false;
    }

    printf("Version: %u\n", g_header->Version);
    printf("Mode: %s\n", g_typeOnly ? "type-only" : "type+members");
    printf("Section counts: fields=%u methods=%u params=%u props=%u\n",
        SectionCount(g_header->FieldsSize, sizeof(FieldDefinition)),
        SectionCount(g_header->MethodsSize, sizeof(MethodDefinition)),
        SectionCount(g_header->ParametersSize, sizeof(ParameterDefinition)),
        SectionCount(g_header->PropertiesSize, sizeof(PropertyDefinition)));

    const TypeLayout& layout = ChooseLayout();
    const uint32_t typeCount = SectionCount(g_header->TypeDefinitionsSize, layout.RecordSize);
    printf("Chosen type layout: %s, recordSize=%u, types=%u, methodRecordSize=%u, hashParams=%s\n",
        layout.Name, layout.RecordSize, typeCount, layout.MethodRecordSize, layout.HashParameters ? "yes" : "no");

    Stats stats;
    for (uint32_t i = 0; i < typeCount; ++i)
        ProcessType(layout, i, stats);

    printf("Summary:\n");
    printf("  matched target types: %u\n", stats.MatchedTypes);
    printf("  modified types:       %u\n", stats.ModifiedTypes);
    printf("  modified fields:      %u\n", stats.ModifiedFields);
    printf("  modified methods:     %u\n", stats.ModifiedMethods);
    printf("  modified parameters:  %u\n", stats.ModifiedParams);
    printf("  modified properties:  %u\n", stats.ModifiedProperties);
    printf("  skipped invalid:      %u\n", stats.SkippedInvalid);
    printf("  skipped internal:     %u\n", stats.SkippedInternal);
    printf("  skipped duplicates:   %u\n", stats.SkippedDuplicateString);

    return true;
}
