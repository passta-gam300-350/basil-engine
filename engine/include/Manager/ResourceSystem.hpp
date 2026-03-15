#ifndef ENGINE_RESOURCE_SYSTEM_HPP
#define ENGINE_RESOURCE_SYSTEM_HPP

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <memory>
#include <jobsystem.hpp>
#include <hashtable.hpp>

#define RP_REFLECTION_IGNORE_ENUM_DEDUCTION_RESTRICTION
#include <rsc-core/rp.hpp>
#include <cstdint>
#include <cstddef>
#include <unordered_map>
#include <vector>
#include <type_traits>
#include <utility>
#include <string_view>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>


// Renamed from Handle to ResourceHandle to avoid conflict with Windows HANDLE
struct ResourceHandle {
    std::uint32_t m_Index{};
    std::uint32_t m_Generation{};
    explicit operator bool() const noexcept { return m_Generation != 0; }
    operator std::uint64_t() const noexcept { return std::uint64_t(m_Index) << 32 | m_Generation; }
    bool operator==(ResourceHandle const& other) {
        return other.m_Index == m_Index && other.m_Generation == m_Generation;
    }
    bool operator!=(ResourceHandle const& other) {
        return !(*this==other);
    }
};

template <>
struct std::hash<ResourceHandle> {
    auto operator()(ResourceHandle hdl) const {
        return std::hash<std::uint64_t>()(std::uint64_t(hdl));
    }
};

using ResourceTypeId_t = std::uintptr_t;

template <typename T>
struct ResourceTypeInitiation {
    static constexpr std::int32_t m_TypeInstance{};
};

template <typename T>
ResourceTypeId_t ResourceTypeId_v{ std::uintptr_t(&ResourceTypeInitiation<T>::m_TypeInstance) };

template <typename T>
ResourceTypeId_t ResourceType_Id() noexcept {
    return reinterpret_cast<ResourceTypeId_t>(&ResourceTypeInitiation<T>::m_TypeInstance);
}

#pragma warning(disable:4324) //disables alignas padding warning
template <typename T>
struct ResourceSlot {
    rp::Guid m_Guid{ rp::null_guid };
    std::uint32_t m_Generation{};
    bool m_Ready{};
    bool m_Alive{};
    alignas(T) unsigned char storage[sizeof(T)];
    T& value() noexcept { return *std::launder(reinterpret_cast<T*>(storage)); }
    const T& value() const noexcept { return *std::launder(reinterpret_cast<const T*>(storage)); }
};

template <typename T, stl_allocator_t<T> A = std::allocator<T>>
class ResourcePool {
public:
    using LoaderFn = std::function<T(const char*)>; //effectively the constructor of memory pool. throwable
    using UnloaderFn = std::function<void(T&)>; //destructs only, deallocation happens lazily. throwable
    using ValueType = T;
    using Pointer = T*;
    using ConstPointer = const T*;

    explicit ResourcePool(LoaderFn loadfn, UnloaderFn unloadfn)
        : m_Loader(loadfn), m_Unloader(unloadfn) {
    }

    ~ResourcePool() {
        Clear();
    }

    //get async, check handle, functionally equivalent of std::future but unlimited gets
    ResourceHandle GetHandle(rp::Guid id);

    Pointer Ptr(ResourceHandle h) noexcept {
        if (h.m_Index >= m_Slots.size()) return nullptr;
        auto& s = m_Slots[h.m_Index];
        if (!s.m_Alive || s.m_Generation != h.m_Generation) return nullptr;
        return &s.value();
    }

    ConstPointer Ptr(ResourceHandle h) const noexcept {
        if (h.m_Index >= m_Slots.size()) return nullptr;
        auto& s = m_Slots[h.m_Index];
        if (!s.m_Alive || s.m_Generation != h.m_Generation) return nullptr;
        return &s.value();
    }

