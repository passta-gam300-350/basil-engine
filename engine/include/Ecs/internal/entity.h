#ifndef LIB_ECS_ENTITY_H
#define LIB_ECS_ENTITY_H

#include <cstdint>
#include <string>
#include "ecs/fwd.h"
#include <rsc-core/guid.hpp>

namespace ecs {
	struct entity {
		using entity_id_t = uint32_t;
		
		struct entity_name_t {
			std::string m_name;

			entity_name_t() = default;
			entity_name_t(entity_name_t const&) = default;
			entity_name_t(entity_name_t&&) noexcept = default;
			entity_name_t(std::string const& e_name) : m_name{ e_name } {};
			entity_name_t(std::string&& e_name) noexcept : m_name{ std::move(e_name) } {};
			entity_name_t(std::string_view const& e_name) : m_name{ e_name } {};

			operator std::string() { return m_name; }
		};

		struct active_t {
			std::uint8_t m_tags; //reserved for future use
		};

	private:
		union {
			struct detail {
				std::uint64_t descriptor : 32; //mainly used for world
				std::uint64_t id : 32;
			} impl;
			std::uint64_t handle;
		};
		//invalids this class and set handle to invalid. does not delete underlying entity. use destroy if the use case is to detroy entity 
		void invalidate() {
			handle = null_handle64;
		}

	public:
		entity() : handle{ null_handle64 } {}
		entity(std::uint32_t desc, std::uint32_t uid) : impl{desc, uid} {}
		entity(std::uint64_t uuid) : handle{ uuid } {}

		operator entity_id_t() const {
			return impl.id;
		};
		operator bool() const;
		bool operator==(entity rhs) const {
			return this->handle == rhs.handle;
		}
		bool operator!=(entity rhs) const {
			return this->handle != rhs.handle;
		}

		template <typename component_t, typename ...c_args>
		component_t& add(c_args&&...);
		template <typename ...component_t>
		void remove();
		template <typename ...component_t>
		decltype(auto) get() const;
		template <typename ...requires_t>
		bool has() const;
		template <typename ...requires_t>
		bool all() const;
		template <typename ...requires_t>
		bool any() const;

		//upstream 1-1
		entity parent();
		//downstream 1-many
		decltype(auto) children();
		void destroy();

		//returns parent entity
		entity set_parent(entity);
		//return child entity
		entity add_child(entity);
		template <class... entity_t>
		decltype(auto) add_children();

		entity duplicate();
		std::string& name();
		std::string const& name() const;

		std::uint32_t get_world_handle() const {
			return impl.descriptor;  
		}

		//id in entt registry (not stable)
		std::uint32_t get_uid() const {
			return impl.id;
		}

		//id in world and entt registry (not stable)
		std::uint64_t get_uuid() const {
			return handle;
		}

		//stable scene handle in scene registry
		rp::Guid get_scene_handle() const;

		//stable id in scene
		std::uint32_t get_scene_uid() const;

		void enable();
		void disable();

		bool is_active() const;

		std::vector<std::pair<std::uint32_t, std::unique_ptr<std::byte[]>>> get_reflectible_components() const;
	};
}

#endif