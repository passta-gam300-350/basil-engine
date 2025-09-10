#include "ecs/system/scheduler.h"

std::atomic<std::uint64_t> ecs::pipeline::thread_ct{};
const std::uint64_t ecs::pipeline::thread_limit{ std::thread::hardware_concurrency() };