    ResourceHandle Find(rp::Guid guid) const noexcept {
        auto it = m_GuidSlots.find(guid);
        if (it == m_GuidSlots.end()) return {};
        return MakeHandle(it->second);
    }

    rp::Guid GetGuid(ResourceHandle h) const noexcept {
        if (h.m_Index >= m_Slots.size()) return rp::null_guid;
        auto& s = m_Slots[h.m_Index];
        if (!s.m_Alive || s.m_Generation != h.m_Generation) return rp::null_guid;
        return s.m_Guid;
    }

    bool Unload(rp::Guid id) {
        auto it = m_GuidSlots.find(id);
        if (it == m_GuidSlots.end()) return false;
        const auto idx = it->second;
        DestroySlot(idx);
        m_GuidSlots.erase(it);
        FreeSlot(idx);
        return true;
    }

    /**
     * @brief Insert a pre-loaded resource directly into the pool (for in-memory editor resources)
     * @param guid GUID to associate with the resource
     * @param resource Already-constructed resource to insert
     * @return true if inserted, false if GUID already exists
     */
    __declspec(noinline) bool InsertPreloaded(rp::Guid guid, T resource) {
        // Check if GUID already exists
        auto it = m_GuidSlots.find(guid);
        if (it != m_GuidSlots.end()) {
            return false;  // Already exists
        }

        // Allocate slot and insert resource
        std::uint32_t idx = AllocateSlot();
        auto& slot = m_Slots[idx];
        slot.m_Guid = guid;
        slot.m_Alive = true;
        slot.m_Ready = true;  // Immediately ready (no async loading)

        // Move-construct or copy-construct into slot storage
        if constexpr (std::is_move_constructible_v<T>) {
            new (slot.storage) T(std::move(resource));
        } else {
            new (slot.storage) T(resource);
        }

        m_GuidSlots.emplace(guid, idx);
        return true;
    }

    void Clear() {
        for (std::uint32_t i = 0; i < m_Slots.size(); ++i) {
            if (m_Slots[i].m_Alive) {
                DestroySlot(i);
                FreeSlotNoBump(i);
            }
        }
        m_GuidSlots.clear();
        m_FreeList.clear();
        m_Slots.clear();
    }

private:
    ResourceHandle MakeHandle(std::uint32_t idx) const noexcept {
        const auto& s = m_Slots[idx];
        return ResourceHandle{ idx, s.m_Generation };
    }

    std::uint32_t AllocateSlot() {
        if (!m_FreeList.empty()) {
            auto idx = m_FreeList.back();
            m_FreeList.pop_back();
            // generation already bumped on free; reuse
            return idx;
        }
        m_Slots.emplace_back();
        return static_cast<std::uint32_t>(m_Slots.size() - 1);
    }

    void DestroySlot(std::uint32_t idx) noexcept {
        auto& s = m_Slots[idx];
        if (!s.m_Alive) return;
        // Call unloader before dtor for external resources
        m_Unloader(s.value());
        s.value().~T();
        s.m_Alive = false;
    }

    void FreeSlot(std::uint32_t idx) noexcept {
        auto& s = m_Slots[idx];
        ++s.m_Generation; // invalidate old handles
        m_FreeList.push_back(idx);
    }

    void FreeSlotNoBump(std::uint32_t idx) noexcept {
        // used by clear() after destroy; still safe to bump for consistency
        ++m_Slots[idx].m_Generation;
        m_FreeList.push_back(idx);
    }

    LoaderFn m_Loader;
    UnloaderFn m_Unloader;
    std::vector<ResourceSlot<T>> m_Slots;
    std::vector<std::uint32_t> m_FreeList;
    std::unordered_map<rp::Guid, std::uint32_t> m_GuidSlots;
};

