/******************************************************************************/
/*!
\file   bindings.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Lightweight reflection for serialization

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef RP_RSC_CORE_BINDINGS_HPP
#define RP_RSC_CORE_BINDINGS_HPP

namespace rp {
	namespace reflection {
		template <typename EnumType>
		struct enum_binding_entry {
			std::string_view m_enum_name;
			EnumType m_enum_value;
		};

		//user defined enum bindings
		template <typename EnumType>
		consteval auto EnumBindings() {
			static_assert(std::is_enum_v<EnumType>, "Type is not an enum!");
			static_assert(sizeof(EnumType) == 0, "No matching overload! Define your own custom bindings for the type");
		}

		template <auto Ptr>
		consteval std::string_view get_enum_name() {
			constexpr std::string_view func{ std::source_location::current().function_name() }; //cpp20++
#if defined(_MSC_VER)
			constexpr std::string_view prefix{ "::" };
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
			const auto brace_pos{ func.rfind("<") };
			const auto prefix_pos{ func.rfind(prefix) };
			const auto start{ prefix_pos + prefix.size() };
			const auto end{ func.rfind(suffix) != std::string_view::npos ? func.rfind(suffix) : func.rfind(">()")};
			return prefix_pos == std::string_view::npos || brace_pos > prefix_pos || func.find(")") < end ? "" : func.substr(start, end - start);
		}

		namespace internal {
			template <typename Type>
			concept tuple_like_binding_type = requires{ typename std::tuple_size<Type>::type; };

			template <typename Type>
			struct static_wrapper {
				const Type m_value;
				static const static_wrapper<Type> static_tmp;
			};

			template <typename Type, auto...Ptr>
			struct type_pointer_to_member_wrapper {};

			template <typename Type>
			consteval const Type& compile_time_inspected_aggregate() {
				return static_wrapper<Type>::static_tmp.m_value;
			}

			template <typename Type, std::size_t Arity>
			struct aggregate_binding_helper {
				static_assert(std::is_aggregate_v<Type>, "Type is not an aggregate! Write your own implementation of static std::tuple<ptrs...> get_bindings() in your custom type");
				static_assert(sizeof(Type) == 0, "Arity unsupported! Update MakeAggregateBindings(Arity, ...) with another macro to support your arity");
			};

			template <typename Type, auto ...Ptrs>
			struct binding_helper {
				static constexpr auto get_bindings_from_obj(Type& obj) {
					const auto make_ptr_bindings{ [](auto& object) {
						return std::tuple{&(object.*Ptrs)...};
					} };
					return make_ptr_bindings(obj);
				}
				static constexpr auto get_bindings_from_obj(Type const& obj) {
					const auto make_ptr_bindings{ [](auto& object) {
						return std::tuple{&(object.*Ptrs)...};
					} };
					return make_ptr_bindings(obj);
				}
				static consteval auto get_ptr_bindings() {
					return type_pointer_to_member_wrapper<Type, Ptrs...>{};
				}
				//static constexpr std::size_t count{sizeof...(Ptrs)};
				static consteval std::size_t arity() {
					return sizeof...(Ptrs);
				}
			};

			template <typename Type>
			struct enum_bruteforce_binding_helper {
				static_assert(std::is_enum_v<Type>, "Type is not an enum!");
#ifndef RP_REFLECTION_IGNORE_ENUM_DEDUCTION_RESTRICTION
				static_assert(sizeof(std::underlying_type_t<Type>) == 1, "Underlying type is unsupported! Supported types are 1byte char(std::int8_t) and unsigned char(std::uint8_t). Overload EnumBindings() for custom enum support");
#endif

				using underlying_type = std::underlying_type_t<Type>;
				static constexpr std::size_t bruteforce_count = (std::numeric_limits<std::uint8_t>::max)() + 1ull;

				static consteval auto get_enum_bindings() {
					constexpr auto bf_tmp{ [] <std::size_t...is>(std::index_sequence<is...>) {
						return std::array{get_enum_name<static_cast<Type>(is)>()...};
					}(std::make_index_sequence<bruteforce_count>{}) };
					constexpr std::size_t elem_ct{ [](auto& arr) {
						std::size_t ct{};
						for (auto sv : arr)
							sv.empty() ? ct : ++ct;
						return ct;
					}(bf_tmp) };
					std::array<enum_binding_entry<Type>, elem_ct> enum_bindings{};
					std::size_t idx{};
					for (std::size_t i = 0; i < bf_tmp.size(); ++i) {
						if (!bf_tmp[i].empty()) {
							enum_bindings[idx++] = enum_binding_entry<Type>{ bf_tmp[i], static_cast<Type>(i) };
						}
					}
					return enum_bindings;
				}
			};
		}
	}
}

#define MakeAggregateBindings(ARITY, ...)																	\
namespace rp {																								\
	namespace reflection {																					\
		namespace internal {																				\
			template <typename Type>																		\
			struct aggregate_binding_helper<Type, ARITY> {													\
				static consteval auto get_ptr_bindings() {													\
					return get_ptr_bindings_relative_to_object_c(compile_time_inspected_aggregate<Type>());	\
				}																							\
				template <std::uint64_t i>																	\
				static consteval auto get_ptr_binding() {													\
					return std::get<i>(get_ptr_bindings());													\
				}																							\
				static constexpr auto get_ptr_bindings_relative_to_object(Type& obj){						\
					auto& [__VA_ARGS__] { obj };															\
					const auto make_ptr_bindings{ [](auto& ...refs) {										\
						return std::tuple{&refs...};														\
					} };																					\
					return make_ptr_bindings(__VA_ARGS__);													\
				}																							\
				static constexpr auto get_ptr_bindings_relative_to_object_c(Type const& obj) {				\
					auto const& [__VA_ARGS__] { obj };														\
					const auto make_ptr_bindings{ [](auto const& ...refs) {									\
						return std::tuple{&refs...};														\
					} };																					\
					return make_ptr_bindings(__VA_ARGS__);													\
				}																							\
			};																								\
		}																									\
	}																										\
}

#include "bindings.tdef"

#endif