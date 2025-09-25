#define TINYDDSLOADER_IMPLEMENTATION

#include "native/loader.h"
#include <locale>
#include <codecvt>
#include <tinyddsloader.h>

namespace {
	//provides a std::istream like extraction, advances data param after extraction
	template <typename t>
	t MemRead(const char*& data) {
		static_assert(std::is_default_constructible_v<t> || std::is_copy_constructible_v<t> && "type must be default / copy constructible");
		if constexpr (std::is_copy_constructible_v<t>) {
			const t* ptr{reinterpret_cast<t const*>(data)};
			data += sizeof(t);
			return *ptr;
		}
		else {
			t val{};
			memcpy(reinterpret_cast<void*>(&val), reinterpret_cast<const void*>(data), sizeof(t));
			data += sizeof(t);
			return val;
		}
	}
	//provides a std::istream like extraction, advances data param after extraction
	void MemRead(const char*& data, char* out, std::size_t count) {
		memcpy(reinterpret_cast<void*>(out), reinterpret_cast<const void*>(data), count);
		data += count;
	}
}

namespace Resource {
	MeshAssetData load_native_mesh_from_memory(const char* data) {
		assert(data && "memory supplied is nullptr!");
		const char* read_ptr{data};
		assert(MemRead<std::uint64_t>(read_ptr) == MeshAssetData::MESH_MAGIC_VALUE && "wrong file signature!");

		MeshAssetData tmp{};

		std::uint64_t vertex_ct{ MemRead<std::uint64_t>(read_ptr) };
		std::uint64_t idx_ct{ MemRead<std::uint64_t>(read_ptr) };
		std::uint64_t texture_ct{ MemRead<std::uint64_t>(read_ptr) };

		tmp.vertices.resize(vertex_ct);
		tmp.indices.resize(idx_ct);
		tmp.textures.resize(texture_ct);

		MemRead(read_ptr, reinterpret_cast<char*>(tmp.vertices.data()), sizeof(MeshAssetData::Vertex) * vertex_ct);
		MemRead(read_ptr, reinterpret_cast<char*>(tmp.indices.data()), sizeof(unsigned int) * idx_ct);
		MemRead(read_ptr, reinterpret_cast<char*>(tmp.textures.data()), sizeof(Resource::Guid) * texture_ct);

		return tmp;
	}

	MeshAssetData load_native_mesh_from_file(std::string const& file_path) {
		std::ifstream ifs{ file_path, std::ios::binary };
		assert(ifs && "file error!");
		std::uint64_t file_magic{};
		ifs.read(reinterpret_cast<char*>(&file_magic), sizeof(file_magic));
		assert(file_magic == MeshAssetData::MESH_MAGIC_VALUE && "wrong file signature!");

		MeshAssetData tmp{};

		std::uint64_t vertex_ct{};
		std::uint64_t idx_ct{};
		std::uint64_t texture_ct{};

		ifs.read(reinterpret_cast<char*>(&vertex_ct), sizeof(vertex_ct));
		ifs.read(reinterpret_cast<char*>(&idx_ct), sizeof(idx_ct));
		ifs.read(reinterpret_cast<char*>(&texture_ct), sizeof(texture_ct));

		tmp.vertices.resize(vertex_ct);
		tmp.indices.resize(idx_ct);
		tmp.textures.resize(texture_ct);

		ifs.read(reinterpret_cast<char*>(tmp.vertices.data()), sizeof(MeshAssetData::Vertex) * vertex_ct);
		ifs.read(reinterpret_cast<char*>(tmp.indices.data()), sizeof(unsigned int) * idx_ct);
		ifs.read(reinterpret_cast<char*>(tmp.textures.data()), sizeof(Resource::Guid) * texture_ct);

		return tmp;
	}

	MaterialAssetData load_native_material_from_memory(const char* data) {
		assert(data && "memory supplied is nullptr!");
		const char* read_ptr{ data };
		assert(MemRead<std::uint64_t>(read_ptr) == MaterialAssetData::MATERIAL_MAGIC_VALUE && "wrong file signature!");

		MaterialAssetData tmp{};

		tmp.shader_guid = MemRead<Resource::Guid>(read_ptr);

		std::uint64_t strsize{ MemRead<std::uint64_t>(read_ptr) };

		tmp.m_Name.resize(strsize);

		MemRead(read_ptr, reinterpret_cast<char*>(tmp.m_Name.data()), strsize);
		MemRead(read_ptr, reinterpret_cast<char*>(&tmp.m_AlbedoColor), sizeof(tmp.m_AlbedoColor));
		MemRead(read_ptr, reinterpret_cast<char*>(&tmp.m_MetallicValue), sizeof(tmp.m_MetallicValue));
		MemRead(read_ptr, reinterpret_cast<char*>(&tmp.m_RoughnessValue), sizeof(tmp.m_RoughnessValue));

		return tmp;
	}

