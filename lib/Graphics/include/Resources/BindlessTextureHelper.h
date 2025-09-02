#pragma once

#include "TextureBindingSystem.h"
#include <memory>
#include <iostream>

// Helper class to simplify bindless texture usage
class BindlessTextureHelper {
public:
    // Initialize bindless texture system
    static std::unique_ptr<ITextureBindingSystem> CreateOptimalBinding() {
        // Try bindless first, fallback to traditional
        auto bindingSystem = TextureBindingFactory::Create(TextureBindingFactory::BindingType::Bindless);
        
        // Log which system is being used
        auto* bindless = dynamic_cast<BindlessTextureBinding*>(bindingSystem.get());
        if (bindless != nullptr) {
            std::cout << "[BindlessTextureHelper] Using bindless texture system" << std::endl;
        } else {
            std::cout << "[BindlessTextureHelper] Using traditional texture binding (bindless not supported)" << std::endl;
        }
        
        return bindingSystem;
    }
    
    // Check if bindless textures are supported
    static bool IsBindlessAvailable() {
        return TextureBindingFactory::IsBindlessSupported();
    }
    
    // Create appropriate shader based on bindless support
    static std::string GetRecommendedVertexShader() {
        if (IsBindlessAvailable()) {
            return "assets/shaders/bindless.vert";
        }
        return "assets/shaders/basic.vert";
    }
    
    static std::string GetRecommendedFragmentShader() {
        if (IsBindlessAvailable()) {
            return "assets/shaders/bindless.frag";
        }
        return "assets/shaders/basic.frag";
    }
    
    // Print bindless texture capabilities
    static void PrintCapabilities() {
        std::cout << "=== Bindless Texture Capabilities ===" << std::endl;
        std::cout << "Bindless textures supported: " << (IsBindlessAvailable() ? "YES" : "NO") << std::endl;
        
        if (IsBindlessAvailable()) {
            // Query maximum bindless handles (if supported)
            GLint maxHandles = 0;
            glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxHandles);
            std::cout << "Max texture units: " << maxHandles << std::endl;
            
            // Note: GL_MAX_BINDLESS_TEXTURE_SIZE_ARB might not be available on all drivers
            std::cout << "Max bindless textures: Up to " << BindlessTextureBinding::MAX_BINDLESS_TEXTURES << " (implementation limit)" << std::endl;
        }
        
        std::cout << "Recommended vertex shader: " << GetRecommendedVertexShader() << std::endl;
        std::cout << "Recommended fragment shader: " << GetRecommendedFragmentShader() << std::endl;
        std::cout << "=====================================" << std::endl;
    }
};