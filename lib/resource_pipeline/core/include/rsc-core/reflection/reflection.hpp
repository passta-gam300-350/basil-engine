/*****************************************************************************/
/* lightweight reflection mainly for serialisation of descriptors and native */
/*****************************************************************************/
#ifndef RP_RSC_CORE_REFLECTION_HPP
#define RP_RSC_CORE_REFLECTION_HPP

#include <array>
#include <tuple>
#include <type_traits>
#include "rsc-core/utility.hpp"
#include "rsc-core/reflection/bindings.hpp"

namespace rp {
	namespace reflection {
		struct probe {
			template <typename Type>
			constexpr operator Type() const;
		};

		template <typename Type>
		struct refl_value_wrapper {
			using type = std::remove_cvref<Type>::type;
			Type m_value;
			constexpr refl_value_wrapper() = default;
			constexpr refl_value_wrapper(Type const& v) : m_value{v} {}
			constexpr refl_value_wrapper(Type && v) : m_value{ std::forward<Type>(v) } {}
			constexpr operator type& () { return m_value; };
			constexpr operator type const& () const { return m_value; };
		};

		template <typename Type, utility::static_string... ss>
		struct refl_field_tag : public refl_value_wrapper<Type> {
			using type = std::remove_cvref<Type>::type;
			using refl_value_wrapper<Type>::refl_value_wrapper;
			using refl_value_wrapper<Type>::operator type&;
			using refl_value_wrapper<Type>::operator type const&;

			static constexpr std::array<std::string_view, sizeof...(ss)> s_tags{ss...};
		};

		template <typename Type>
		constexpr auto& refl_decay(refl_value_wrapper<Type>& v) {
			return v.m_value;
		}

		//specialize this for external types// define arity, bindings
		template <typename Type>
		struct ExternalTypeMetadata;

		template <typename Type, auto... ptrs>
		using ExternalTypeBinderMetadata = internal::binding_helper<Type, ptrs...>;

		//specialise and add arity static member
		template <typename Type, typename = void>
		struct is_external_reflection_type : std::false_type {};

		template <typename Type>
		struct is_external_reflection_type<Type, std::void_t<decltype(sizeof(ExternalTypeMetadata<Type>))>> : std::true_type {};

		template <typename Type>
		struct is_custom_reflection_type {
			static constexpr bool value{ requires {
				{ Type::arity() } -> std::convertible_to<std::size_t>;
			} && requires{ !std::same_as<decltype(std::declval<Type>().arity()), bool>; } };
		};

		template <typename Type>
		static constexpr bool is_custom_reflection_type_v = is_custom_reflection_type<Type>::value;

		template <typename Type>
		static constexpr bool is_external_reflection_type_v = is_external_reflection_type<Type>::value;

		template <typename Type>
		concept arity_supported_types = std::is_aggregate_v<Type> || is_custom_reflection_type_v<std::remove_cvref_t<Type>> || is_external_reflection_type_v<std::remove_cvref_t<Type>>;

		template <typename Type, typename = void>
		struct is_sequence_container : std::false_type {};

		template <typename Type>
		struct is_sequence_container<Type, std::void_t<decltype(std::declval<Type>().begin()), decltype(std::declval<Type>().end()), typename Type::value_type>> : std::true_type {};

		template <typename Type>
		inline constexpr bool is_sequence_container_v = is_sequence_container<Type>::value;

		template <typename Type, typename = void>
		struct is_associative_container : std::false_type {};

		template <typename Type>
		struct is_associative_container<Type, std::void_t<typename Type::key_type, typename Type::mapped_type>> : std::true_type {};

		template <typename Type>
		inline constexpr bool is_associative_container_v = is_associative_container<Type>::value;

		template <typename, typename = void>
		struct is_iterator_type : std::false_type {};

		template <typename T>
		struct is_iterator_type<T, std::void_t<typename std::iterator_traits<T>::iterator_category>> : std::true_type {};

		template <typename T>
		inline constexpr bool is_iterator_type_v = is_iterator_type<T>::value;

		// Primary template: defaults to false
		template <typename, template <typename...> class>
		struct is_specialization_of : std::false_type {};

		// Partial specialization: true if T is an instantiation of Template<...>
		template <template <typename...> class Template, typename... Args>
		struct is_specialization_of<Template<Args...>, Template> : std::true_type {};

		// Helper variable template (C++14+)
		template <typename T, template <typename...> class Template>
		inline constexpr bool is_specialization_of_v = is_specialization_of<T, Template>::value;


