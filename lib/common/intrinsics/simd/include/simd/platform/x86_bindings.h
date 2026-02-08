/******************************************************************************/
/*!
\file   x86_bindings.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  x86 SIMD type bindings

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef INTRIN_SIMD_TYPES_H
#define INTRIN_SIMD_TYPES_H
#include <immintrin.h>
#include <concepts>
#include <cassert>

//concepts and constraints
template <typename t>
concept primitive_type = requires(t ty) {
	{ std::is_fundamental_v<t> || std::is_pointer_v<t> };
};

/*template <typename t>
concept vectorized_type = requires(t ty) {
	{ std::is }
};*/	

template <typename from, typename to, int vt> requires primitive_type<to> /*&& vectorized_type<from>*/
struct is_vectorizable_to {
	static constexpr bool value{sizeof(to)*vt <= sizeof(from)};
};

template <typename from, typename to, int vt>
constexpr bool is_vectorizable_to_v{ is_vectorizable_to<from, to, vt>::value };

//__m128 type agnostic wrappers
template <typename t> requires primitive_type<t>
struct deduce_128bits {
	using type = __m128i;
};

template <>
struct deduce_128bits<float>{
	using type = __m128;
};

template <>
struct deduce_128bits<double> {
	using type = __m128d;
};

//__m256 type agnostic wrappers
template <typename t> requires primitive_type<t>
struct deduce_256bits {
	using type = __m256i;
};

template <>
struct deduce_256bits<float> {
	using type = __m256;
};

template <>
struct deduce_256bits<double> {
	using type = __m256d;
};

template <typename t>
struct b128 {
	using data_type = deduce_128bits<t>::type;
	
	b128() : _data{} {};
	b128(data_type const& dt) : _data{ dt } {};
	b128(b128<t> const& d) : _data{ d._data } {};
	~b128() {};

	b128& operator=(b128 const& rhs) { this->_data = rhs._data; return *this; }

	b128& operator+=(b128 const& rhs) { return *this = *this + rhs; }
	b128& operator-=(b128 const& rhs) { return *this = *this - rhs; }
	b128& operator*=(b128 const& rhs) { return *this = *this * rhs; }
	b128& operator/=(b128 const& rhs) { return *this = *this / rhs; }
	b128& operator^=(b128 const& rhs) { return *this = *this ^ rhs; }
	b128& operator&=(b128 const& rhs) { return *this = *this & rhs; }
	b128& operator|=(b128 const& rhs) { return *this = *this | rhs; }

	data_type _data;
};

template <typename t>
struct b256 {
	using data_type = deduce_256bits<t>::type;

	b256() : _data{} {};
	b256(data_type const& dt) : _data{ dt } {};
	b256(b256<t> const& d) : _data{ d._data } {};
	~b256() {};

	b256& operator=(b256 const& rhs) { return this->_data = rhs._data; }

	b256& operator+=(b256 const& rhs) { return *this = *this + rhs; }
	b256& operator-=(b256 const& rhs) { return *this = *this - rhs; }
	b256& operator*=(b256 const& rhs) { return *this = *this * rhs; }
	b256& operator/=(b256 const& rhs) { return *this = *this / rhs; }
	b256& operator^=(b256 const& rhs) { return *this = *this ^ rhs; }
	b256& operator&=(b256 const& rhs) { return *this = *this & rhs; }
	b256& operator|=(b256 const& rhs) { return *this = *this | rhs; }


	data_type _data;
};

//vectorised helper class
template <typename to, int ct> requires is_vectorizable_to_v<typename deduce_256bits<to>::type, to, ct>
struct v128b : public b128<to> {
	using type = to;
	
	v128b() = default;
	v128b(b128<to> const& b) : b128<to>(b) {};
	v128b(v128b<to, ct> const& v) : b128<to>(v._data) {};
	type& operator[](int idx) {
		assert(idx <= ct && "vector subscript outbounds");
		return reinterpret_cast<type*>(&(b128<type>::_data))[idx];
	}
	type const& operator[](int idx) const {
		assert(idx <= ct && "vector subscript outbounds");
		return reinterpret_cast<type*>(&(b128<type>::_data))[idx];
	}
};

template <typename to, int ct> requires is_vectorizable_to_v<typename deduce_256bits<to>::type, to, ct>
struct v256b : public b256<to> {
	using type = to;

	v256b() = default;
	type& operator[](int idx) {
		assert(idx <= ct && "vector subscript outbounds");
		return reinterpret_cast<type*>(&(b256<type>::_data))[idx];
	}
	type const& operator[](int idx) const {
		assert(idx <= ct && "vector subscript outbounds");
		return reinterpret_cast<type*>(&(b256<type>::_data))[idx];
	}
};

