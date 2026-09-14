#pragma once

namespace Metadata
{
    typedef struct _FieldDefinition
    {
        uint32_t NameIndex;
        uint32_t TypeIndex;
        uint32_t Token;
    } FieldDefinition;

    typedef struct _PropertyDefinition
    {
        uint32_t NameIndex;
        uint32_t Get;
        uint32_t Set;
        uint32_t Attrs;
        uint32_t Token;
    } PropertyDefinition;

    typedef struct _ParameterDefinition
    {
        uint32_t NameIndex;
        uint32_t Token;
        uint32_t TypeIndex;
    } ParameterDefinition;

    typedef struct _MethodDefinition
    {
        uint32_t NameIndex;
        uint32_t DeclaringType;
        uint32_t ReturnType;
        uint32_t ReturnParameterToken;
        uint32_t ParameterStart;
        uint32_t GenericContainerIndex;
        uint32_t Token;
        uint16_t Flags;
        uint16_t IFlags;
        uint16_t Slot;
        uint16_t ParameterCount;
    } MethodDefinition;

#pragma pack(push, p1, 4)
    typedef struct _Header
    {
        uint32_t Sanity;
        uint32_t Version;
        uint32_t StringLiteralOffset;
        uint32_t StringLiteralSize;
        uint32_t StringLiteralDataOffset;
        uint32_t StringLiteralDataSize;
        uint32_t StringOffset;
        uint32_t StringSize;
        uint32_t EventsOffset;
        uint32_t EventsSize;
        uint32_t PropertiesOffset;
        uint32_t PropertiesSize;
        uint32_t MethodsOffset;
        uint32_t MethodsSize;
        uint32_t ParameterDefaultValuesOffset;
        uint32_t ParameterDefaultValuesSize;
        uint32_t FieldDefaultValuesOffset;
        uint32_t FieldDefaultValuesSize;
        uint32_t FieldAndParameterDefaultValueDataOffset;
        uint32_t FieldAndParameterDefaultValueDataSize;
        uint32_t FieldMarshaledSizesOffset;
        uint32_t FieldMarshaledSizesSize;
        uint32_t ParametersOffset;
        uint32_t ParametersSize;
        uint32_t FieldsOffset;
        uint32_t FieldsSize;
        uint32_t GenericParametersOffset;
        uint32_t GenericParametersSize;
        uint32_t GenericParameterConstraintsOffset;
        uint32_t GenericParameterConstraintsSize;
        uint32_t GenericContainersOffset;
        uint32_t GenericContainersSize;
        uint32_t NestedTypesOffset;
        uint32_t NestedTypesSize;
        uint32_t InterfacesOffset;
        uint32_t InterfacesSize;
        uint32_t VtableMethodsOffset;
        uint32_t VtableMethodsSize;
        uint32_t InterfaceOffsetsOffset;
        uint32_t InterfaceOffsetsSize;
        uint32_t TypeDefinitionsOffset;
        uint32_t TypeDefinitionsSize;
        uint32_t ImagesOffset;
        uint32_t ImagesSize;
        uint32_t AssembliesOffset;
        uint32_t AssembliesSize;
        uint32_t FieldRefsOffset;
        uint32_t FieldRefsSize;
        uint32_t ReferencedAssembliesOffset;
        uint32_t ReferencedAssembliesSize;
        uint32_t AttributeDataOffset;
        uint32_t AttributeDataSize;
        uint32_t AttributeDataRangeOffset;
        uint32_t AttributeDataRangeSize;
        uint32_t UnresolvedIndirectCallParameterTypesOffset;
        uint32_t UnresolvedIndirectCallParameterTypesSize;
        uint32_t UnresolvedIndirectCallParameterRangesOffset;
        uint32_t UnresolvedIndirectCallParameterRangesSize;
        uint32_t WindowsRuntimeTypeNamesOffset;
        uint32_t WindowsRuntimeTypeNamesSize;
        uint32_t WindowsRuntimeStringsOffset;
        uint32_t WindowsRuntimeStringsSize;
        uint32_t ExportedTypeDefinitionsOffset;
        uint32_t ExportedTypeDefinitionsSize;
    } Header;
#pragma pack(pop, p1)

    bool IsInternalType(const std::string& name);

    // Optional flags are set by Entry.cpp.
    void SetTypeOnly(bool value);
    void SetVerbose(bool value);

    bool Process(std::vector<uint8_t>& buffer);
}
