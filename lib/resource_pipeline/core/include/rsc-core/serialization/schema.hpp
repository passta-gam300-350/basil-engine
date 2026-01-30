#ifndef RP_RSC_CORE_SCHEMA_HPP
#define RP_RSC_CORE_SCHEMA_HPP

#include <format>
#include <fstream>
#include "rsc-core/reflection/reflection.hpp"

namespace rp {
	namespace serialization {
		template <utility::static_string schema_name>
		struct schema_builder {
			static_assert(std::false_type::value && "unsupported archive format! define your own archive format specialisation, see in_archive<\"bin\">");
		};

		struct schema_full_sig_128 {
			std::uint64_t schema_name_hs;
			std::uint64_t schema_layout_hs;
		};

		template <utility::static_string schema_name_s, typename schema_type>
		struct schema_meta {
			using type = schema_type;
			static consteval std::uint64_t hashed_name() {
				return utility::string_hash(schema_name_s);
			}
			static consteval std::string_view schema_name() {
				return std::string_view(schema_name_s);
			}
			static consteval std::string_view schema_type_name() {
				return utility::get_type_name<schema_type>();
			}
			static consteval std::uint64_t schema_merged_signature() {
				constexpr auto field_typenames{ reflection::get_field_typenames<schema_type>() };
				std::uint64_t hash{ utility::string_hash(schema_name_s) };
				for (std::string_view field_typename : field_typenames) {
					hash = utility::string_hash(field_typename, hash);
				}
				return hash;
			}
			//schema signature of only the schema table
			static consteval std::uint64_t schema_signature() {
				constexpr auto field_typenames{ reflection::get_field_typenames<schema_type>() };
				std::uint64_t hash{ utility::string_hash("") };
				for (std::string_view field_typename : field_typenames) {
					hash = utility::string_hash(field_typename, hash);
				}
				return hash;
			}
			static consteval std::uint64_t schema_full_signature() {
				return schema_full_sig_128{hashed_name(), schema_signature()};
			}
			static consteval auto schema_data() {
				constexpr auto field_typenames{ reflection::get_field_typenames<schema_type>() };
				return field_typenames;
			}
		};
	}
}

#endif