struct ResourceRegistry {
    //type-erased vtable mapping for resource type
    struct VTable {
        std::function<ResourceHandle(void*, rp::Guid)> m_Get;
        std::function<void*()> m_GetPool;
        std::function<ResourceHandle(void*, rp::Guid)> m_Find;
        std::function<rp::Guid(void*, ResourceHandle)> m_GetGuid;
        std::function<void* (void*, ResourceHandle)> m_Ptr;
        std::function<const void* (const void*, ResourceHandle)> m_ConstPtr;
        std::function<bool (void*, rp::Guid)> m_Unload;
        std::function<void(void*)> m_Clear;
    };

    using TypeNameHash = std::uint64_t;

    struct Entry {
        void* m_Pool; //type erasure for ResourcePool<T>*
        VTable m_Vt;
        std::string_view m_TypeName;
    };

    static ResourceRegistry& Instance() {
        static ResourceRegistry inst;
        return inst;
    }

    template <typename T>
    using PoolType = ResourcePool<T>;

    template <typename T>
    void RegisterType(typename PoolType<T>::LoaderFn loader, typename PoolType<T>::UnloaderFn unloader, std::string_view type_name = {})
    {
        const auto id = ResourceTypeId_v<T>;
        if (m_Entries.find(id) != m_Entries.end()) 
            return;

        // Create per-type singleton pool lazily
        static PoolType<T> pool(loader, unloader);

        Entry e;
        e.m_Pool = &pool;
        e.m_TypeName = type_name.empty() ? std::string_view(typeid(T).name()) : type_name;

        e.m_Vt = {
            //get
            [](void* p, rp::Guid g) -> ResourceHandle {
                return static_cast<PoolType<T>*>(p)->GetHandle(g);
            },
            //get_pool
            []() -> void* { return static_cast<void*>(&pool); },
            //find
            [](void* p, rp::Guid g) -> ResourceHandle {
                return static_cast<PoolType<T>*>(p)->Find(g);
            },
            //guid
            [](void* p, ResourceHandle h) -> rp::Guid {
                return static_cast<PoolType<T>*>(p)->GetGuid(h);
            },
            //ptr
            [](void* p, ResourceHandle h) -> void* {
                return static_cast<void*>(static_cast<PoolType<T>*>(p)->Ptr(h));
            },
            //const ptr
            [](const void* p, ResourceHandle h) -> const void* {
                return static_cast<const void*>(static_cast<const PoolType<T>*>(p)->Ptr(h));
            },
            //unload
            [](void* p, rp::Guid g) -> bool {
                return static_cast<PoolType<T>*>(p)->Unload(g);
            },
            //clear all
            [](void* p) -> void {
                static_cast<PoolType<T>*>(p)->Clear();
            }
        };

        m_Entries.emplace(id, e);
        m_TypeNamePoolId.emplace(rp::utility::compute_string_hash(type_name), id);
    }

    template <typename T>
    PoolType<T>* Pool() {
        auto it = m_Entries.find(ResourceType_Id<T>());
        return it == m_Entries.end() ? nullptr : static_cast<PoolType<T>*>(it->second.m_Pool);
    }

    Entry* Pool(rp::BasicIndexedGuid biguid) {
        auto typeit = m_TypeNamePoolId.find(biguid.m_typeindex);
        assert(typeit != m_TypeNamePoolId.end() && "typehash not found. probably different from registered");
        auto it = m_Entries.find(typeit->second);
        return it == m_Entries.end() ? nullptr : &it->second;
    }

    template <typename T>
    T* Get(rp::Guid guid, ResourceHandle* out_handle = nullptr) {
        if (!guid) return nullptr;
        auto* pool = Pool<T>();
        if (!pool) return nullptr;
        ResourceHandle h = pool->GetHandle(guid);
        if (out_handle) *out_handle = h;
        return pool->Ptr(h);
    }

    template <typename T>
    T* Get(rp::TypedGuid<T> guid) {
        return Get<T>(guid.m_guid);
    }

    template <rp::utility::static_string ss>
    typename rp::TypeNameGuid<ss>::type* Get(rp::TypeNameGuid<ss> guid) {
        return Get<rp::TypeNameGuid<ss>::type>(guid.m_guid);
    }

