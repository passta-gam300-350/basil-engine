#include "native/mesh.h"
#include "native/material.h"
#include "native/model.h"
#include "native/shader.h"
#include "native/texture.h"

namespace {
	//provides a std::istream like unsafe extraction, advances data param after extraction
	template <typename t>
	void MemWrite(char*& buff, t const& data) {
		memcpy(reinterpret_cast<void*>(buff), reinterpret_cast<const void*>(&data), sizeof(t));
		buff += sizeof(t);
	}
	//provides a std::istream like unsafe extraction, advances data param after extraction
	void MemWrite(char*& buff, const char* data, std::size_t count) {
		memcpy(reinterpret_cast<void*>(buff), reinterpret_cast<const void*>(data), count);
		buff += count;
	}
}

namespace Resource {
	MeshAssetData& MeshAssetData::operator>>(std::ofstream& outp) {
		*reinterpret_cast<const MeshAssetData*>(this) >> outp;
		return *this;
	}

	MeshAssetData const& MeshAssetData::operator>>(std::ofstream& outp) const {
		assert(outp && "file error!");

		outp.write(reinterpret_cast<const char*>(&MeshAssetData::MESH_MAGIC_VALUE), sizeof(std::uint64_t));

		std::uint64_t vsize{ vertices.size() };
		std::uint64_t isize{ indices.size() };
		std::uint64_t tsize{ textures.size() };
		std::uint64_t ttsize{ texture_type.size() };

		outp.write(reinterpret_cast<const char*>(&vsize), sizeof(std::uint64_t));
		outp.write(reinterpret_cast<const char*>(&isize), sizeof(std::uint64_t));
		outp.write(reinterpret_cast<const char*>(&tsize), sizeof(std::uint64_t));
		outp.write(reinterpret_cast<const char*>(&ttsize), sizeof(std::uint64_t));

		outp.write(reinterpret_cast<const char*>(vertices.data()), vertices.size() * sizeof(MeshAssetData::Vertex));
		outp.write(reinterpret_cast<const char*>(indices.data()), indices.size() * sizeof(unsigned int));
		outp.write(reinterpret_cast<const char*>(textures.data()), textures.size() * sizeof(Resource::Guid));
		outp.write(reinterpret_cast<const char*>(texture_type.data()), texture_type.size());

		return *this;
	}

	std::uint64_t MeshAssetData::DumpToMemory(char* buff, std::uint64_t size) const {
		std::uint64_t dumpsize{ sizeof(MeshAssetData::MESH_MAGIC_VALUE) + sizeof(std::uint64_t)*3 + vertices.size() * sizeof(MeshAssetData::Vertex) + indices.size() * sizeof(unsigned int) + textures.size() * sizeof(Resource::Guid) };
		std::uint64_t r_bytes{ size - dumpsize };
		assert(buff && "invalid buffer!");
		assert(size >= dumpsize && "buffer too small!");

		MemWrite(buff, &MeshAssetData::MESH_MAGIC_VALUE);

		MemWrite(buff, vertices.size());
		MemWrite(buff, indices.size());
		MemWrite(buff, textures.size());
		MemWrite(buff, texture_type.size());

		MemWrite(buff, reinterpret_cast<const char*>(vertices.data()), vertices.size() * sizeof(MeshAssetData::Vertex));
		MemWrite(buff, reinterpret_cast<const char*>(indices.data()), indices.size() * sizeof(unsigned int));
		MemWrite(buff, reinterpret_cast<const char*>(textures.data()), textures.size() * sizeof(Resource::Guid));
		MemWrite(buff, reinterpret_cast<const char*>(texture_type.data()), texture_type.size());

		return r_bytes;
	}

	MaterialAssetData& MaterialAssetData::operator>>(std::ostream& outp) {
		*reinterpret_cast<const MaterialAssetData*>(this) >> outp;
		return *this;
	}

	MaterialAssetData const& MaterialAssetData::operator>>(std::ostream& outp) const {
		assert(outp && "file error!");

		outp.write(reinterpret_cast<const char*>(&MaterialAssetData::MATERIAL_MAGIC_VALUE), sizeof(MaterialAssetData::MATERIAL_MAGIC_VALUE));
		outp.write(reinterpret_cast<const char*>(&shader_guid), sizeof(shader_guid));

		std::uint64_t strsize{ m_Name.size() };

		outp.write(reinterpret_cast<const char*>(&strsize), sizeof(strsize));
		outp.write(reinterpret_cast<const char*>(m_Name.data()), m_Name.size());

		outp.write(reinterpret_cast<const char*>(&m_AlbedoColor), sizeof(m_AlbedoColor));
		outp.write(reinterpret_cast<const char*>(&m_MetallicValue), sizeof(m_MetallicValue));
		outp.write(reinterpret_cast<const char*>(&m_RoughnessValue), sizeof(m_RoughnessValue));

		return *this;
	}

