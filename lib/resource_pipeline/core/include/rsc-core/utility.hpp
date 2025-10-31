#ifndef RP_RSC_CORE_UTILITY_HPP
#define RP_RSC_CORE_UTILITY_HPP

#include <string>
#include <utility>
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <string_view>
#include <filesystem>
#include <source_location>

namespace rp {
	namespace utility {
		//creates a bit mask of len bit from the least significant bit.
		template <std::uint64_t bit_len, std::uint64_t bit>
		struct lo_bitmask_t {
			static constexpr std::uint64_t mask{ (std::uint64_t(0x1) << bit) - 1 };

			static_assert(bit_len <= 64 && "invalid bit len!");
			static_assert(bit <= bit_len && "invalid bit field!");
		};
		template <std::uint64_t bit_len, std::uint64_t bit>
		struct hi_bitmask_t {
			static constexpr std::uint64_t mask{ ~lo_bitmask_t<bit_len, bit_len - bit>::mask };
		};
		template <std::size_t sz>
		struct static_string {
			char m_data[sz];
			//const char* m_raw;

			char* data() { return m_data; };
			const char* c_str() { return m_data; };
			std::string str() const { return std::string(m_data); };
			constexpr std::size_t size() { return sz; };

			constexpr static_string(const char(&str)[sz]) {
				std::copy(str, str+sz, m_data);
				//m_raw = str;
			}
			constexpr operator std::string_view() const {
				return std::string_view(m_data, sz - 1);
			}
		};
		//FNV-1a https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
		//strict
		consteval std::uint64_t string_hash(std::string_view str, std::uint64_t hash = 14695981039346656037ull) {
			for (char c : str) {
				hash ^= static_cast<std::uint64_t>(c);
				hash *= 1099511628211ull; //FNV prime
			}
			return hash;
		}
		//FNV-1a
		//maybe constexpr
		constexpr std::uint64_t compute_string_hash(std::string_view str, std::uint64_t hash = 14695981039346656037ull) {
			for (char c : str) {
				hash ^= static_cast<std::uint64_t>(c);
				hash *= 1099511628211ull; //FNV prime
			}
			return hash;
		}
		template<typename T>
		consteval std::string_view get_type_name() {
			constexpr std::string_view func{ std::source_location::current().function_name() }; //cpp20++
#if defined(_MSC_VER)
			constexpr std::string_view prefix{ "get_type_name<" };
			constexpr std::string_view suffix{ ">(void)" };
#else
#if defined(__clang__)
			constexpr std::string_view prefix{ "get_type_name() [T = " };
			constexpr std::string_view suffix{ "]" };
#else
			constexpr std::string_view prefix{ "get_type_name() [with T = " };
			constexpr std::string_view suffix{ "]" };
#endif
#endif
			const auto start = func.find(prefix) + prefix.size();
			const auto end = func.rfind(suffix);
			return func.substr(start, end - start);
		}

		template <typename Type>
		struct type_hash {
			static consteval std::uint64_t value() {
				return string_hash(get_type_name<Type>());
			}
		};
		std::string get_relative_path(std::string const& path, std::string const& base = {}) {
			std::filesystem::path pth{ path };
			return pth.is_relative() ? path : std::filesystem::relative(path, base.empty() ? std::filesystem::current_path() : std::filesystem::path{base}).string();
		}
	}
	template <std::uint64_t bit>
	constexpr std::uint8_t lo_bitmask8_v{ static_cast<std::uint32_t>(utility::lo_bitmask_t<8, bit>::mask) };
	template <std::uint64_t bit>
	constexpr std::uint16_t lo_bitmask16_v{ static_cast<std::uint32_t>(utility::lo_bitmask_t<16, bit>::mask) };
	template <std::uint64_t bit>
	constexpr std::uint32_t lo_bitmask32_v{ static_cast<std::uint32_t>(utility::lo_bitmask_t<32, bit>::mask) };
	template <std::uint64_t bit>
	constexpr std::uint64_t lo_bitmask64_v{ utility::lo_bitmask_t<64, bit>::mask };

	template <std::uint64_t bit>
	constexpr std::uint8_t hi_bitmask8_v{ static_cast<std::uint32_t>(utility::hi_bitmask_t<8, bit>::mask) };
	template <std::uint64_t bit>
	constexpr std::uint16_t hi_bitmask16_v{ static_cast<std::uint32_t>(utility::hi_bitmask_t<16, bit>::mask) };
	template <std::uint64_t bit>
	constexpr std::uint32_t hi_bitmask32_v{ static_cast<std::uint32_t>(utility::hi_bitmask_t<32, bit>::mask) };
	template <std::uint64_t bit>
	constexpr std::uint64_t hi_bitmask64_v{ utility::hi_bitmask_t<64, bit>::mask };
}

#endif