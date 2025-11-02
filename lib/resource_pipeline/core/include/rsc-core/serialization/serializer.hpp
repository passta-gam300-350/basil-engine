#ifndef RP_RSC_CORE_SERIALIZER_HPP
#define RP_RSC_CORE_SERIALIZER_HPP

#include "rsc-core/serialization/archive.hpp"

namespace rp {
	namespace serialization {
		template <rp::utility::static_string format>
		struct serializer {
			static_assert(std::false_type::value && "unknown serializer, define your own specialisation for each format. see serializer<\"bin\">");
		};
		template <>
		struct serializer<"bin"> {
			using in_archive_type = in_archive<"bin">;
			using out_archive_type = out_archive<"bin">;
			template <typename Type>
			static void serialize(Type const& val, std::string const& arcpath) {
				out_archive_type{ arcpath }.write(val);
			}
			template <typename Type>
			static void serialize(Type const& val, out_archive_type& arc) {
				arc.write(val);
			}
			template <typename Type>
			static void serialize(Type const& val, out_archive_type&& arc) {
				arc.write(val);
			}
			template <typename Type>
			[[nodiscard]] static Type deserialize(std::byte const* blob) {
				return in_archive_type{ blob }.read_blob<Type>();
			}
			template <typename Type>
			[[nodiscard]] static Type deserialize(std::string const& arcpath) {
				return in_archive_type{ arcpath }.read<Type>();
			}
			template <typename Type>
			[[nodiscard]] static Type deserialize(in_archive_type& arc) {
				return arc.read<Type>();
			}
			template <typename Type>
			[[nodiscard]] static Type deserialize(in_archive_type&& arc) {
				return arc.read<Type>();
			}
		};
		template <>
		struct serializer<"txt"> {
			using in_archive_type = in_archive<"txt">;
			using out_archive_type = out_archive<"txt">;
			template <typename Type>
			static void serialize(Type const& val, std::string const& arcpath) {
				out_archive_type{ arcpath }.write(val);
			}
			template <typename Type>
			static void serialize(Type const& val, out_archive_type& arc) {
				arc.write(val);
			}
			template <typename Type>
			static void serialize(Type const& val, out_archive_type&& arc) {
				arc.write(val);
			}
			template <typename Type>
			[[nodiscard]] static Type deserialize(Type const& val, std::string const& arcpath) {
				return in_archive_type{ arcpath }.read<Type>();
			}
			template <typename Type>
			[[nodiscard]] static Type deserialize(in_archive_type& arc) {
				return arc.read<Type>();
			}
			template <typename Type>
			[[nodiscard]] static Type deserialize(in_archive_type&& arc) {
				return arc.read<Type>();
			}
		};

		using binary_serializer = serializer<"bin">;
		using text_serializer = serializer<"txt">;
	}
}

#endif