	std::uint64_t MaterialAssetData::DumpToMemory(char* buff, std::uint64_t size) const {
		std::uint64_t dumpsize{ sizeof(MaterialAssetData::MATERIAL_MAGIC_VALUE) + sizeof(std::uint64_t) + m_Name.size() + sizeof(m_AlbedoColor) + sizeof(m_MetallicValue) + sizeof(m_RoughnessValue) };
		std::uint64_t r_bytes{ size - dumpsize };
		assert(buff && "invalid buffer!");
		assert(size >= dumpsize && "buffer too small!");

		MemWrite(buff, &MaterialAssetData::MATERIAL_MAGIC_VALUE);
		MemWrite(buff, shader_guid);

		MemWrite(buff, m_Name.size());
		
		MemWrite(buff, reinterpret_cast<const char*>(m_Name.data()), m_Name.size());
		MemWrite(buff, reinterpret_cast<const char*>(&m_AlbedoColor), sizeof(m_AlbedoColor));
		MemWrite(buff, reinterpret_cast<const char*>(&m_MetallicValue), sizeof(m_MetallicValue));
		MemWrite(buff, reinterpret_cast<const char*>(&m_RoughnessValue), sizeof(m_RoughnessValue));

		return r_bytes;
	}

	ShaderAssetData& ShaderAssetData::operator>>(std::ostream& outp) {
		*reinterpret_cast<const ShaderAssetData*>(this) >> outp;
		return *this;
	}

	ShaderAssetData const& ShaderAssetData::operator>>(std::ostream& outp) const {
		assert(outp && "file error!");

		outp.write(reinterpret_cast<const char*>(&ShaderAssetData::SHADER_MAGIC_VALUE), sizeof(ShaderAssetData::SHADER_MAGIC_VALUE));
		
		std::uint64_t strsize{ m_Name.size() };
		std::uint64_t vstrsize{ m_VertPath.size() };
		std::uint64_t fstrsize{ m_FragPath.size() };

		outp.write(reinterpret_cast<const char*>(&strsize), sizeof(strsize));
		outp.write(reinterpret_cast<const char*>(&vstrsize), sizeof(vstrsize));
		outp.write(reinterpret_cast<const char*>(&fstrsize), sizeof(fstrsize));

		outp.write(reinterpret_cast<const char*>(m_Name.data()), strsize);
		outp.write(reinterpret_cast<const char*>(m_VertPath.data()), vstrsize);
		outp.write(reinterpret_cast<const char*>(m_FragPath.data()), fstrsize);

		return *this;
	}

	std::uint64_t ShaderAssetData::DumpToMemory(char* buff, std::uint64_t size) const {
		std::uint64_t dumpsize{ sizeof(ShaderAssetData::SHADER_MAGIC_VALUE) + sizeof(std::uint64_t)*3 + m_Name.size() + m_VertPath.size() + m_FragPath.size() };
		std::uint64_t r_bytes{ size - dumpsize };
		assert(buff && "invalid buffer!");
		assert(size >= dumpsize && "buffer too small!");

		MemWrite(buff, ShaderAssetData::SHADER_MAGIC_VALUE);
		
		MemWrite(buff, m_Name.size());
		MemWrite(buff, m_VertPath.size());
		MemWrite(buff, m_FragPath.size());

		MemWrite(buff, reinterpret_cast<const char*>(m_Name.data()), m_Name.size());
		MemWrite(buff, reinterpret_cast<const char*>(m_VertPath.data()), sizeof(m_VertPath.size()));
		MemWrite(buff, reinterpret_cast<const char*>(m_FragPath.data()), sizeof(m_FragPath.size()));

		return r_bytes;
	}

	ModelResource const& ModelResource::operator>>(std::ofstream& outp) const {
		std::uint32_t mesh_ct{ static_cast<std::uint32_t>(m_meshes.size()) };
		std::uint32_t material_ct{ static_cast<std::uint32_t>(m_mats.size()) };

		outp.write(reinterpret_cast<const char*>(&m_guid), sizeof(m_guid));
		outp.write(reinterpret_cast<const char*>(&mesh_ct), sizeof(mesh_ct));
		outp.write(reinterpret_cast<const char*>(&material_ct), sizeof(material_ct));

		for (auto const& mesres : m_meshes) {
			outp.write(reinterpret_cast<const char*>(&mesres), sizeof(mesres));
		}

		for (auto const& matres : m_mats) {
			outp.write(reinterpret_cast<const char*>(&matres), sizeof(matres));
		}
		return *this;
	}

	ModelResource& ModelResource::operator>>(std::ofstream& outp) {
		std::uint32_t mesh_ct{ static_cast<std::uint32_t>(m_meshes.size()) };
		std::uint32_t material_ct{ static_cast<std::uint32_t>(m_mats.size()) };

		outp.write(reinterpret_cast<const char*>(&m_guid), sizeof(m_guid));
		outp.write(reinterpret_cast<const char*>(&mesh_ct), sizeof(mesh_ct));
		outp.write(reinterpret_cast<const char*>(&material_ct), sizeof(material_ct));

		for (auto const& mesres : m_meshes) {
			outp.write(reinterpret_cast<const char*>(&mesres), sizeof(mesres));
		}

		for (auto const& matres : m_mats) {
			outp.write(reinterpret_cast<const char*>(&matres), sizeof(matres));
		}
		return *this;
	}
	TextureAsset& TextureAsset::operator>>(std::ofstream& outp) {
		*reinterpret_cast<const TextureAsset*>(this) >> outp;
		return *this;
	}
	TextureAsset const& TextureAsset::operator>>(std::ofstream& outp) const {
		outp.write(reinterpret_cast<const char*>(&TextureAsset::TEXTURE_MAGIC_VALUE), sizeof(TextureAsset::TEXTURE_MAGIC_VALUE));
		std::size_t sz{ m_TexData.GetBufferSize() };
		outp.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
		outp.write(reinterpret_cast<const char*>(m_TexData.GetConstBufferPointer()), m_TexData.GetBufferSize());
		return *this;
	}
}