		template <typename Type, std::size_t ProbeCount>
		struct is_aggregate_initializable_with {
			static constexpr bool value{ [] <size_t... is>(std::index_sequence<is...>) {
				return requires { Type{ (static_cast<void>(is), probe())... }; };
			}(std::make_index_sequence<ProbeCount>{}) };
		};

		template <typename Type, std::size_t ProbeCount>
		static constexpr bool is_aggregate_initializable_with_v = is_aggregate_initializable_with<Type, ProbeCount>::value;

		template <typename Type, size_t ProbeCount = 0>
		requires arity_supported_types<Type>
		consteval std::size_t get_arity() {
			if constexpr (is_custom_reflection_type_v<Type>)
				return Type::arity();
			if constexpr (is_external_reflection_type_v<Type>)
				return ExternalTypeMetadata<Type>::ExternalTypeBinder::arity();
			else if constexpr (is_aggregate_initializable_with_v<Type, ProbeCount>) //aggregate initialise until fail
				return get_arity<Type, ProbeCount + 1>();
			else
				return ProbeCount - 1;
		}

		template <typename Type>
		consteval auto get_enum_list() {
			if constexpr (is_custom_reflection_type_v<Type>)
				return EnumBindings<Type>();
			else
				return internal::enum_bruteforce_binding_helper<Type>::get_enum_bindings();
		}

		template <typename Type>
		auto map_enum_name(Type v) {
			auto list{ get_enum_list<Type>() };
			auto it{ std::find_if(list.begin(), list.end(), [v](const auto& entry) {
				return entry.m_enum_value == v;
				}) };
			return it == list.end() ? "" : it->m_enum_name;
		}

		template <typename Type>
		auto map_enum_value(std::string_view sv) {
			auto list{ get_enum_list<Type>() };
			auto it{ std::find_if(list.begin(), list.end(), [sv](const auto& entry) {
				return entry.m_enum_name == sv;
				}) };
			return it == list.end() ? list.begin()->m_enum_value : it->m_enum_value; //placeholder, return 1st if fails
		}

		template <auto Ptr>
		consteval std::string_view get_field_name() {
			return get_enum_name<Ptr>();
		}

		template <auto Ptr>
		consteval std::string_view get_field_name_from_ptr_to_field() {
			constexpr std::string_view func{ std::source_location::current().function_name() }; //cpp20++
#if defined(_MSC_VER)
			constexpr std::string_view prefix{ "->" };
			constexpr std::string_view suffix{ ">(void)" };
#else
#if defined(__clang__) //under maintainence, this part is not tested the implementation defined names might be different
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
			const auto end{ func.rfind(suffix) };
			return prefix_pos == std::string_view::npos || brace_pos > prefix_pos ? "" : func.substr(start, end - start);
		}

		template <typename Type>
		requires arity_supported_types<Type>
		consteval auto get_field_names() {
			if constexpr (is_external_reflection_type_v<std::remove_cvref_t<Type>>){
				using cvremoved_type = std::remove_cvref_t<Type>;
				return[]<auto ... ptrs>(internal::type_pointer_to_member_wrapper<cvremoved_type, ptrs...>) {
					return std::array{ get_field_name<ptrs>()... };
				}(ExternalTypeMetadata<cvremoved_type>::ExternalTypeBinder::get_ptr_bindings());
			}
			else {
				return[]<std::size_t ... is>(std::index_sequence<is...>) {
					constexpr auto ptrs{ internal::aggregate_binding_helper<Type, get_arity<Type>()>::get_ptr_bindings() };
					return std::array{ get_field_name_from_ptr_to_field<std::get<is>(ptrs)>()... };
				}(std::make_index_sequence<get_arity<Type>()>{});
			}
		}

		template <typename Type>
		struct refl_field_view {
			std::string_view m_field_name;
			Type* m_field_ptr;
		};

		struct refl_any{
		private:
			std::uint64_t m_typehash;
			std::string_view m_typename;
			void* m_data;
		public:
			template <typename Type>
			refl_any(Type* ptr) : m_typehash{ utility::string_hash(utility::get_type_name<std::remove_cvref_t<Type>>()) }, m_typename{ utility::get_type_name<std::remove_cvref_t<Type>>() }, m_data{ ptr } {}

			template <typename Type>
			Type* try_exact_cast() {
				return (utility::string_hash(utility::get_type_name<std::remove_cvref_t<Type>>()) == m_typehash) ? reinterpret_cast<Type*>(m_data) : nullptr;
			}
			std::string type_name() const {
				return std::string(m_typename.begin(), m_typename.end());
			}
		};

