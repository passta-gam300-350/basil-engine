#ifndef LIB_ECS_SCHEDULER
#define LIB_ECS_SCHEDULER

#include <entt/entt.hpp>
#include <vector>
#include <unordered_set>
#include <string>
#include <stdexcept>
#include <future>
#include <iostream>

#include "ecs/fwd.h"
#include "ecs/internal/world.h"

namespace ecs {

    //unit of work
    struct task {

    };

    template <typename ...requires_t>
    struct query_set {
        template<typename ...excludes_t>
        query_set(excludes_t...);

        std::unordered_set<entt::id_type> reads;
        std::unordered_set<entt::id_type> writes;
        std::unordered_set<entt::id_type> excludes;
    };

    template <>
    struct query_set<> {
        query_set(std::unordered_set<entt::id_type> const& read_set, std::unordered_set<entt::id_type> const& write_set);
        query_set(std::unordered_set<entt::id_type> const& read_set, std::unordered_set<entt::id_type> const& write_set, std::unordered_set<entt::id_type> const& exclude_set);

        std::unordered_set<entt::id_type> reads;
        std::unordered_set<entt::id_type> writes;
        std::unordered_set<entt::id_type> excludes;
    };

    //pipeline for work, task are pushed into the pipeline, owns the worker thread
    struct pipeline {
        struct task_list {

        };
    };

    // A scheduler for entt systems that attempts to parallelize system execution.
    struct Scheduler {
    public:
        using system_fn = system_callback;

        // Forward declaration for the fluent builder.
        struct SystemBuilder;

        /**
         * @brief Adds a new system to the scheduler.
         * @param fn The system function to be executed.
         * @return A SystemBuilder instance to declare component dependencies.
         */
        SystemBuilder add_system(system_fn fn);

        /**
         * @brief Analyzes system dependencies and creates an execution schedule.
         * This must be called after adding all systems and before calling run().
         * The schedule is composed of stages, where all systems in a stage can run in parallel.
         */
        void compile();

        /**
         * @brief Executes the compiled schedule of systems on a given registry.
         * @param registry The entt::registry to operate on.
         */
        void run(world wrld);

        Scheduler() = default;
        Scheduler(Scheduler const& other) : m_nodes{ other.m_nodes }, m_schedule{ other.m_schedule }, m_compiled{ other.m_compiled } {}
        Scheduler(Scheduler&& other) noexcept : m_nodes{ std::move(other.m_nodes) }, m_schedule{ std::move(other.m_schedule) }, m_compiled{ other.m_compiled } {}

        void debug_print_schedule();

    private:
        // Internal representation of a system and its dependencies.
        struct SystemNode {
            system_fn function;
            std::unordered_set<entt::id_type> reads;
            std::unordered_set<entt::id_type> writes;
            std::string name = "unnamed system";
            std::uint64_t flag;

            SystemNode() = default;
            SystemNode(system_fn fn) : function{ std::move(fn) }, reads{}, writes{} {}
            SystemNode(SystemNode const& other) : function{ other.function }, reads{ other.reads }, writes{ other.writes }, name{ other.name } {}
            SystemNode(SystemNode&& other) noexcept : function{ std::move(other.function) }, reads{ std::move(other.reads) }, writes{ std::move(other.writes) }, name{ std::move(other.name) } {}
        };

        // The list of all systems added to the scheduler.
        std::vector<SystemNode> m_nodes;

        // The compiled execution plan. Each inner vector is a "stage" of system
        // indices that can be executed in parallel.
        std::vector<std::vector<size_t>> m_schedule;

        bool m_compiled = false;

    public:
        /**
         * @class SystemBuilder
         * @brief A helper class providing a fluent API to define system dependencies.
         */
        struct SystemBuilder {
        public:
            SystemBuilder(Scheduler& scheduler, SystemNode& node)
                : m_scheduler(scheduler), m_node(node) {
            }

            SystemBuilder(SystemBuilder const& other)
                : m_scheduler(other.m_scheduler), m_node(other.m_node) {
            }

            /**
             * @brief Declares that the system reads from component T.
             * @tparam T The component type.
             * @return A reference to the builder for chaining.
             */
            template<typename T>
            SystemBuilder& read() {
                m_node.reads.insert(entt::type_hash<T>::value());
                return *this;
            }

            /**
             * @brief Declares that the system writes to component T.
             * @tparam T The component type.
             * @return A reference to the builder for chaining.
             */
            template<typename T>
            SystemBuilder& write() {
                m_node.writes.insert(entt::type_hash<T>::value());
                return *this;
            }

