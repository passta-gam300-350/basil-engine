#ifndef ENGINE_RESOURCE_SYSTEM_HPP
#define ENGINE_RESOURCE_SYSTEM_HPP

#include <memory>
#include <jobsystem.hpp>
#include <hashtable.hpp>
#include <serialisation/guid.h>
#include <cstdint>
#include <cstddef>
#include <unordered_map>
#include <vector>
#include <type_traits>
#include <utility>
#include <string_view>

struct Handle {
    std::uint32_t m_Index{};
    std::uint32_t m_Generation{};
    explicit operator bool() const noexcept { return m_Generation != 0; }
};

using ResourceTypeId_t = std::uintptr_t;

template <typename T>
struct ResourceTypeInitiation {
    static constexpr std::int32_t m_TypeInstance{};
};

template <typename T>
constexpr ResourceTypeId_t ResourceTypeId_v{ &ResourceTypeInitiation<T>::m_TypeInstance };

template <typename T>
constexpr ResourceTypeId_t ResourceType_Id() noexcept {
    return ResourceTypeId_v<T>;
}

template <typename T>
struct ResourceSlot {
    Resource::Guid m_Guid{ Resource::null_guid };
    std::uint32_t m_Generation{};
    std::atomic<bool> m_Ready{};
    bool m_Alive{};
    alignas(T) unsigned char storage[sizeof(T)];
    T& value() noexcept { return *std::launder(reinterpret_cast<T*>(storage)); }
    const T& value() const noexcept { return *std::launder(reinterpret_cast<const T*>(storage)); }
};

template <typename T, stl_allocator_t<T> A = std::allocator<T>>
class ResourcePool {
public:
    using LoaderFn = std::function<T&(std::ifstream&)>; //effectively the constructor of memory pool. throwable
    using UnloaderFn = std::function<void(T&)>; //destructs only, deallocation happens lazily. throwable
    using ValueType = T;
    using Pointer = T*;

    explicit ResourcePool(LoaderFn loadfn, UnloaderFn unloadfn)
        : m_Loader(loadfn), m_Unloader(unloadfn) {
    }

    //get async, check handle, functionally equivalent of std::future but unlimited gets
    Handle GetHandle(Resource::Guid id);

    Pointer Ptr(Handle h) noexcept;

    const Pointer* Ptr(Handle h) const noexcept {
        if (h.index >= m_Slots.size()) return nullptr;
        auto& s = m_Slots[h.index];
        if (!s.m_Alive || s.Generation != h.Generation) return nullptr;
        return &s.value();
    }

    Handle Find(Resource::Guid guid) const noexcept {
        auto it = m_GuidSlots.find(id);
        if (it == m_GuidSlots.end()) return {};
        return MakeHandle(it->second);
    }

    Resource::Guid GetGuid(Handle h) const noexcept {
        if (h.m_Index >= m_Slots.size()) return 0;
        auto& s = m_Slots[h.m_Index];
        if (!s.m_Alive || s.m_Generation != h.m_Generation) return 0;
        return s.m_Guid;
    }

