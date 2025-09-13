#ifndef UUID_HPP
#define UUID_HPP
#include <cstdint>
#include <string>
#include <bitset>
#include <random>

/*
 4.4.  Algorithms for Creating a UUID from Truly Random or
      Pseudo-Random Numbers

   The version 4 UUID is meant for generating UUIDs from truly-random or
   pseudo-random numbers.

   The algorithm is as follows:

   o  Set the two most significant bits (bits 6 and 7) of the
      clock_seq_hi_and_reserved to zero and one, respectively.

   o  Set the four most significant bits (bits 12 through 15) of the
      time_hi_and_version field to the 4-bit version number from
      Section 4.1.3.

   o  Set all the other bits to randomly (or pseudo-randomly) chosen
      values.



Leach, et al.               Standards Track                    [Page 14]

RFC 4122                  A UUID URN Namespace                 July 2005


   See Section 4.5 for a discussion on random numbers.

*/


template <size_t N>
concept AllowedSize = (N == 128) || (N == 64);

template <size_t N>
	requires AllowedSize<N>
class UUID
{
	std::bitset<N> bits;


	static_assert(AllowedSize<N>, "UUID size must be either 64 or 128 bits");
public:
    static UUID Generate();
	UUID();
	template <size_t M>
		requires AllowedSize<M>
	UUID(const UUID<M>& other);
	template <size_t M>
		requires AllowedSize<M>
	UUID& operator=(const UUID<M>& other);

	template<size_t M>
		requires AllowedSize<M>
	bool operator==(const UUID<M>& other) const;
	template<size_t M>
		requires AllowedSize<M>
	bool operator!=(const UUID<M>& other) const;


	uint64_t getLow() const;
	uint64_t getHigh() const;

    std::string ToString();

};

template <size_t N>
	requires AllowedSize<N>
UUID<N> UUID<N>::Generate()
{
	static_assert(AllowedSize<N>, "UUID size must be either 64 or 128 bits");
	UUID<N> uuid;
	std::random_device rd;
	std::mt19937 gen(rd());
	if constexpr (N == 128)
	{
		// Call it twice to get 128 bits
		std::uniform_int_distribution<uint64_t> dis(0, UINT64_MAX);
		uint64_t part1 = dis(gen);
		uint64_t part2 = dis(gen);
		uuid.bits = (std::bitset<N>(part1) << 64) | std::bitset<N>(part2);
		// Set the version to 4 -- bits 12 through 15
		uuid.bits[76] = 0;
		uuid.bits[77] = 1;
		uuid.bits[78] = 0;
		uuid.bits[79] = 0;
		// Set the variant to 10xx -- bits 6 and 7 of the clock_seq_hi_and_reserved
		uuid.bits[62] = 1;
		uuid.bits[63] = 0;


		

	}
	else if constexpr (N == 64)
	{
		std::uniform_int_distribution<uint64_t> dis(0, UINT64_MAX);
		uint64_t part = dis(gen);
		uuid.bits = std::bitset<N>(part);
		// Set the version to 4 -- bits 12 through 15
		uuid.bits[12] = 0;
		uuid.bits[13] = 1;
		uuid.bits[14] = 0;
		uuid.bits[15] = 0;
		// Set the variant to 10xx -- bits 6 and 7 of the clock_seq_hi_and_reserved
		uuid.bits[6] = 1;
		uuid.bits[7] = 0;

		
	}

	
	return uuid;
}


template <size_t N>
	requires AllowedSize<N>
UUID<N>::UUID() : bits(0)
{
}
template <size_t N>
	requires AllowedSize<N>
template <size_t M>
	requires AllowedSize<M>
UUID<N>::UUID(const UUID<M>& other)
{
	if constexpr (N == M)
	{
		bits = other.bits;
		
	}
	else
	{
		bits = std::bitset<N>(other.bits.to_ullong());
	}
}

template <size_t N>
	requires AllowedSize<N>
template <size_t M>
	requires AllowedSize<M>
UUID<N>& UUID<N>::operator=(const UUID<M>& other)
{
	if (this != &other)
	{
		if constexpr (N == M)
		{
			bits = other.bits;
			
		}
		else
		{
			bits = std::bitset<N>(other.bits.to_ullong());
			

		}
	}
	return *this;
}

template <size_t N>
	requires AllowedSize<N>
template <size_t M>
	requires AllowedSize<M>
bool UUID<N>::operator==(const UUID<M>& other) const
{
	if constexpr (N == M)
	{
		return bits == other.bits;
	}
	else
	{
		return bits.to_ullong() == other.bits.to_ullong();
	}
}
template <size_t N>
	requires AllowedSize<N>
template <size_t M>
	requires AllowedSize<M>
bool UUID<N>::operator!=(const UUID<M>& other) const
{
	return !(*this == other);
}
template <size_t N>
	requires AllowedSize<N>
std::string UUID<N>::ToString()
{
	std::string result;
	if constexpr (N == 128)
	{
		// Format: 8-4-4-4-12
		// Example: 550e8400-e29b-41d4-a716-446655440000
		for (int i = 0; i < 32; i++)
		{
			if (i == 8 || i == 12 || i == 16 || i == 20)
			{
				result += '-';
			}
			uint8_t byte = 0;
			for (int j = 0; j < 4; j++)
			{
				byte = (byte << 1) | bits[N - 1 - (i * 4 + j)];
			}
			result += "0123456789abcdef"[byte];
		}
	} else if constexpr (N == 64)
	{
		// Format: 8-4-4-4-12 (same as 128 but shorter)
		// Example: 550e8400-e29b-41d4-a716
		for (int i = 0; i < 16; i++)
		{
			if (i == 8 || i == 12 || i == 16)
			{
				result += '-';
			}
			uint8_t byte = 0;
			for (int j = 0; j < 4; j++)
			{
				byte = (byte << 1) | bits[N - 1 - (i * 4 + j)];
			}
			result += "0123456789abcdef"[byte];
		}
	}
	else
	{
		throw std::runtime_error("Unsupported UUID size");
	}
	return result;
}



template <size_t N> requires AllowedSize<N>
uint64_t UUID<N>::getLow() const
{
	uint64_t low = 0;
	for (size_t i = 0; i < 64 && i < N; ++i)
	{
		if (bits.test(i))
		{
			low |= (1ULL << i);
		}
	}
}

template <size_t N> requires AllowedSize<N>
uint64_t UUID<N>::getHigh() const
{
	uint64_t high = 0;
	for (size_t i = 64; i < N; ++i)
	{
		if (bits.test(i))
		{
			high |= (1ULL << (i - 64));
		}
	}
	return high;
}



#endif  // UUID_HPP