    template <typename T>
    ResourceHandle Find(rp::Guid id) {
        auto* pool = Pool<T>();
        return pool ? pool->Find(id) : ResourceHandle{};
    }

    template <typename T>
    rp::Guid GetGuid(ResourceHandle h) {
        auto* pool = Pool<T>();
        return pool ? pool->GetGuid(h) : rp::null_guid;
    }

    template <typename T>
    bool Unload(rp::Guid guid) {
        auto* pool = Pool<T>();
        return pool ? pool->Unload(guid) : false;
    }

    bool Unload(rp::BasicIndexedGuid biguid) {
        Entry* pool = Pool(biguid);
        return pool ? pool->m_Vt.m_Unload(pool->m_Pool, biguid.m_guid) : false;
    }

    bool Exists(rp::BasicIndexedGuid biguid) const;

    /**
     * @brief Register an in-memory resource (for editor-created assets without files)
     *
     * This allows registering resources that don't have backing files, such as
     * materials/meshes created by the editor at runtime. These resources can later
     * transition to file-based when saved.
     *
     * @tparam T Resource type (e.g., std::shared_ptr<Material>)
     * @param guid GUID to associate with the resource
     * @param resource The resource instance to register
     * @return true if registered successfully, false if GUID already exists
     *
     * @note The resource type must be registered via REGISTER_RESOURCE_TYPE before use
     * @note Editor is responsible for unregistering resources when deleted
     */
    template <typename T>
    bool RegisterInMemory(rp::Guid guid, T resource) {

    	auto* pool = Pool<T>();
        if (!pool) {
            spdlog::error("ResourceRegistry: Cannot register in-memory resource - type not registered");
            return false;
        }

        bool success = pool->InsertPreloaded(guid, std::move(resource));
        if (!success) {
            spdlog::warn("ResourceRegistry: GUID {} already exists, skipping registration", guid.to_hex());
        }
        return success;
    }

    /**
     * @brief Unregister an in-memory resource
     *
     * Removes a resource previously registered via RegisterInMemory(). This should
     * be called when deleting unsaved editor assets.
     *
     * @tparam T Resource type
     * @param guid GUID of the resource to unregister
     * @return true if unregistered, false if not found
     */
    template <typename T>
    bool UnregisterInMemory(rp::Guid guid) {
        return Unload<T>(guid);  // Unload handles cleanup
    }

    void ReleaseAll() {
        for (auto entry : m_Entries) {
            entry.second.m_Vt.m_Clear(entry.second.m_Pool);
        }
    }

private:
    std::unordered_map<ResourceTypeId_t, Entry> m_Entries;
    std::unordered_map<TypeNameHash, ResourceTypeId_t> m_TypeNamePoolId;
};

class MemoryMappedFile {
public:
    MemoryMappedFile(const std::wstring& path)
        : m_HFile(INVALID_HANDLE_VALUE), m_HMap(nullptr), m_Data(nullptr), m_Size(0)
    {
        // 1. Open the file
        m_HFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (m_HFile == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to open file");
        }

        // 2. Get file size
        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(m_HFile, &fileSize)) {
            CloseHandle(m_HFile);
            throw std::runtime_error("Failed to get file size");
        }
        m_Size = static_cast<std::size_t>(fileSize.QuadPart);

        // 3. Create file mapping
        m_HMap = CreateFileMappingW(m_HFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!m_HMap) {
            CloseHandle(m_HFile);
            throw std::runtime_error("Failed to create file mapping");
        }

