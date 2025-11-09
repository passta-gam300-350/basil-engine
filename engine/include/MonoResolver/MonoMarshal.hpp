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