	MaterialAssetData load_native_material_from_file(std::string const& file_path) {
		std::ifstream ifs{ file_path, std::ios::binary };
		assert(ifs && "file error!");
		std::uint64_t file_magic{};
		ifs.read(reinterpret_cast<char*>(&file_magic), sizeof(file_magic));
		assert(file_magic == MaterialAssetData::MATERIAL_MAGIC_VALUE && "wrong file signature!");

		MaterialAssetData tmp{};

		std::uint64_t strsize{};
		
		ifs.read(reinterpret_cast<char*>(&tmp.shader_guid), sizeof(Resource::Guid));
		ifs.read(reinterpret_cast<char*>(&strsize), sizeof(strsize));

		tmp.m_Name.resize(strsize);

		ifs.read(reinterpret_cast<char*>(&tmp.m_Name), sizeof(strsize));

		ifs.read(reinterpret_cast<char*>(&tmp.m_AlbedoColor), sizeof(tmp.m_AlbedoColor));
		ifs.read(reinterpret_cast<char*>(&tmp.m_MetallicValue), sizeof(tmp.m_MetallicValue));
		ifs.read(reinterpret_cast<char*>(&tmp.m_RoughnessValue), sizeof(tmp.m_RoughnessValue));

		return tmp;
	}

	ShaderAssetData load_native_shader_from_memory(const char* data) {
		assert(data && "memory supplied is nullptr!");
		const char* read_ptr{ data };
		assert(MemRead<std::uint64_t>(read_ptr) == ShaderAssetData::SHADER_MAGIC_VALUE && "wrong file signature!");

		ShaderAssetData tmp{};

		std::uint64_t strsize{ MemRead<std::uint64_t>(read_ptr) };
		std::uint64_t fstrsize{ MemRead<std::uint64_t>(read_ptr) };
		std::uint64_t vstrsize{ MemRead<std::uint64_t>(read_ptr) };


		tmp.m_Name.resize(strsize);
		tmp.m_VertPath.resize(vstrsize);
		tmp.m_FragPath.resize(fstrsize);

		MemRead(read_ptr, reinterpret_cast<char*>(tmp.m_Name.data()), strsize);
		MemRead(read_ptr, reinterpret_cast<char*>(tmp.m_VertPath.data()), vstrsize);
		MemRead(read_ptr, reinterpret_cast<char*>(tmp.m_FragPath.data()), fstrsize);

		return tmp;
	}

	ShaderAssetData load_native_shader_from_file(std::string const& file_path) {
		std::ifstream ifs{ file_path, std::ios::binary };
		assert(ifs && "file error!");
		std::uint64_t file_magic{};
		ifs.read(reinterpret_cast<char*>(&file_magic), sizeof(file_magic));
		assert(file_magic == ShaderAssetData::SHADER_MAGIC_VALUE && "wrong file signature!");

		ShaderAssetData tmp{};

		std::uint64_t strsize{};
		std::uint64_t fstrsize{};
		std::uint64_t vstrsize{};

		ifs.read(reinterpret_cast<char*>(&strsize), sizeof(strsize));
		ifs.read(reinterpret_cast<char*>(&vstrsize), sizeof(vstrsize));
		ifs.read(reinterpret_cast<char*>(&fstrsize), sizeof(fstrsize));

		tmp.m_Name.resize(strsize);
		tmp.m_VertPath.resize(vstrsize);
		tmp.m_FragPath.resize(fstrsize);

		ifs.read(reinterpret_cast<char*>(tmp.m_Name.data()), sizeof(strsize));
		ifs.read(reinterpret_cast<char*>(tmp.m_VertPath.data()), sizeof(vstrsize));
		ifs.read(reinterpret_cast<char*>(tmp.m_FragPath.data()), sizeof(fstrsize));

		return tmp;
	}
	/*
	TextureAssetData load_dds_texture(std::string const& file_name) {
		tinyddsloader::DDSFile ddsfile;
		ddsfile.Load(file_name.c_str());
		assert(tinyddsloader::Result::Success && "fail to load dds for " && file_name.c_str());
		return ddsfile;
	}*/

	TextureAssetData load_dds_texture_from_memory(const char* data) {
		DirectX::ScratchImage ddsfile;
		DirectX::TexMetadata texmeta;
		auto readptr{ data };
		auto size = MemRead<std::uint64_t>(readptr);
		DirectX::LoadFromDDSMemory(reinterpret_cast<const std::byte*>(readptr), size, DirectX::DDS_FLAGS_NONE, &texmeta, ddsfile);
		//assert(tinyddsloader::Result::Success && "fail to load dds from memory from " && data);
		return ddsfile;
	}
}