#include "Ecs/internal/reflection.h"

TypeInfo ResolveType(TypeName t_name) {
    return entt::resolve(t_name);
}