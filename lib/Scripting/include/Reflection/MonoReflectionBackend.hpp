/******************************************************************************/
/*!
\file   MonoReflectionBackend.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the declaration for the MonoReflectionBackend class, which
provides an interface for reflecting over managed types and their members in the Mono runtime.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
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