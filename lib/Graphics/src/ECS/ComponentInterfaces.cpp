#include "ECS/ComponentInterfaces.h"

namespace ComponentUtils {
    static IComponentAccessor* s_ComponentAccessor = nullptr;
    
    void SetComponentAccessor(IComponentAccessor* accessor) {
        s_ComponentAccessor = accessor;
    }
    
    IComponentAccessor* GetComponentAccessor() {
        return s_ComponentAccessor;
    }
}