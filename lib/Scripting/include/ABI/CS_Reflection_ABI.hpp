#ifndef CS_REFLECTION_ABI_HPP
#define CS_REFLECTION_ABI_HPP

#include <functional>

#include "ManagedAssembly.hpp"
struct ManagedAssembly;
struct CSKlass;
struct CSKlassInstance;

typedef struct _MonoClass MonoClass;
typedef struct _MonoImage MonoImage;
typedef struct _MonoMethod MonoMethod;
typedef struct MonoVTable MonoVTable;
typedef struct _MonoClassField MonoClassField;
typedef struct _MonoProperty MonoProperty;
typedef struct _MonoEvent MonoEvent;

using ClassEnumerator = std::function<void(char const* name_space, char const* name, MonoClass* klass, ManagedAssembly* assembly)>;

using PropertyEnumerator = std::function<void(char const* name, char const* type, CSKlassInstance* klass, MonoProperty* property)>;
using StaticPropertyEnumerator = std::function<void(char const* name, char const* type, CSKlass* klass, MonoProperty* property)>;

using FieldEnumerator = std::function<void(char const* name, char const* type ,CSKlassInstance* klass, MonoClassField* field)>;
using StaticFieldEnumerator = std::function<void(char const* name, char const* type, CSKlass* klass, MonoClassField* field)>;

using MethodEnumerator = std::function<void(char const* name, char const* retType, CSKlassInstance* klass, MonoMethod* method)>;
using StaticMethodEnumerator = std::function<void(char const* name, char const* retType, CSKlass* klass, MonoMethod* method)>;

using EventEnumerator = std::function<void(char const* name, char const* type, CSKlassInstance* klass, MonoEvent* event)>;
using StaticEventEnumerator = std::function<void(char const* name, char const* type, CSKlass* klass, MonoEvent* event)>;



#endif