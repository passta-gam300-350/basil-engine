/******************************************************************************/
/*!
\file   MonoMarshal.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the declaration for the MonoMarshal class, which
is responsible for marshaling data between native and managed representations
in the Mono runtime.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef MONOMARSHAL_HPP

#define MONOMARSHAL_HPP
#include <cstdint>
typedef struct _MonoObject MonoObject;
typedef std::int32_t		mono_bool;
class MonoMarshal
{
public:
	template <typename T>
	static auto NativeToMono(T value) -> decltype(auto);

};

#include "MonoResolver/MonoMarshal.inl"
#endif // MONOMARSHAL_HPP