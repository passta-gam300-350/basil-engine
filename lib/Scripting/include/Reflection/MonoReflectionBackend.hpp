#ifndef MONO_REFLECTION_BACKEND_HPP
#define MONO_REFLECTION_BACKEND_HPP

#include "ABI/CS_Reflection_ABI.hpp"

class MonoReflectionBackend {
public:
	static void EnumerateClass(ManagedAssembly* assembly, ClassEnumerator const& enumerator);

	static void EnumerateField(CSKlass* klass, StaticFieldEnumerator const& enumerator);
	static void EnumerateField(CSKlassInstance* instance, FieldEnumerator const& enumerator);

	static void EnumerateProperty(CSKlass* klass, StaticPropertyEnumerator const& enumerator);
	static void EnumerateProperty(CSKlassInstance* instance, PropertyEnumerator const& enumerator);

	static void EnumerateMethod(CSKlass* klass, StaticMethodEnumerator const& enumerator);
	static void EnumerateMethod(CSKlassInstance* instance, MethodEnumerator const& enumerator);

	static void EnumerateEvent(CSKlass* klass, StaticEventEnumerator const& enumerator);
	static void EnumerateEvent(CSKlassInstance* instance, EventEnumerator const& enumerator);



};


#endif