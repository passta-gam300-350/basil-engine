#ifndef RP_RSC_CORE_ARCHIVE_HPP
#define RP_RSC_CORE_ARCHIVE_HPP

#include <format>
#include <fstream>
#include "rsc-core/reflection/reflection.hpp"

namespace rp{
	namespace serialization {
		template <utility::static_string format_name>
		struct in_archive {
			static_assert(std::false_type::value && "unsupported archive format! define your own archive format specialisation, see in_archive<\"bin\">");
		};

		template <utility::static_string format_name>
		struct out_archive {
			static_assert(std::false_type::value && "unsupported archive format! define your own archive format specialisation, see out_archive<\"bin\">");
		};

		template <>
		struct in_archive<"bin"> {
			std::ifstream m_ifs;
			std::byte const* m_data;
			std::byte const* m_read_ptr;
			in_archive() = delete;
			in_archive(std::byte const* data) : m_ifs{}, m_data{data}, m_read_ptr{data} {}
			in_archive(std::string const& filepath) : m_ifs{filepath, std::ios::binary}, m_data{}, m_read_ptr{} {}
			~in_archive() { m_ifs.close(); }
			template <typename Type>
			Type read() {
				if constexpr (std::is_trivially_copyable_v<Type>) {
					Type v{};
					m_ifs.read(reinterpret_cast<char*>(&v), sizeof(Type));
					return v;
				}
				else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, std::string>) {
					std::size_t cont_sz{ read<std::size_t>() };
					std::remove_cvref_t<Type> v(cont_sz, '\0');
					m_ifs.read(reinterpret_cast<char*>(v.data()), cont_sz);
					return v;
				}
				else if constexpr (reflection::is_associative_container_v<Type>) {
					Type v{};
					std::size_t cont_sz{ read<std::size_t>() };
					auto cont_view{ reflection::reflect(v) };
					//no reserve because not all containers supports reserve
					while (cont_sz--) {
						cont_view.emplace(read<decltype(cont_view)::input_type>());
					}
					return v;
				}
				else if constexpr (reflection::is_sequence_container_v<Type>) {
					Type v{};
					std::size_t cont_sz{ read<std::size_t>() };
					auto cont_view{ reflection::reflect(v) };
					using underlying_type = decltype(cont_view)::underlying_type;
					//fast deserialization if trivial
					if constexpr (cont_view.get_cont_traits().is_contiguous_v && std::is_trivially_copyable_v<underlying_type>) {
						cont_view.resize(cont_sz);
						if (cont_sz > 0) {
							m_ifs.read(reinterpret_cast<char*>(&(*cont_view.begin())), sizeof(underlying_type)*cont_sz);
						}
					}
					else {
						//this does not guarantees reserve mem, because not all containers supports reserve
						cont_view.reserve(cont_sz);
						while (cont_sz--) {
							cont_view.emplace(cont_view.cend(), read<underlying_type>());
						}
					}
					return v;
				}
				else if constexpr (std::is_class_v<Type>) {
					Type v{};
					reflection::reflect(v).each([&](auto& field) {
						*field.m_field_ptr = read<std::remove_pointer_t<std::remove_cvref_t<decltype(field.m_field_ptr)>>>();
						});
					return v;
				}
				else {
					return Type{};
				}
			}
			void blob_read_bytes(char* v, std::uint64_t sz) {
				std::memcpy(v, m_read_ptr, sz);
				m_read_ptr += sz;
			}
			template <typename Type>
			Type read_blob() {
				if constexpr (std::is_trivially_copyable_v<Type>) {
					Type v{};
					blob_read_bytes(reinterpret_cast<char*>(&v), sizeof(Type));
					return v;
				}
				else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, std::string>) {
					std::size_t cont_sz{ read_blob<std::size_t>() };
					std::remove_cvref_t<Type> v(cont_sz, '\0');
					blob_read_bytes(reinterpret_cast<char*>(v.data()), cont_sz);
					return v;
				}
				else if constexpr (reflection::is_associative_container_v<Type>) {
					Type v{};
					std::size_t cont_sz{ read_blob<std::size_t>() };
					auto cont_view{ reflection::reflect(v) };
					//no reserve because not all containers supports reserve
					while (cont_sz--) {
						cont_view.emplace(read_blob<decltype(cont_view)::input_type>());
					}
					return v;
				}
				else if constexpr (reflection::is_sequence_container_v<Type>) {
					Type v{};
					std::size_t cont_sz{ read_blob<std::size_t>() };
					auto cont_view{ reflection::reflect(v) };
					using underlying_type = decltype(cont_view)::underlying_type;
					//fast deserialization if trivial
					if constexpr (cont_view.get_cont_traits().is_contiguous_v && std::is_trivially_copyable_v<underlying_type>) {
						cont_view.resize(cont_sz);
						if (cont_sz > 0) {
							blob_read_bytes(reinterpret_cast<char*>(&(*cont_view.begin())), sizeof(underlying_type) * cont_sz);
						}
					}
					else {
						//this does not guarantees reserve mem, because not all containers supports reserve
						cont_view.reserve(cont_sz);
						while (cont_sz--) {
							cont_view.emplace(cont_view.cend(), read_blob<underlying_type>());
						}
					}
					return v;
				}
				else if constexpr (std::is_class_v<Type>) {
					Type v{};
					reflection::reflect(v).each([&](auto& field) {
						*field.m_field_ptr = read_blob<std::remove_pointer_t<std::remove_cvref_t<decltype(field.m_field_ptr)>>>();
						});
					return v;
				}
				else {
					return Type{};
				}
			}
		};

		template <>
		struct out_archive<"bin"> {
			std::ofstream m_ofs;
			out_archive() = delete;
			out_archive(std::string const& filepath) : m_ofs{ filepath, std::ios::binary } {}
			~out_archive() { m_ofs.close(); }
			template <typename Type>
			void write(Type const& v) {
				if constexpr (std::is_trivially_copyable_v<Type>) {
					m_ofs.write(reinterpret_cast<const char*>(&v), sizeof(Type));
				}
				else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, std::string>) {
					write(v.size());
					m_ofs.write(reinterpret_cast<const char*>(v.data()), v.size());
				}
				else if constexpr (reflection::is_associative_container_v<Type>) {
					auto cont{ reflection::reflect(v) };
					write(cont.size());
					cont.each([&](auto const& field) {
						write(field);
						});
				}
				else if constexpr (reflection::is_sequence_container_v<Type>) {
					auto cont{ reflection::reflect(v) };
					write(cont.size());
					using underlying_type = decltype(cont)::underlying_type;
					//fast serialization if trivial
					if constexpr (cont.get_cont_traits().is_contiguous_v && std::is_trivially_copyable_v<underlying_type>){
						if (cont.size() > 0) {
							m_ofs.write(reinterpret_cast<const char*>(&(*cont.begin())), sizeof(underlying_type)*cont.size());
						}
					}
					else {
						cont.each([&](auto const& field) {
							write(field);
							});
					}
				}
				else if constexpr (std::is_class_v<Type>) {
					reflection::reflect(v).each([&](auto const& field) {
						write(*field.m_field_ptr);
						});
				}
			}
		};

		template <>
		struct in_archive<"txt"> {
			std::ifstream m_ifs;
			in_archive() = delete;
			in_archive(std::string const& filepath) : m_ifs{ filepath, std::ios::in } {}
			~in_archive() { m_ifs.close(); }
			template <typename Type>
			Type read() {
				if constexpr (std::is_fundamental_v<Type>) {
					Type v{};
					m_ifs >> v;
					return v;
				}
				if constexpr (std::is_same_v<std::remove_cvref_t<Type>, std::string>) {
					Type v{};
					m_ifs >> v;
					return v.substr(1, v.size()-2);
				}
				else if constexpr (reflection::is_associative_container_v<Type>) {
					Type v{};
					auto cont_view{ reflection::reflect(v) };
					//no reserve because not all containers supports reserve
					cont_view.emplace(read<decltype(cont_view)::input_type>());
					return v;
				}
				else if constexpr (reflection::is_sequence_container_v<Type>) {
					Type v{};
					std::size_t cont_sz{ read<std::size_t>() };
					auto cont_view{ reflection::reflect(v) };
					cont_view.reserve(cont_sz);
					while (cont_sz--) {
						cont_view.emplace(cont_view.cend(), read<decltype(cont_view)::underlying_type>());
					}
					return v;
				}
				else if constexpr (std::is_class_v<Type>) {
					Type v{};
					reflection::reflect(v).each([&](auto& field) {
						*field.m_field_ptr = read<std::remove_pointer_t<std::remove_cvref_t<decltype(field.m_field_ptr)>>>();
						});
					return v;
				}
				else {
					return Type{};
				}
			}
		};

		template <>
		struct out_archive<"txt"> {
			std::ofstream m_ofs;
			out_archive() = delete;
			out_archive(std::string const& filepath) : m_ofs{ filepath, std::ios::out } {}
			~out_archive() { m_ofs.close(); }
			template <typename Type>
			void write(Type const& v, std::size_t padding = 0) {
				if constexpr (std::is_fundamental_v<Type>) {
					m_ofs << v<<'\n';
				}
				if constexpr (std::is_same_v<std::remove_cvref_t<Type>, std::string>) {
					m_ofs << '\"' << v << "\"\n";
				}
				else if constexpr (reflection::is_associative_container_v<Type> || reflection::is_sequence_container_v<Type>) {
					auto cont{ reflection::reflect(v) };
					cont.each([&](auto const& field) {
						write(field, padding);
						});
				}
				else if constexpr (std::is_class_v<Type>) {
					if (padding) {
						m_ofs << '\n';
					}
					reflection::reflect(v).each([&](auto const& field) {
						m_ofs << std::string(padding, ' ') << field.m_field_name << ": ";
						write(*field.m_field_ptr, padding + field.m_field_name.size());
						});
				}
			}
		};

		using binary_in_archive = in_archive<"bin">;
		using binary_out_archive = out_archive<"bin">;
		using text_in_archive = in_archive<"txt">;
		using text_out_archive = out_archive<"txt">;
	}
}

#endif