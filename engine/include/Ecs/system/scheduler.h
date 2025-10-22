#ifndef LIB_ECS_SCHEDULER
#define LIB_ECS_SCHEDULER

#include <entt/entt.hpp>
#include <vector>
#include <unordered_set>
#include <string>
#include <stdexcept>
#include <iostream>
#include <jobsystem.hpp>
#include <queue>

#include "ecs/fwd.h"
#include "ecs/internal/world.h"
#include "ecs/system/system.h"

namespace ecs {
    struct SystemDependencyGraph {
        struct Node {
            size_t index{};                       // index in systems vector
            std::vector<size_t> successors;     // edges to other nodes
            size_t indegree = 0;                // for topological sort
        };

        std::vector<Node> nodes;
    };

    struct Scheduler {
        static std::unique_ptr<Scheduler>& InstancePtr() {
            static std::unique_ptr<Scheduler> inst{std::make_unique<Scheduler>()};
            return inst;
        }
        static void Release() {
            InstancePtr().reset();
        }
        static Scheduler& Instance() {
            return *InstancePtr();
        }

        //note: this is not expected to be called every frame
        static SystemDependencyGraph BuildDependencyGraph() {
            SystemDependencyGraph graph;
            auto systems = SystemRegistry::Instance().GetActiveSystems();

            // Map: system index -> graph node
            graph.nodes.resize(systems.size());
            for (size_t i = 0; i < systems.size(); ++i) {
                graph.nodes[i].index = systems[i].m_Id;
            }

            auto intersects = [](auto& lhs, auto& rhs) {
                for (auto& t : lhs)
                    if (std::find(rhs.begin(), rhs.end(), t) != rhs.end())
                        return true;
                return false;
                };

            for (size_t i = 0; i < systems.size(); ++i) {
                if (!systems[i].m_Enabled) continue;
                for (size_t j = 0; j < systems.size(); ++j) {
                    if (i == j || !systems[j].m_Enabled) continue;

                    bool conflict =
                        intersects(systems[i].m_Writes, systems[j].m_Reads) ||
                        intersects(systems[i].m_Reads, systems[j].m_Writes) ||
                        intersects(systems[i].m_Writes, systems[j].m_Writes);

                    if (conflict) {
                        graph.nodes[i].successors.push_back(j);
                        graph.nodes[j].indegree++;
                    }
                }
            }

            return graph;
        }

        static std::vector<std::vector<size_t>> TopologicalPhases(const SystemDependencyGraph& graph) {
            std::vector<std::vector<size_t>> phases;
            auto indegree = std::vector<size_t>(graph.nodes.size());
            for (size_t i = 0; i < graph.nodes.size(); ++i)
                indegree[i] = graph.nodes[i].indegree;

            std::queue<size_t> q;
            for (size_t i = 0; i < indegree.size(); ++i)
                if (indegree[i] == 0) q.push(i);

            while (!q.empty()) {
                std::vector<size_t> phase;
                size_t count = q.size();
                for (size_t k = 0; k < count; ++k) {
                    size_t idx = q.front(); q.pop();
                    phase.push_back(idx);
                    for (auto succ : graph.nodes[idx].successors) {
                        if (--indegree[succ] == 0) q.push(succ);
                    }
                }
                phases.push_back(std::move(phase));
            }

            return phases;
        }

        //very expensive do not run often
        static void CompileJobSchedule() {
            Scheduler& scheduler{ Instance() };
            scheduler.m_JobGraph = BuildDependencyGraph();
            scheduler.m_JobSchedulePhases = TopologicalPhases(scheduler.m_JobGraph);
            std::vector<std::vector<std::size_t>> jobdependencies{ scheduler.m_JobGraph.nodes.size()};
            for (std::size_t i{}; i < scheduler.m_JobGraph.nodes.size(); i++) {
                auto const& node{ scheduler.m_JobGraph.nodes[i] };
                for (std::size_t succ : node.successors) {
                    jobdependencies[succ].emplace_back(i);
                }
            }
            scheduler.m_JobDependencies = jobdependencies;
        }

        static JobID Run(world w) {
            Scheduler& scheduler{ Instance() };
            SystemDependencyGraph& jobgraph{ scheduler.m_JobGraph };
            std::vector<JobID> jobhandles{ jobgraph.nodes.size()};
            std::vector<JobID> jobdep{};
            for (auto const& phase : scheduler.m_JobSchedulePhases) {
                for (std::size_t node_idx : phase) {
                    //ensure that systemregistry do not destroy sysbase while scheduler is running
                    //SystemBase* sys_fn{ SystemRegistry::GetSystem(jobgraph.nodes[node_idx].index) };
                    for (auto dep_idx : scheduler.m_JobDependencies[node_idx]) {
                        jobdep.emplace_back(jobhandles[dep_idx]);
                    }
                    //JobID jid = scheduler.m_JobSystem.submit({}, jobdep, JobSys::make_packaged_job([](world wrld, SystemBase* sys) {sys->FixedUpdate(wrld); }, w, SystemRegistry::GetSystem(jobgraph.nodes[node_idx].index)));
                    JobID jid = scheduler.m_JobSystem.submit({}, jobdep, [](world wrld, SystemBase* sys) -> JobTask {
                        sys->FixedUpdate(wrld); 
                        co_return;
                        }, w, SystemRegistry::GetSystem(jobgraph.nodes[node_idx].index));
                    jobhandles[node_idx] = jid;
                    jobdep.clear();
                }
            }
            JobID finished = scheduler.m_JobSystem.submit({}, jobhandles, JobSys::make_packaged_job([] {}));
            return finished;
        }
        static void RunUntilCompletion(world w) {
            Instance().m_JobSystem.wait_for(Run(w));
        }

        static Scheduler& SetSystemThreads(std::int32_t thread_count) {
            Scheduler& inst{ Instance() };
            if (inst.m_JobSystem.get_thread_ct() == thread_count) {
                return inst;
            }
            inst.m_JobSystem.~JobSystem();
            new (&inst.m_JobSystem) JobSystem{ thread_count };
            return inst;
        }

        JobSystem m_JobSystem{4};
    private:
        std::vector<std::vector<std::size_t>> m_JobSchedulePhases;
        std::vector<std::vector<std::size_t>> m_JobDependencies;
        SystemDependencyGraph m_JobGraph;
    };
}

#endif