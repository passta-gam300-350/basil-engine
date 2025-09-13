#include <DirectXTex.h>
#include "texture/texture.h"
#include "compiler/compile.h"
#define RESOURCE_SUPPRESS_COMPILE_FAILURE

#ifdef RESOURCE_SUPPRESS_COMPILE_FAILURE
#include <iostream>
#include <locale>
#include <codecvt>
#endif

namespace Resource {
	void init() {
		CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	}

	void compile_texture(ResourceDescriptor const& rdesc) {
		DirectX::ScratchImage image;
		DirectX::ScratchImage cimage;
		DirectX::TexMetadata info;
		
		auto hres = LoadFromWICFile(rdesc.m_intermediate_files.c_str(), DirectX::WIC_FLAGS::WIC_FLAGS_NONE, &info, image);
#ifdef RESOURCE_SUPPRESS_COMPILE_FAILURE
		if (FAILED(hres)) {
			std::cout << "Warning [Texture]: Texture compilation failed! Unable to access file at \"" << std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(rdesc.m_intermediate_files) << "\"\n";
			return;
		}
#endif
		assert(!FAILED(hres) && "load fail");
		hres = Compress(image.GetImages(), image.GetImageCount(), info, DXGI_FORMAT_BC7_UNORM, DirectX::TEX_COMPRESS_PARALLEL | DirectX::TEX_COMPRESS_BC7_QUICK, DirectX::TEX_THRESHOLD_DEFAULT, cimage);
		assert(!FAILED(hres) && "compress fail");
		std::wstring outp{ Resource::get_output_dir_wstr() + rdesc.m_guid.to_hex_no_delimiter_wstr() + L".dds"};
		hres = DirectX::SaveToDDSFile(cimage.GetImages(), cimage.GetImageCount(), cimage.GetMetadata(), DirectX::DDS_FLAGS_NONE, outp.c_str());
		assert(!FAILED(hres) && "save fail");
	}
}