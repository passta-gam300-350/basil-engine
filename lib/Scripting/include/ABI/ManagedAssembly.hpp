/******************************************************************************/
/*!
\file   ManagedAssembly.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Managed assembly wrapper

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef MANAGEDASSEMBLY_HPP
#define MANAGEDASSEMBLY_HPP

#include <string_view>

#include <mono/metadata/appdomain.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/image.h>
#include <memory>

// Forward declarations for Mono types
struct CSAssembly;

struct ManagedAssembly
{
    std::unique_ptr<CSAssembly> assembly;
    MonoAssembly* assemblyHandle;
    bool isLoaded;

    explicit ManagedAssembly(std::unique_ptr<CSAssembly> assembly);
    bool Load(MonoDomain* domain);
    void Unload() noexcept;

    std::string_view Name() const noexcept;
    std::string_view Path() const noexcept;
    std::size_t Size() const noexcept;

    MonoImage* Image() const noexcept;
    bool IsLoaded() const noexcept;

	~ManagedAssembly();
};

#endif // MANAGEDASSEMBLY_HPP
