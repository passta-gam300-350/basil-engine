#include <crtdbg.h> // For memory leak detection
#include <importer/importer.hpp>

int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetBreakAlloc(150151);
	/*ModelDescriptor mdesc;
	mdesc.mesh_asset_source = "assets/models/tinbox/tin_box.obj";
	mdesc.base.m_guid = rp::Guid::generate();
	rp::ResourceTypeImporterRegistry::Serialize(rp::utility::type_hash<ModelDescriptor>::value(), "yaml", "modeltest.desc", reinterpret_cast<std::byte*>(&mdesc));
	rp::ResourceTypeImporterRegistry::Import(rp::utility::type_hash<ModelDescriptor>::value(), "modeltest.desc");
	auto mesh = rp::serialization::binary_serializer::deserialize<MeshResourceData>(mdesc.base.m_guid.to_hex()+".mesh");*/

	/*CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	DirectX::ScratchImage image;
	DirectX::ScratchImage cimage;
	DirectX::TexMetadata info;

	auto hres = LoadFromWICFile(string_to_wstring("assets/models/tinbox/tin_can_lambert1_BaseColor.png").c_str(), DirectX::WIC_FLAGS::WIC_FLAGS_NONE, &info, image);
	assert(!FAILED(hres) && "load fail");
	hres = Compress(image.GetImages(), image.GetImageCount(), info, static_cast<DXGI_FORMAT>(DXGI_FORMAT_BC3_UNORM), DirectX::TEX_COMPRESS_PARALLEL, DirectX::TEX_THRESHOLD_DEFAULT, cimage);
	assert(!FAILED(hres) && "compress fail");
	hres = DirectX::SaveToDDSFile(cimage.GetImages(), cimage.GetImageCount(), cimage.GetMetadata(), DirectX::DDS_FLAGS_NONE, L"test.dds");
	assert(!FAILED(hres) && "save fail");*/
	/*std::string rdesc = "assets/Ghost_Model1.fbx.desc";

	auto importertype{ rp::ResourceTypeImporterRegistry::GetDescriptorImporterType(rdesc) };
	auto biguid{ rp::ResourceTypeImporterRegistry::GetDescriptorGuid(rdesc) };
	auto file_path{ "imports/" + biguid.m_guid.to_hex() + rp::ResourceTypeImporterRegistry::GetImporterSuffix(importertype)};
	rp::ResourceTypeImporterRegistry::Import(importertype, rdesc, file_path);
	rp::ResourceTypeImporterRegistry::GetDescriptorGuid(rdesc);*/

	//this code block demonstrates the memleak in assimp's fbx importer
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(
			"assets/Ghost_Model1.fbx",
			aiProcess_Triangulate |
			aiProcess_GenSmoothNormals |
			aiProcess_CalcTangentSpace |
			aiProcess_JoinIdenticalVertices
		);
		importer.FreeScene();
	}
	
	return 0;
}