		struct refl_field_any_view {
			std::string_view m_field_name;
			refl_any m_field_value;
		};

		template <typename Type>
		struct type_wrapper {
			using type = Type;
		};
		
		template <typename SeqType>
		struct refl_seq_view {
			using underlying_type = SeqType::value_type;
			using reference_type = underlying_type&;
			using iterator = SeqType::iterator;
			using const_iterator = SeqType::const_iterator;

			SeqType& m_seqcont;

			refl_seq_view() = delete;
			refl_seq_view(SeqType& v) : m_seqcont{v} {}

			iterator begin() {
				return m_seqcont.begin();
			}
			const_iterator begin() const {
				return m_seqcont.begin();
			}
			iterator end() {
				return m_seqcont.end();
			}
			const_iterator end() const {
				return m_seqcont.end();
			}
			const_iterator cbegin() const {
				return m_seqcont.begin();
			}
			const_iterator cend() const {
				return m_seqcont.end();
			}

			std::size_t size() const {
				return m_seqcont.size();
			}

			template <typename ...Args>
			iterator emplace(const_iterator pos, Args&&... cargs) {
				return m_seqcont.emplace(pos, cargs...);
			}

			void reserve(std::size_t sz) {
				return m_seqcont.reserve(sz);
			}

			template <typename InvokableEach>
			void each(InvokableEach&& fn_each) {
				std::for_each(m_seqcont.begin(), m_seqcont.end(), std::forward<InvokableEach>(fn_each));
			}
		};

		template <typename AssociateType>
		struct refl_assoc_view {
			using mapped_type = AssociateType::mapped_type;
			using key_type = AssociateType::key_type;
			using underlying_type = AssociateType::value_type;
			using input_type = std::pair<key_type, mapped_type>;
			using reference_type = underlying_type&;
			using iterator = AssociateType::iterator;
			using const_iterator = AssociateType::const_iterator;

			AssociateType& m_asccont;

			refl_assoc_view() = delete;
			refl_assoc_view(AssociateType& v) : m_asccont{ v } {}

			iterator begin() {
				return m_asccont.begin();
			}
			const_iterator begin() const {
				return m_asccont.begin();
			}
			iterator end() {
				return m_asccont.end();
			}
			const_iterator end() const {
				return m_asccont.end();
			}
			const_iterator cbegin() const {
				return m_asccont.begin();
			}
			const_iterator cend() const {
				return m_asccont.end();
			}

			std::size_t size() const {
				return m_asccont.size();
			}

			iterator find(key_type const& key) {
				return m_asccont.find(key);
			}

			const_iterator find(key_type const& key) const{
				return m_asccont.find(key);
			}

			template <typename ...Args>
			std::pair<iterator, bool> emplace(Args&&... cargs) {
				return m_asccont.emplace(cargs...);
			}

			template <typename InvokableEach>
			void each(InvokableEach&& fn_each) {
				std::for_each(m_asccont.begin(), m_asccont.end(), std::forward<InvokableEach>(fn_each));
			}
		};

		template <typename Type>
		struct refl_array_any {
			using type = Type;
			using array_type = std::array<refl_field_any_view, get_arity<Type>()>;
			using iterator = array_type::iterator;
			using const_iterator = array_type::const_iterator;

			array_type m_array;
			template <typename... FieldAnys>
			refl_array_any(type_wrapper<Type>, FieldAnys&&...fields) : m_array{ std::forward<FieldAnys>(fields)... } {}
			iterator begin() {
				return m_array.begin();
			}
			const_iterator begin() const {
				return m_array.begin();
			}
			iterator end() {
				return m_array.end();
			}
			const_iterator end() const {
				return m_array.end();
			}
		};

		template <typename Type, typename... Fields>
		struct refl_tuple {
			using type = Type;
			std::tuple<Fields...> m_tuple;
			refl_tuple(type_wrapper<Type>, Fields&&...fields) : m_tuple{ std::forward_as_tuple(std::forward<Fields>(fields)...) } {}

			template <std::uint64_t index>
			auto get() {
				return std::get<index>(m_tuple);
			}
			template <std::uint64_t index>
			auto field_name() {
				return std::get<index>(m_tuple).m_field_name;
			}
			template <std::uint64_t index>
			auto value() {
				return std::get<index>(m_tuple).m_field_value;
			}
			template <typename Invokable>
			decltype(auto) apply(Invokable&& fn) {
				return std::apply(fn, m_tuple);
			}
			template <typename InvokableEach>
			void each(InvokableEach&& fn_each) {
				[] <std::size_t ...is>(auto& tuple, auto& fn, std::index_sequence<is...>) {
					(fn(std::get<is>(tuple)), ...);
				}(m_tuple, fn_each, std::make_index_sequence<std::tuple_size_v<decltype(m_tuple)>>{});
			}
		};

