#ifndef RESOURCE_IMPORTER_TEXTURE
#define RESOURCE_IMPORTER_TEXTURE

#include <DirectXTex.h>
#include <native/texture.h>
#include "descriptors/texture.hpp"

inline TextureResourceData ImportTexture(TextureDescriptor const& texDesc) {
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	DirectX::ScratchImage image;
	DirectX::ScratchImage flippedImage;
	DirectX::ScratchImage cimage;
	DirectX::TexMetadata info;

	auto hres = LoadFromWICFile(string_to_wstring(rp::utility::resolve_path(texDesc.base.m_source)).c_str(), DirectX::WIC_FLAGS::WIC_FLAGS_NONE, &info, image);
	assert(!FAILED(hres) && "load fail");

	// Flip vertically to convert from DirectX (top-down) to OpenGL (bottom-up) orientation
	hres = DirectX::FlipRotate(image.GetImages(), image.GetImageCount(), info, DirectX::TEX_FR_FLIP_VERTICAL, flippedImage);
	assert(!FAILED(hres) && "flip fail");

	hres = Compress(flippedImage.GetImages(), flippedImage.GetImageCount(), flippedImage.GetMetadata(), static_cast<DXGI_FORMAT>(texDesc.compression), DirectX::TEX_COMPRESS_PARALLEL, DirectX::TEX_THRESHOLD_DEFAULT, cimage);
	assert(!FAILED(hres) && "compress fail");
	DirectX::Blob dxblob;
	hres = DirectX::SaveToDDSMemory(cimage.GetImages(), cimage.GetImageCount(), cimage.GetMetadata(), DirectX::DDS_FLAGS_NONE, dxblob);
	assert(!FAILED(hres) && "save fail");
	TextureResourceData texdata;
	texdata.m_TexData.AllocateExact(dxblob.GetBufferSize());
	std::memcpy(texdata.m_TexData.Raw(), dxblob.GetConstBufferPointer(), dxblob.GetBufferSize());
	return texdata;
}

RegisterResourceTypeImporter(TextureDescriptor, TextureResourceData, "texture", ".texture", ImportTexture, ".png", ".jpeg", ".jpg")

#endif