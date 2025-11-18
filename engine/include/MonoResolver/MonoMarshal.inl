/******************************************************************************/
/*!
\file   MonoMarshal.inl
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the inline definition for the MonoMarshal class, which
is responsible for marshaling data between native and managed representations
in the Mono runtime.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/


#ifndef MONOMARSHAL_INLINE
#define MONOMARSHAL_INLINE
#include <concepts>
#include <string>
#include <type_traits>
#include <mono/utils/mono-publib.h>
#include <mono/metadata/object.h>
#include <mono/jit/jit.h>
#include "MonoMarshal.hpp"



template <typename T>
inline auto MonoMarshal::NativeToMono(T value) -> decltype(auto)
{

	return;
}


template <>
inline auto MonoMarshal::NativeToMono<bool>(bool value) -> decltype(auto)
{
	return value ? (mono_bool)1 : (mono_bool)0;
}

template <>
inline auto MonoMarshal::NativeToMono<int>(int value) -> decltype(auto)
{
	return static_cast<std::int32_t>(value);
}

template <>
inline auto MonoMarshal::NativeToMono<float>(float value) -> decltype(auto)
{
	return value;
}
template <>
inline auto MonoMarshal::NativeToMono<double>(double value) -> decltype(auto)
{
	return value;
}



template <>
inline auto MonoMarshal::NativeToMono<const char*>(const char* value) -> decltype(auto)
{
	MonoString* monoStr = mono_string_new(mono_domain_get(), value);
	return monoStr;
}


template <>
inline auto MonoMarshal::NativeToMono<uint64_t>(uint64_t value) -> decltype(auto)
{
	uint64_t managedValue = value;
	return managedValue;
}

#endif
