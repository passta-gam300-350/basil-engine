#ifndef RP_RSC_CORE_SERIALIZER_HPP
#define RP_RSC_CORE_SERIALIZER_HPP

#include "rsc-core/serialization/archive.hpp"
#include <yaml-cpp/yaml.h>

namespace rp {
	namespace serialization {
		template <rp::utility::static_string format>
		struct serializer {
			static_assert(format.m_data[0] == '\0' && false, "unknown serializer, define your own specialisation for each format. see serializer<\"bin\">");
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


namespace rp {
	namespace serialization {
		template <>
		struct in_archive<"yaml"> {
			YAML::Node nd;
			in_archive() = delete;
			in_archive(std::string const& filepath) : nd{ YAML::LoadFile(filepath) } {}
			~in_archive() {}
			template <typename Type>
			Type read(YAML::Node const& cnd) {
				if constexpr (std::is_enum_v<Type>) {
					return reflection::map_enum_value<Type>(cnd.as<std::string>());
				}
				else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, rp::Guid>) {
					return rp::Guid::to_guid(cnd.as<std::string>());
				}
				else if constexpr (std::is_fundamental_v<Type> || std::is_same_v<std::remove_cvref_t<Type>, std::string>) {
					return cnd.as<Type>();
				}
				else if constexpr (reflection::is_associative_container_v<Type>) {
					Type v{};
					if (cnd.IsMap()) {
						for (auto it = cnd.begin(); it != cnd.end(); ++it) {
							v[read<typename Type::key_type>(it->first)] = read<typename Type::mapped_type>(it->second);
						}
					}
					return v;
				}
				else if constexpr (reflection::is_sequence_container_v<Type>) {
					if (!cnd.IsDefined() || cnd.IsNull()) {
						return Type{};  // Return empty container for null/undefined nodes
					}
					if constexpr (std::is_fundamental_v<typename Type::value_type> || std::is_same_v<std::remove_cvref_t<typename Type::value_type>, std::string>) {
						return cnd.as<Type>();
					}
					else {
						Type v{};
						for (auto it = cnd.begin(); it != cnd.end(); ++it) {
							v.emplace(v.end(), read<typename Type::value_type>(*it));
						}
						return v;
					}
				}
				else if constexpr (std::is_class_v<Type>) {
					Type v{};
					reflection::reflect(v).each([&](auto& field) {
						*field.m_field_ptr = read<std::remove_pointer_t<std::remove_cvref_t<decltype(field.m_field_ptr)>>>(cnd[std::string(field.m_field_name.begin(), field.m_field_name.end())]);
						});
					return v;
				}
				else {
					return Type{};
				}
			}
			template <typename Type>
			Type read() {
				return read<Type>(nd);
			}
		};

		template <>
		struct out_archive<"yaml"> {
			std::ofstream m_ofs;
			YAML::Node out;
			out_archive() = delete;
			out_archive(std::string const& filepath) : m_ofs{ filepath } {}
			~out_archive() { m_ofs << out; m_ofs.close(); }
			template <typename Type>
			void write(Type const& v, YAML::Node& nd) {
				if constexpr (std::is_enum_v<Type>) {
					nd = std::string(reflection::map_enum_name(v));
				}
				else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, rp::Guid>) {
					nd = v.to_hex();
				}
				else if constexpr (std::is_fundamental_v<Type> || std::is_same_v<std::remove_cvref_t<Type>, std::string>) {
					nd = v;
				}
				else if constexpr (reflection::is_associative_container_v<Type>) {
					auto cont{ reflection::reflect(v) };
					cont.each([&](auto const& field) {
						YAML::Node member{};
						write(field.second, member);
						nd[field.first] = member;
						});
				}
				else if constexpr (reflection::is_sequence_container_v<Type>) {
					auto cont{ reflection::reflect(v) };
					cont.each([&](auto const& field) {
						YAML::Node member{};
						write(field, member);
						nd.push_back(member);
						});
				}
				else if constexpr (std::is_class_v<Type>) {
					reflection::reflect(v).each([&](auto const& field) {
						YAML::Node member{};
						write(*field.m_field_ptr, member);
						nd[std::string(field.m_field_name.begin(), field.m_field_name.end())] = member;
						});
				}
			}
			template <typename Type>
			void write(Type const& v) {
				write(v, out);
			}
		};

		template <>
		struct serializer<"yaml"> {
			using in_archive_type = in_archive<"yaml">;
			using out_archive_type = out_archive<"yaml">;
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

		using yaml_serializer = serializer<"yaml">;
	}
}


#endif