//opinion: disgusting constexpr branches maybe consider using template specialisation for better readability and possible speed up in compilation time if tad is faster than constexpr eval
template <typename t>
b128<t> operator+(b128<t> const& lhs, b128<t> const& rhs) {
	if constexpr (std::is_same_v<typename deduce_128bits<t>::type, __m128>) {
		return b128<t>(_mm_add_ps(lhs._data, rhs._data));
	}
	else if constexpr (std::is_same_v<typename deduce_128bits<t>::type, __m128d>) {
		return b128<t>(_mm_add_pd(lhs._data, rhs._data));
	}
	else if constexpr (std::is_signed_v<t>){ //assume __m128i for everything else since __mm128h and stuff are all typedefs of i anyways
		if constexpr (std::is_same_v<t, std::int8_t>) {
			return b128<t>(_mm_adds_epi8(lhs._data, rhs._data));
		}
		else if constexpr (std::is_same_v<t, std::int16_t>) {
			return b128<t>(_mm_adds_epi16(lhs._data, rhs._data));
		}
		else if constexpr (std::is_same_v<t, std::int32_t>) {
			return b128<t>(_mm_add_epi32(lhs._data, rhs._data));
		}
		else {
			return b128<t>(_mm_add_epi64(lhs._data, rhs._data));
		}
	}
	else {
		if constexpr (std::is_same_v<t, std::uint8_t>) {
			return b128<t>(_mm_adds_epu8(lhs._data, rhs._data));
		}
		else if constexpr (std::is_same_v<t, std::uint16_t>) {
			return b128<t>(_mm_adds_epu16(lhs._data, rhs._data));
		}
		else if constexpr (std::is_same_v<t, std::uint32_t>) {
			return b128<t>(_mm_add_epu32(lhs._data, rhs._data));
		}
		else {
			return b128<t>(_mm_add_epu64(lhs._data, rhs._data));
		}
	}
}

//opinion: disgusting constexpr branches maybe consider using template specialisation for better readability and possible speed up in compilation time if tad is faster than constexpr eval
template <typename t>
b128<t> operator-(b128<t> const& lhs, b128<t> const& rhs) {
	if constexpr (std::is_same_v<typename deduce_128bits<t>::type, __m128>) {
		return b128<t>(_mm_sub_ps(lhs._data, rhs._data));
	}
	else if constexpr (std::is_same_v<typename deduce_128bits<t>::type, __m128d>) {
		return b128<t>(_mm_sub_pd(lhs._data, rhs._data));
	}
	else if constexpr (std::is_signed_v<t>) { //assume __m128i for everything else since __mm128h and stuff are all typedefs of i anyways
		if constexpr (std::is_same_v<t, std::int8_t>) {
			return b128<t>(_mm_subs_epi8(lhs._data, rhs._data));
		}
		else if constexpr (std::is_same_v<t, std::int16_t>) {
			return b128<t>(_mm_subs_epi16(lhs._data, rhs._data));
		}
		else if constexpr (std::is_same_v<t, std::int32_t>) {
			return b128<t>(_mm_sub_epi32(lhs._data, rhs._data));
		}
		else {
			return b128<t>(_mm_sub_epi64(lhs._data, rhs._data));
		}
	}
	else {
		if constexpr (std::is_same_v<t, std::uint8_t>) {
			return b128<t>(_mm_subs_epu8(lhs._data, rhs._data));
		}
		else if constexpr (std::is_same_v<t, std::uint16_t>) {
			return b128<t>(_mm_subs_epu16(lhs._data, rhs._data));
		}
		else if constexpr (std::is_same_v<t, std::uint32_t>) {
			return b128<t>(_mm_sub_epu32(lhs._data, rhs._data));
		}
		else {
			return b128<t>(_mm_sub_epu64(lhs._data, rhs._data));
		}
	}
}

//opinion: disgusting constexpr branches maybe consider using template specialisation for better readability and possible speed up in compilation time if tad is faster than constexpr eval
template <typename t>
b128<t> operator*(b128<t> const& lhs, b128<t> const& rhs) {
	if constexpr (std::is_same_v<typename deduce_128bits<t>::type, __m128>) {
		return b128<t>(_mm_mul_ps(lhs._data, rhs._data));
	}
	else if constexpr (std::is_same_v<typename deduce_128bits<t>::type, __m128d>) {
		return b128<t>(_mm_mul_pd(lhs._data, rhs._data));
	}
	else if constexpr (std::is_signed_v<t>) { //assume __m128i for everything else since __mm128h and stuff are all typedefs of i anyways
		if constexpr (std::is_same_v<t, std::int16_t>) {
			return b128<t>(_mm_mullo_epi16(lhs._data, rhs._data));
		}
		else if constexpr (std::is_same_v<t, std::int32_t>) {
			return b128<t>(_mm_mullo_epi32(lhs._data, rhs._data));
		}
		else {
			return b128<t>(_mm_mullo_epi64(lhs._data, rhs._data));
		}
	}
	else {
		if constexpr (std::is_same_v<t, std::uint16_t>) {
			return b128<t>(_mm_mullo_epu16(lhs._data, rhs._data));
		}
		else if constexpr (std::is_same_v<t, std::uint32_t>) {
			return b128<t>(_mm_mullo_epu32(lhs._data, rhs._data));
		}
		else {
			return b128<t>(_mm_mullo_epu64(lhs._data, rhs._data));
		}
	}
}