        // 4. Map view of file
        m_Data = static_cast<const std::byte*>(MapViewOfFile(m_HMap, FILE_MAP_READ, 0, 0, 0));
        if (!m_Data) {
            CloseHandle(m_HMap);
            CloseHandle(m_HFile);
            throw std::runtime_error("Failed to map view of file");
        }
    }
    MemoryMappedFile() = default;
    ~MemoryMappedFile() {
        if (m_Data) UnmapViewOfFile(m_Data);
        if (m_HMap) CloseHandle(m_HMap);
        if (m_HFile != INVALID_HANDLE_VALUE) CloseHandle(m_HFile);
    }

    // Prevent copying
    MemoryMappedFile(const MemoryMappedFile&) = delete;
    MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;

    // Allow moving
    MemoryMappedFile(MemoryMappedFile&& other) noexcept
        : m_HFile(other.m_HFile), m_HMap(other.m_HMap), m_Data(other.m_Data), m_Size(other.m_Size)
    {
        other.m_HFile = INVALID_HANDLE_VALUE;
        other.m_HMap = nullptr;
        other.m_Data = nullptr;
        other.m_Size = 0;
    }

    MemoryMappedFile& operator=(MemoryMappedFile&& other) noexcept {
        if (this != &other) {
            this->~MemoryMappedFile();
            m_HFile = other.m_HFile;
            m_HMap = other.m_HMap;
            m_Data = other.m_Data;
            m_Size = other.m_Size;
            other.m_HFile = INVALID_HANDLE_VALUE;
            other.m_HMap = nullptr;
            other.m_Data = nullptr;
            other.m_Size = 0;
        }
        return *this;
    }

    const std::byte* data() const { return m_Data; }
    std::size_t size() const { return m_Size; }

    // Safe accessor
    const std::byte* getRange(std::size_t offset, std::size_t length) const {
        if (offset + length > m_Size) {
            throw std::out_of_range("Requested range is out of bounds");
        }
        return m_Data + offset;
    }

private:
    HANDLE m_HFile;
    HANDLE m_HMap;
    const std::byte* m_Data;
    std::size_t m_Size;
};


struct ResourceSystem {
    struct FileEntry {
        rp::Guid m_Guid;
        std::string m_Path;
        std::uint64_t m_Offset;
        std::uint64_t m_Size;
    };

    static std::unique_ptr<ResourceSystem>& InstancePtr() {
        static std::unique_ptr<ResourceSystem> inst{std::make_unique<ResourceSystem>()};
        return inst;
    }

    static void Release() {
        ResourceRegistry::Instance().ReleaseAll();
        InstancePtr().reset();
    }

    static ResourceSystem& Instance() {
        return *InstancePtr();
    }

    static ResourceSystem& SetResourceThreads(std::int32_t thread_count) {
        ResourceSystem& inst{ Instance() };
        if (inst.m_JobSystem.get_thread_ct() == thread_count) {
            return inst;
        }
        inst.m_JobSystem.~JobSystem();
        new (&inst.m_JobSystem) JobSystem{ thread_count };
        return inst;
    }

    void ImportAssetList(std::string const& assetfilename, std::string const& imported_path) {
        if (!std::filesystem::exists(assetfilename))
            return;
        auto assetlist = rp::serialization::yaml_serializer::deserialize<std::map<std::string, rp::BasicIndexedGuid>>(assetfilename);
        for (auto [name, typed] : assetlist) {
            ResourceSystem::FileEntry fentry{};
            fentry.m_Guid = typed.m_guid;
            std::string tname = rp::ResourceTypeRegistry::GetResourceTypeName(typed).value();
            fentry.m_Path = imported_path + "/" + typed.m_guid.to_hex() + "." + (tname == "font_atlas"?"font":tname);
            fentry.m_Size = std::filesystem::file_size(fentry.m_Path);
            ResourceSystem::Instance().m_FileEntries.emplace(typed.m_guid, fentry);
            ResourceSystem::Instance().m_NameToGuid[name] = typed.m_guid;
        }
    }

    const char* GetMappedFilePtr(rp::Guid);
    template <typename Fn, typename ...Args>
    auto Dispatch(Fn&& fn, Args&&... args) {
        return m_JobSystem.submit({}, {}, JobSys::make_packaged_job(std::forward<Fn>(fn), std::forward<Args>(args)...));
    }

