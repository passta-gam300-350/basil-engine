#include <vector>
#include "ecs/internal/world.h"
#include "ecs/internal/entity.h"
#include "ecs/internal/reflection.h"
#include "ecs/utility/utility.h"
#include "ecs/system/scheduler.h"

#include <entt/entity/snapshot.hpp>

#include <yaml-cpp/yaml.h>

namespace ecs {

	std::unique_ptr<std::vector<entt::registry>> worlds{};
	std::unique_ptr<std::vector<Scheduler>> world_systems{};

	STRONG_INLINE std::uint64_t make_qword(std::uint32_t hi_dword, std::uint32_t low_dword) { return (static_cast<std::uint64_t>(hi_dword) << 32) | (low_dword); }

	entt::registry& get_world_registry(std::uint32_t wid)
	{
		return WorldRegistry::Instance()[wid];
	}

	entt::registry& get_world_registry(world wrld)
	{
		return WorldRegistry::Instance()[wrld.impl.handle];
	}

	entt::registry& world::detail::get_registry() const
	{
		return get_world_registry(handle);
	}

	Scheduler& get_system_scheduler(world wrld)
	{
		return (*world_systems)[wrld.impl.handle];
	}

	Scheduler& world::detail::get_scheduler() const
	{
		return get_system_scheduler(handle);
	}

	STRONG_INLINE entt::entity world::detail::entt_entity_cast(ecs::entity enty)
	{
		return static_cast<entt::entity>(enty.get_uid());
	}

	STRONG_INLINE std::uint32_t world::detail::entity_id_cast(entt::entity entt_entity)
	{
		return static_cast<std::uint32_t>(entt_entity);
	}

	ecs::entity world::detail::entity_cast(entt::entity entt_entity) const
	{
		return ecs::entity{ handle, static_cast<std::uint32_t>(entt_entity) };
	}

	world world::new_world_instance()
	{
		return WorldRegistry::NewWorld();
	}

	void world::destroy_world()
	{
		WorldRegistry::EraseWorld(*this);
	}

	world::operator bool() const {
		return impl.handle != invalid_handle && WorldRegistry::Exists(*this);
	}

	bool world::is_valid(entity e) {
		return impl.get_registry().valid(detail::entt_entity_cast(e.get_uid()));
	}

	WorldRegistry& WorldRegistry::Instance() {
		static WorldRegistry reg_instance{};
		return reg_instance;
	}

	bool WorldRegistry::Exists(world w) {
		WorldRegistry& wreg{ Instance() };
		return w.impl.handle < wreg.m_sparse_handles.size() && wreg.m_sparse_handles[w.impl.handle];
	}

	world WorldRegistry::NewWorld() {
		WorldRegistry& wreg{ Instance() };
		if (wreg.m_FreeList == null_handle31) {
			std::uint32_t internal_id{ static_cast<std::uint32_t>(wreg.m_packed_worlds.size()) };
			std::uint32_t hdl{ static_cast<std::uint32_t>(wreg.m_sparse_handles.size()) };
			wreg.m_packed_worlds.emplace_back(hdl, entt::registry());
			wreg.m_sparse_handles.emplace_back(internal_id);
			return world(hdl);
		}
		else {
			std::uint32_t hdl{ wreg.m_FreeList };
			std::uint32_t internal_id{ static_cast<std::uint32_t>(wreg.m_packed_worlds.size()) };
			wreg.m_FreeList = wreg.m_sparse_handles[wreg.m_FreeList].m_Idx;
			wreg.m_packed_worlds.emplace_back(hdl, entt::registry());
			wreg.m_sparse_handles[hdl].m_Idx = internal_id;
			wreg.m_sparse_handles[hdl].m_Destroy = 0;
			return world(hdl);
		}
	}

	void WorldRegistry::EraseWorld(world wrld) {
		WorldRegistry& wreg{ Instance() };
		if (wreg.m_packed_worlds.size() == 1) {
			wreg.m_packed_worlds.clear();
		}
		else {
			wreg.m_sparse_handles[wreg.m_packed_worlds.back().first].m_Idx = wreg.m_sparse_handles[wrld.impl.handle].m_Idx;
			std::swap(wreg.m_packed_worlds[wreg.m_sparse_handles[wrld.impl.handle].m_Idx], wreg.m_packed_worlds.back());
			wreg.m_packed_worlds.pop_back();
		}
		wreg.m_sparse_handles[wrld.impl.handle].m_Idx = wreg.m_FreeList;
		wreg.m_sparse_handles[wrld.impl.handle].m_Destroy = 1;
		wreg.m_FreeList = wrld.impl.handle;
	}

	void WorldRegistry::Clear() {
		WorldRegistry& wreg{ Instance() };
		wreg.m_FreeList = null_handle31;
		wreg.m_packed_worlds.clear();
		wreg.m_sparse_handles.clear();
	}

	entt::registry& WorldRegistry::operator[](int idx) {
		WorldRegistry& wreg{ Instance() };
		assert(wreg.m_sparse_handles[idx].m_Idx != null_handle31);
		return wreg.m_packed_worlds[wreg.m_sparse_handles[idx].m_Idx].second;
	}

	void world::serialise_world_bin(std::string const& outputFilename) {
		std::ofstream out(outputFilename, std::ios::binary);
		entt::snapshot snap{ impl.get_registry() };
		auto out_archive{ [&out](auto value) {
			out.write(reinterpret_cast<const char*>(&value), sizeof(value));
			} };
		snap.template get<entt::entity>(out_archive);
		for (auto& ser : ReflectionRegistry::BinSerializerRegistryInstance()) {
			ser.m_Saver(snap, out);
		}
	}

	void world::deserialise_world_bin(std::string const& inputFilename) {
		std::ifstream in(inputFilename, std::ios::binary);
		entt::snapshot_loader loader{ impl.get_registry()};
		auto in_archive{ [&in](auto value) {
			in.read(reinterpret_cast<char*>(&value), sizeof(value));
			} };
		loader.template get<entt::entity>(in_archive);
		for (auto& ser : ReflectionRegistry::BinSerializerRegistryInstance()) {
			ser.m_Loader(loader, in);
		}
	}

	entity world::add_entity()
	{
		entt::registry& reg{ impl.get_registry() };
		entt::entity new_entity{ reg.create() };
		reg.emplace<entity::entity_name_t>(new_entity, default_name);
		return entity{ impl.handle, static_cast<std::uint32_t>(new_entity) };
	}

	void world::remove_entity(entity enty)
	{
		impl.get_registry().destroy(detail::entt_entity_cast(enty));
	}

	void world::update()
	{
		impl.get_scheduler().run(*this);
	}

	void shutdown_ecs()
	{
		worlds.reset();
		world_systems.reset();
	}

	void world::LoadYAML(std::string const& path) {
		YAML::Node root{YAML::LoadFile(path)};
		for (const auto& entity_node : root) {
			DeserializeEntity(impl.get_registry(), entity_node);
		}
	}

	void world::SaveYAML(std::string const& path) {
		YAML::Node root{  };
		entt::registry& reg{ impl.get_registry() };
		auto allentities = reg.view<entt::entity>();
		for (const auto& entity : allentities) {
			root = SerializeEntity<YAML::Node>(reg, entity);
		}
	}

	void world::UnloadNonGlobals() {
		
	}
	void world::UnloadAll() {
		impl.get_registry().clear();
	}

}