		template <typename Type>
		auto make_reflect_field_view(std::string_view sv, Type* v) {
			return refl_field_view<Type>{sv, v};
		}

		template <typename Type>
		auto make_reflect_field_any_view(std::string_view sv, Type* v) {
			return refl_field_any_view{ sv, refl_any{v} };
		}

		template <typename Type>
		auto make_reflect_tuple(Type& v) {
			if constexpr (is_external_reflection_type_v<std::remove_cvref_t<Type>>) {
				using cvremoved_type = std::remove_cvref_t<Type>;
				return[]<typename NameTuple, typename RefTuple, std::size_t ...is>(NameTuple const& nt, RefTuple rt, std::index_sequence<is...>) {
					return refl_tuple(type_wrapper<Type>{}, make_reflect_field_view(std::get<is>(nt), std::get<is>(rt))...);
				}(get_field_names<cvremoved_type>(), ExternalTypeMetadata<cvremoved_type>::ExternalTypeBinder::get_bindings_from_obj(v), std::make_index_sequence<get_arity<cvremoved_type>()>{});
			}
			else {
				return[]<typename NameTuple, typename RefTuple, std::size_t ...is>(NameTuple const& nt, RefTuple rt, std::index_sequence<is...>) {
					return refl_tuple(type_wrapper<Type>{}, make_reflect_field_view(std::get<is>(nt), std::get<is>(rt))...);
				}(get_field_names<Type>(), internal::aggregate_binding_helper<Type, get_arity<Type>()>::get_ptr_bindings_relative_to_object(v), std::make_index_sequence<get_arity<Type>()>{});
			}
		}

		template <typename Type>
		auto make_reflect_array_any(Type& v) {
			if constexpr (is_external_reflection_type_v<std::remove_cvref_t<Type>>) {
				using cvremoved_type = std::remove_cvref_t<Type>;
				return[]<typename NameTuple, typename RefTuple, std::size_t ...is>(NameTuple const& nt, RefTuple rt, std::index_sequence<is...>) {
					return refl_array_any(type_wrapper<Type>{}, make_reflect_field_any_view(std::get<is>(nt), std::get<is>(rt))...);
				}(get_field_names<cvremoved_type>(), ExternalTypeMetadata<cvremoved_type>::ExternalTypeBinder::get_bindings_from_obj(v), std::make_index_sequence<get_arity<cvremoved_type>()>{});
			}
			else {
				return[]<typename NameTuple, typename RefTuple, std::size_t ...is>(NameTuple const& nt, RefTuple rt, std::index_sequence<is...>) {
					return refl_array_any(type_wrapper<Type>{}, make_reflect_field_any_view(std::get<is>(nt), std::get<is>(rt))...);
				}(get_field_names<Type>(), internal::aggregate_binding_helper<Type, get_arity<Type>()>::get_ptr_bindings_relative_to_object(v), std::make_index_sequence<get_arity<Type>()>{});
			}
		}

		template <typename Type>
		auto make_reflect_seq(Type& v) {
			return refl_seq_view{ v };
		}

		template <typename Type>
		auto make_reflect_associative(Type& v) {
			return refl_assoc_view{ v };
		}

		template <typename Type>
		auto make_reflect_tuple_from_pair(Type& v) {
			constexpr auto name_array{ std::array{"first", "second"} };
			return refl_tuple(type_wrapper<Type>{}, make_reflect_field_view(std::get<0>(name_array), &v.first), make_reflect_field_view(std::get<1>(name_array), &v.second));
		}

		template <typename Type>
		auto reflect(Type& v) {
			if constexpr (is_associative_container_v<Type>) {
				return make_reflect_associative(v);
			}
			else if constexpr (is_sequence_container_v<Type>) {
				return make_reflect_seq(v);
			}
			else if constexpr (is_specialization_of_v<std::remove_cvref_t<Type>, std::pair>) {
				return make_reflect_tuple_from_pair(v);
			}
			else {
				return make_reflect_tuple(v);
			}
		}

		template <typename Type>
		auto reflect_runtime(Type& v) {
			return make_reflect_array_any(v);
		}
	}
}

#endif