            /**
             * @brief Sets an optional name for the system for debugging.
             * @param name The name of the system.
             * @return A reference to the builder for chaining.
             */
            SystemBuilder& set_name(std::string_view name) {
                m_node.name = name;
                return *this;
            }

            // Allow chaining `add_system` calls directly.
            SystemBuilder add_system(system_fn fn) {
                return m_scheduler.add_system(std::move(fn));
            }

        private:
            Scheduler& m_scheduler;
            SystemNode& m_node;
        };
    };

    inline Scheduler::SystemBuilder Scheduler::add_system(system_fn fn) {
        m_compiled = false;
        m_nodes.emplace_back(SystemNode{ std::move(fn) });
        return SystemBuilder(*this, m_nodes.back());
    }

    inline void Scheduler::compile() {
        m_schedule.clear();

        const size_t num_systems = m_nodes.size();
        if (num_systems == 0) {
            m_compiled = true;
            return;
        }

        // 1. Build the dependency graph.
        // An edge from A to B means A must run before B.
        std::vector<std::vector<size_t>> adj(num_systems);
        std::vector<size_t> in_degree(num_systems, 0);

        for (size_t i = 0; i < num_systems; ++i) {
            for (size_t j = i + 1; j < num_systems; ++j) {
                bool dependency_found = false;

                // Check for conflicts: i -> j (i must run before j)
                // This happens if i writes to a component that j reads from or writes to.
                // For write-write conflicts, the order of add_system() calls serves as a tie-breaker.
                for (const auto& write_comp : m_nodes[i].writes) {
                    if (m_nodes[j].reads.count(write_comp) || m_nodes[j].writes.count(write_comp)) {
                        adj[i].push_back(j);
                        in_degree[j]++;
                        dependency_found = true;
                        break;
                    }
                }
                if (dependency_found) continue;

                // Check for conflicts: j -> i (j must run before i)
                // This happens if j writes to a component that i reads from or writes to.
                for (const auto& write_comp : m_nodes[j].writes) {
                    if (m_nodes[i].reads.count(write_comp) || m_nodes[i].writes.count(write_comp)) {
                        adj[j].push_back(i);
                        in_degree[i]++;
                        break;
                    }
                }
            }
        }

        // 2. Perform a topological sort (Kahn's algorithm) to create execution stages.
        std::vector<size_t> queue;
        for (size_t i = 0; i < num_systems; ++i) {
            if (in_degree[i] == 0) {
                queue.push_back(i);
            }
        }

        size_t processed_count = 0;
        while (!queue.empty()) {
            m_schedule.push_back(queue);
            processed_count += queue.size();

            std::vector<size_t> next_queue;
            for (const size_t u : queue) {
                for (const size_t v : adj[u]) {
                    in_degree[v]--;
                    if (in_degree[v] == 0) {
                        next_queue.push_back(v);
                    }
                }
            }
            queue = std::move(next_queue);
        }

        // 3. Check for dependency cycles.
        if (processed_count != num_systems) {
            throw std::runtime_error("A cycle was detected in the system dependency graph.");
        }

        m_compiled = true;
    }

    inline void Scheduler::run(world wrld) {
        if (!m_compiled) {
            throw std::runtime_error("Scheduler must be compiled before running.");
        }

        for (const auto& stage : m_schedule) {
            if (stage.empty()) continue;

            // Optimization: If a stage has only one system, no need for async overhead.
            if (stage.size() == 1) {
                m_nodes[stage[0]].function(wrld);
            }
            else {
                std::vector<std::future<void>> futures;
                futures.reserve(stage.size());

                for (const size_t system_index : stage) {
                    // Launch each system in the stage asynchronously.
                    futures.push_back(std::async(std::launch::async, [&, system_index]() {
                        this->m_nodes[system_index].function(wrld);
                        }));
                }

                // Create a synchronization barrier: wait for all systems in the current stage to complete.
                for (auto& f : futures) {
                    f.get();
                }
            }
        }
    }
    inline void Scheduler::debug_print_schedule()
    {
        if (m_compiled) {
            std::cout << "[Scheduler] Compiled execution plan:\n";
            int stage_num = 0;
            for (const auto& stage : m_schedule) {
                std::cout << "  Stage " << ++stage_num << ": ";
                for (const auto& system_index : stage) {
                    std::cout << m_nodes[system_index].name << " | ";
                }
                std::cout << "\n";
            }
            std::cout << "-------------------------------------\n";
        }
    }

    template <ecs_system_callback ecs_system_callback_t>
    auto world::add_system(ecs_system_callback_t sys_fn) {
        return impl.get_scheduler().add_system(sys_fn);
    }
}

#endif