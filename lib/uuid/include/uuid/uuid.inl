#ifndef UUID_INL
#define UUID_INL
#include <random>
template <size_t N>
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

	} else if constexpr (N == 64)
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

#endif // UUID_INL