    bool Unload(Resource::Guid id) {
        auto it = m_GuidSlots.find(id);
        if (it == m_GuidSlots.end()) return false;
        const auto idx = it->second;
        DestroySlot(idx);
        m_GuidSlots.erase(it);
        FreeSlot(idx);
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
    Handle MakeHandle(std::uint32_t idx) const noexcept {
        const auto& s = m_Slots[idx];
        return Handle{ idx, s.m_Generation };
    }

    std::uint32_t AllocateSlot() {
        if (!FreeList.empty()) {
            auto idx = FreeList.back();
            FreeList.pop_back();
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
        Unloader(s.value());
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
    std::unordered_map<Resource::Guid, std::uint32_t> m_GuidSlots;
};

struct ResourceRegistry {
    //type-erased vtable mapping for resource type
    struct VTable {
        std::function<Handle(void*, Resource::Guid)> m_Get;
        std::function<void*()> m_GetPool;
        std::function<Handle(void*, Resource::Guid)> m_Find;
        std::function<Resource::Guid(void*, Handle)> m_GetGuid;
        std::function<void* (void*, Handle)> m_Ptr;
        std::function<const void* (const void*, Handle)> m_ConstPtr;
        std::function<bool (void*, Resource::Guid)> m_Unload;
    };

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
        if (m_Entries.find(id) != m_Entries.end()) return;

        // Create per-type singleton pool lazily
        static Pool<T> pool(loader, unloader);

        Entry e;
        e.m_Pool = &pool;
        e.m_TypeName = type_name.empty() ? std::string_view(typeid(T).name()) : type_name;

        e.vt = {
            //get
            [](void* p, Resource::Guid g) -> Handle {
                return static_cast<PoolT<T>*>(p)->Get(g);
            },
            //get_pool
            []() -> void* { return static_cast<void*>(&pool); },
            //find
            [](void* p, Resource::Guid g) -> Handle {
                return static_cast<PoolT<T>*>(p)->Find(g);
            },
            //guid
            [](void* p, Handle h) -> Resource::Guid {
                return static_cast<PoolT<T>*>(p)->GetGuid(h);
            },
            //ptr
            [](void* p, Handle h) -> void* {
                return static_cast<void*>(static_cast<PoolT<T>*>(p)->Ptr(h));
            },
            //const ptr
            [](const void* p, Handle h) -> const void* {
                return static_cast<const void*>(static_cast<const PoolT<T>*>(p)->Ptr(h));
            },
            //unload
            [](void* p, Resource::Guid g) -> bool {
                return static_cast<PoolT<T>*>(p)->Unload(g);
            }
        };

        m_Entries.emplace(id, e);
    }

    template <typename T>
    PoolType<T>* Pool() {
        auto it = m_Entries.find(ResourceType_Id<T>());
        return it == m_Entries.end() ? nullptr : static_cast<PoolType<T>*>(it->second.m_Pool);
    }

    template <typename T>
    T* Get(Resource::Guid guid, Handle* out_handle = nullptr) {
        auto* pool = Pool<T>();
        if (!pool) return nullptr;
        Handle h = pool->Get(guid);
        if (out_handle) *out_handle = h;
        return pool->Ptr(h);
    }

    template <typename T>
    Handle Find(Resource::Guid id) {
        auto* pool = Pool<T>();
        return pool ? pool->Find(id) : Handle{};
    }

    template <typename T>
    Resource::Guid GetGuid(Handle h) {
        auto* pool = Pool<T>();
        return pool ? pool->GetGuid(h) : Resource::null_guid;
    }

    template <typename T>
    bool Unload(Resource::Guid guid) {
        auto* pool = Pool<T>();
        return pool ? pool->Unload(guid) : false;
    }

private:
    hashtable<ResourceTypeId_t, Entry> m_Entries;
};

struct ResourceSystem {
    struct FileEntry {
        std::string m_Path;
        std::uint64_t m_Offset;
    };

    static ResourceSystem& Instance() {
        static ResourceSystem inst;
        return inst;
    }

    std::ifstream& GetMappedFilestream(Resource::Guid);
    template <typename Fn, typename ...Args>
    auto Dispatch(Fn&& fn, Args&&... args) {
        return m_JobSystem.schedule(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

private:
    hashtable<Resource::Guid, FileEntry> m_FileEntries;
    JobSystem m_JobSystem;
};

#define REGISTER_RESOURCE_TYPE(T, loader_fn, unloader_fn)            \
    namespace {                                                      \
        struct T##_resource_registrar {                              \
            T##_resource_registrar() {                               \
                ResourceRegistry::instance().register_type<T>(       \
                    loader_fn, unloader_fn, #T);                     \
            }                                                        \
        };                                                           \
        static T##_resource_registrar g_##T##_resource_registrar;    \
    }


template <typename T, stl_allocator_t<T> A>
Handle ResourcePool<T, A>::GetHandle(Resource::Guid guid) {
    auto it = m_GuidSlots.find(guid);
    if (it != m_GuidSlots.end()) {
        return make_handle(it->second);
    }

    std::uint32_t idx = AllocateSlot();
    auto& slot = Slots[idx];
    slot.m_Guid = guid;
    slot.m_Alive = true;

    std::ifstream& ifs{ ResourceSystem::Instance().GetMappedFilestream(guid) };
    auto async_dispatch_job_wrapper{ [&slot](loaderfn, ifstrm) {
        try {
            if constexpr (std::is_default_constructible_v<T>) {
                new (slot.storage) T{ loaderfn(ifstrm) };
            }
            else {
                new (slot.storage) T(std::move(loaderfn(ifstrm)));
            }
        }
        catch (...) {
            spdlog::error("[Resource System] Error loading file for guid {}", guid.to_hex_no_delimiter());
            DestroySlot(idx);
            FreeSlot(idx);
            return;
        }
            slot.m_Ready = true;
        }};

    ResourceSystem::Instance().Dispatch(async_dispatch_job_wrapper, m_Loader, ifs);

    m_GuidSlots.emplace(guid, idx);
    return MakeHandle(idx);
}

#endif //!ENGINE_RESOURCE_SYSTEM_HPP
