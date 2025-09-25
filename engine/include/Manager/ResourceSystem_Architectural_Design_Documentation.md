# Resource System Architectural Design
## Motivation
Design a robust, generic plug and play resource system that is capable of handling and management of underlying resources in demanding time constraint high performance scenarios 
## Constraints
1. Resource System is solely responsible and takes full ownership of resources. It should be considered the single point of truth for resolution of resource request
2. There should only be 1 instance of a resource at any given time.
2. Resource types functions as plugins during compile time
3. Resources are not referenced with names.
4. The main means of resource identification and resolution is guid.
5. The resource system manages its memory budget that is allocated at time of construct. It may grow or shrink its memory pools but should not exceed the define max memory budget

## Requirements
1. Asynchronous loading of resources
2. Resource priorities and strong reference optimisation
3. Efficient job scheduling
4. Delayed unloading and resource recall mechanisms
5. Compile-time double dispatch for resource loading and acquistion
6. Threadsafe bookkeeping of control blocks

## Resource type
### Concept
A resource is any arbituary type that inherits the abstract class resource_type_base and defines the functions to handle the loading and unloading of the data
```cxx
template <typename Loadfn, typename Unloadfn, typename ... RawTypes>
struct ResourceType : public ResourceTypeBase {
    void Load(const char* buffer) override { std::apply(m_Load, m_Raw); };
    void Unload() override { std::apply(m_Unload, m_Raw); };
    ~ResourceType() noexcept;

    ...

    Unloadfn m_Unload;
    Loadfn m_Load;
    std::tuple<RawTypes...> m_Raw;
};
```
```cxx
struct ResourceTypeBase{
    virtual void Load(const char* buffer) = 0;
    virtual void Unload() = 0;
    virtual ~ResourceTypeBase() noexcept;
};
```
## Resource control block
### Concept
A control block that defines the state of the resource, bookkeeps the references of the resource. Resolve handles and underlying resource referencing
### Structure
```cxx
struct ResourceControlBlock{
    std::atomic<int32_t> m_References;
    ResourceState m_State;
    ResourceCallback m_cb;
};
```
## Resource handle 
### Concept
An opaque handle to the resource for resource synchronisation and bookkeeping. Resource control block abstraction and direct resource reference
this handle is the main interface for resource acquirisation. This is functionally similar to a composition of std::shared_future shared get() and std::shared_ptr reference management

## Resource registry
### Concept
The main registry for the resource system. Assumes full ownership of all managed resource, resource control block and issues handles
Resolves resource acquisition request and maintains guid to resource handle mapping. 

### Structure
```cxx
struct ResourceRegistry{
    std::unordered_map<Resource::Guid, std::unique_ptr<ResourceTypeBase>> m_RawResources;
    std::unordered_map<Resource::Guid, ResourceControlBlock> m_ManagedResources;
    ...
};
```