//opinion: disgusting constexpr branches maybe consider using template specialisation for better readability and possible speed up in compilation time if tad is faster than constexpr eval
template <typename t>
b128<t> operator/(b128<t> const& lhs, b128<t> const& rhs) {
	if constexpr (std::is_same_v<typename deduce_128bits<t>::type, __m128>) {
		return b128<t>(_mm_div_ps(lhs._data, rhs._data));
	}
	else if constexpr (std::is_same_v<typename deduce_128bits<t>::type, __m128d>) {
		return b128<t>(_mm_div_pd(lhs._data, rhs._data));
	}
	else if constexpr (std::is_signed_v<t>) { //assume __m128i for everything else since __mm128h and stuff are all typedefs of i anyways
		if constexpr (std::is_same_v<t, std::int8_t>) {
			return b128<t>(_mm_div_epi8(lhs._data, rhs._data));
		}
		else if constexpr (std::is_same_v<t, std::int16_t>) {
			return b128<t>(_mm_div_epi16(lhs._data, rhs._data));
		}
		else if constexpr (std::is_same_v<t, std::int32_t>) {
			return b128<t>(_mm_div_epi32(lhs._data, rhs._data));
		}
		else {
			return b128<t>(_mm_div_epi64(lhs._data, rhs._data));
		}
	}
	else {
		if constexpr (std::is_same_v<t, std::uint8_t>) {
			return b128<t>(_mm_div_epu8(lhs._data, rhs._data));
		}
		else if constexpr (std::is_same_v<t, std::uint16_t>) {
			return b128<t>(_mm_div_epu16(lhs._data, rhs._data));
		}
		else if constexpr (std::is_same_v<t, std::uint32_t>) {
			return b128<t>(_mm_div_epu32(lhs._data, rhs._data));
		}
		else {
			return b128<t>(_mm_div_epu64(lhs._data, rhs._data));
		}
	}
}

//opinion: disgusting constexpr branches maybe consider using template specialisation for better readability and possible speed up in compilation time if tad is faster than constexpr eval
template <typename t>
b128<t> operator^(b128<t> const& lhs, b128<t> const& rhs) {
	if constexpr (std::is_same_v<typename deduce_128bits<t>::type, __m128>) {
		return b128<t>(_mm_xor_ps(lhs._data, rhs._data));
	}
	else if constexpr (std::is_same_v<typename deduce_128bits<t>::type, __m128d>) {
		return b128<t>(_mm_xor_pd(lhs._data, rhs._data));
	}
	else if constexpr (sizeof(t)<=sizeof(std::uint32_t)) { //assume __m128i for everything else since __mm128h and stuff are all typedefs of i anyways
		return b128<t>(_mm_xor_epi32(lhs._data, rhs._data));
	}
	else {
		return b128<t>(_mm_xor_epi64(lhs._data, rhs._data));
	}
}

template <typename t>
b128<t> operator&(b128<t> const& lhs, b128<t> const& rhs) {
	if constexpr (std::is_same_v<typename deduce_128bits<t>::type, __m128>) {
		return b128<t>(_mm_and_ps(lhs._data, rhs._data));
	}
	else if constexpr (std::is_same_v<typename deduce_128bits<t>::type, __m128d>) {
		return b128<t>(_mm_and_pd(lhs._data, rhs._data));
	}
	else if constexpr (sizeof(t) <= sizeof(std::uint32_t)) { //assume __m128i for everything else since __mm128h and stuff are all typedefs of i anyways
		return b128<t>(_mm_and_epi32(lhs._data, rhs._data));
	}
	else {
		return b128<t>(_mm_and_epi64(lhs._data, rhs._data));
	}
}

template <typename t>
b128<t> operator|(b128<t> const& lhs, b128<t> const& rhs) {
	if constexpr (std::is_same_v<typename deduce_128bits<t>::type, __m128>) {
		return b128<t>(_mm_or_ps(lhs._data, rhs._data));
	}
	else if constexpr (std::is_same_v<typename deduce_128bits<t>::type, __m128d>) {
		return b128<t>(_mm_or_pd(lhs._data, rhs._data));
	}
	else if constexpr (sizeof(t) <= sizeof(std::uint32_t)) { //assume __m128i for everything else since __mm128h and stuff are all typedefs of i anyways
		return b128<t>(_mm_or_epi32(lhs._data, rhs._data));
	}
	else {
		return b128<t>(_mm_or_epi64(lhs._data, rhs._data));
	}
}


#endif