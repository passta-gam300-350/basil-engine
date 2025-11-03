#ifndef MONOTYPE_DESCRIPTOR_HPP
#define MONOTYPE_DESCRIPTOR_HPP
#include <cstdint>
#include "rsc-core/rp.hpp"

enum struct Kind : uint8_t
{
    Unknown = 0,

    // ---- Primitives ----
    Bool,
    Int,
    UInt,
    Float,
    Double,
    String,

    // ---- Math / Structs ----
    Vec2,
    Vec3,
    Vec4,
    Quaternion,
    Color,

    // ---- Containers ----
    Array,
    VectorInt,
    VectorFloat,
    VectorGUID,
    VectorObject,

    // ---- Engine references ----
    GUID,          // asset / entity handle
    ObjectRef,     // pointer to an engine object

    // ---- Misc ----
    Enum,
    Custom,        // user-defined C++ type reflected at runtime
};

enum struct ManagedKind : uint8_t
{
    Unknown = 0,

    // ----- Primitives -----
    System_Void,
    System_Boolean,
    System_Char,
    System_Int8,
    System_UInt8,
    System_Int16,
    System_UInt16,
    System_Int32,
    System_UInt32,
    System_Int64,
    System_UInt64,
    System_Single,
    System_Double,
    System_String,

    // ----- Containers / Collections -----
    System_Array,
    System_List,
    System_Dictionary,

    // ----- Common Structs -----
    System_Vector2,
    System_Vector3,
    System_Vector4,
    System_Quaternion,
    System_Color,

    // ----- Engine-level references -----
    Engine_ObjectRef,  // any engine object handle
    Engine_GUID,       // asset/entity GUID reference

    // ----- Custom user-defined types -----
    Custom,
};

struct MonoTypeDescriptor
{
	Kind kind;
    ManagedKind managedKind;
    std::string cpp_name;
    std::string managed_name;

    uint32_t nativeHash = 0;

    // ---- Boolean metadata ----
    bool isValueType = false;
    bool isGeneric = false;
    bool isArray = false;
    bool isEngineType = false;
    bool isPrimitive = false;
    bool isSerializable = false;
    bool isUserType = false;
    bool isPublic = true;

    rp::Guid guid;

    const MonoTypeDescriptor* elementDescriptor = nullptr;
};

#endif