    static void LoadFileLists(std::string_view filelist);
    static void LoadConfig(YAML::Node& cfg);
    static YAML::Node GetDefaultConfig();

    rp::Guid GetGuidByName(std::string const& name) const {
        auto it = m_NameToGuid.find(name);
        return it != m_NameToGuid.end() ? it->second : rp::null_guid;
    }

    std::unordered_map<rp::Guid, FileEntry> m_FileEntries;
    std::unordered_map<std::string, MemoryMappedFile> m_MappedIO;
    std::unordered_map<std::string, rp::Guid> m_NameToGuid;

private:
    std::string m_ResourceRootDirectory;
    bool m_GlobFiles{};
    JobSystem m_JobSystem{4};
};

#ifndef ESCAPE_PARENTHESIS
#define ESCAPE_PARENTHESIS(...) __VA_ARGS__
#endif

#define REGISTER_RESOURCE_TYPE_SHARED_PTR(T, loader_fn, unloader_fn)            \
    namespace {                                                      \
        struct T##_resource_registrar {                              \
            T##_resource_registrar() {                               \
                ResourceRegistry::Instance().RegisterType<std::shared_ptr<T>>(       \
                    loader_fn, unloader_fn, #T);                     \
            }                                                        \
        };                                                           \
        static T##_resource_registrar g_##T##_resource_registrar;    \
    }

#define REGISTER_RESOURCE_TYPE(T, loader_fn, unloader_fn)            \
    namespace {                                                      \
        struct T##_resource_registrar {                              \
            T##_resource_registrar() {                               \
                ResourceRegistry::Instance().RegisterType<T>(       \
                    loader_fn, unloader_fn, #T);                     \
            }                                                        \
        };                                                           \
        static T##_resource_registrar g_##T##_resource_registrar;    \
    }

#define REGISTER_RESOURCE_TYPE_ALIASE(T, A, loader_fn, unloader_fn)            \
    namespace {                                                      \
        struct A##_resource_registrar {                              \
            A##_resource_registrar() {                               \
                ResourceRegistry::Instance().RegisterType<T>(        \
                    loader_fn, unloader_fn, #A);                     \
            }                                                        \
        };                                                           \
        static A##_resource_registrar g_##A##_resource_registrar;    \
    }


template <typename T, stl_allocator_t<T> A>
ResourceHandle ResourcePool<T, A>::GetHandle(rp::Guid guid) {
    auto it = m_GuidSlots.find(guid);
    if (it != m_GuidSlots.end()) {
        return MakeHandle(it->second);
    }

    std::uint32_t idx = AllocateSlot();
    auto& slot = m_Slots[idx];
    slot.m_Guid = guid;
    slot.m_Alive = true;

    auto mmio_ptr{ ResourceSystem::Instance().GetMappedFilePtr(guid) };
    auto async_dispatch_job_wrapper{ [&slot, guid, this, idx](auto loaderfn, const char* mmptr) {
        try {
            if constexpr (std::is_copy_constructible_v<T>) {
                new (slot.storage) T{ loaderfn(mmptr) };
            }
            else {
                new (slot.storage) T(std::move(loaderfn(mmptr)));
            }
        }
        catch (...) {
            spdlog::error("[Resource System] Error loading file for guid {}", guid.to_hex());
            DestroySlot(idx);
            FreeSlot(idx);
            return;
        }
            slot.m_Ready = true;
        }};

    //ResourceSystem::Instance().Dispatch(async_dispatch_job_wrapper, m_Loader, mmio_ptr);
    async_dispatch_job_wrapper(m_Loader, mmio_ptr);
    m_GuidSlots.emplace(guid, idx);
    return MakeHandle(idx);
}

inline bool ResourceRegistry::Exists(rp::BasicIndexedGuid biguid) const {
    return ResourceSystem::Instance().m_FileEntries.find(biguid.m_guid) != ResourceSystem::Instance().m_FileEntries.end();
}

#endif //!ENGINE_RESOURCE_SYSTEM_HPP
