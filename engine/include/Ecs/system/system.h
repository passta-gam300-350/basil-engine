#ifndef LIB_ECS_SYS_H
#define LIB_ECS_SYS_H

#include "ecs/fwd.h"
#include <string>
#include "ecs/internal/world.h"
#include <functional>
#include <yaml-cpp/yaml.h>

namespace ecs {
	struct SystemBase {
		virtual void Init() {};
        virtual void LoadConfig(YAML::Node& nd) {};
		virtual void Update(world&, float dt) {};
		virtual void FixedUpdate(world&) {};
        virtual void Exit() {};
		virtual ~SystemBase() = 0 {};
	};

	// Describes one system’s metadata and callable
	struct SystemDescriptor {
		std::string m_Name;
        std::uint64_t m_Id;
		std::vector<entt::id_type> m_Reads;
		std::vector<entt::id_type> m_Writes;
		float m_UpdateRate;							
		bool m_Enabled;									// set by YAML

        SystemDescriptor() = default;
        SystemDescriptor(std::string const& mname, std::uint64_t mid, std::vector<entt::id_type> const& mr, std::vector<entt::id_type> const& mw, float ur, bool e, std::function<SystemBase* (world&, float)> fn)
            :m_Name{mname}, m_Id{mid}, m_Reads{mr}, m_Writes{mw}, m_UpdateRate{ur}, m_Enabled{e}, m_Factory{fn} {
        }

    private:
        std::function<SystemBase*(world&, float)> m_Factory;   //creates an instance of system
	};

    struct SystemRegistry {
    public:
        static SystemRegistry& instance() {
            static SystemRegistry inst;
            return inst;
        }

        void RegisterSystem(SystemDescriptor desc) {
            m_AllSystems.push_back(std::move(desc));
        }

        // Returns only enabled systems
        std::vector<SystemDescriptor> GetActiveSystems() const {
            std::vector<SystemDescriptor> out;
            for (auto& s : m_AllSystems)
                if (s.m_Enabled) out.push_back(s);
            return out;
        }

    private:
        std::vector<SystemDescriptor> m_AllSystems; //do not delete systems
        std::unordered_map<std::uint64_t, std::unique_ptr<SystemBase*>> m_ActiveSystems;

        SystemRegistry() = default;
    };

    constexpr std::nullptr_t NullSystemFn = nullptr;

    class GenericSystem : public SystemBase {
    public:
        using InitFn = std::function<void()>;
        using LoadFn = std::function<void(YAML::Node&)>;
        using UpdateFn = std::function<void(world&, float)>;
        using FixedUpdateFn = std::function<void(world&)>;
        using ExitFn = std::function<void()>;

        GenericSystem(InitFn i, UpdateFn u, FixedUpdateFn f, ExitFn e)
            : initFn(std::move(i)), updateFn(std::move(u)), fixedUpdateFn(std::move(f)), exitFn(std::move(e)) {}

        void Init() override { if (initFn) initFn(); }
        void Update(world& w, float dt) override { if (updateFn) updateFn(w, dt); }
        void FixedUpdate(world& w) override { if (fixedUpdateFn) fixedUpdateFn(w); }
        void Exit() override { if (exitFn) exitFn(); }

    private:
        InitFn initFn;
        UpdateFn updateFn;
        FixedUpdateFn fixedUpdateFn;
        ExitFn exitFn;
    };

#define RegisterSystemDerived(NAME, TYPE, READS, WRITES, UPDATE_PER_SEC, ...)                           \
    namespace {                                                                                         \
        static_assert(std::is_base_of_v<SystemBase, TYPE> && #TYPE && "Type is not derived");                     \
        static_assert(!std::is_abstract_v<TYPE> && #TYPE && "Type should not be abstract");                    \
        static auto NAME##_SYSTEM_FACTORY = []()->SystemBase*{                                                 \
            return new TYPE{__VA_ARGS__};                                                               \
        };                                                                                              \
        auto NAME##_SYSTEM_REG = [&] {                                                                   \
            SystemRegistry::instance().RegisterSystem({                                                 \
               #NAME, std::uint64_t(&NAME##_SYSTEM_FACTORY), READS, WRITES, UPDATE_PER_SEC, false, NAME##_SYSTEM_FACTORY                       \
                });                                                                                     \
            return 0;                                                                                   \
        }();                                                                                            \
    }

#define RegisterSystemGeneric(NAME, INIT_FN, LOAD_FN, UPDATE_FN, FIXED_UPDATE, EXIT_FN, UPDATE_PER_SEC, READS, WRITES)  \
    namespace {                                                                                         \
        static auto NAME##_SYSTEM_FACTORY = []()->SystemBase*{                                                 \
            return new GenericSystem{ INIT_FN, LOAD_FN, UPDATE_FN, FIXED_UPDATE, EXIT_FN };                      \
        };                                                                                              \
        auto NAME##_SYSTEM_REG = [&]{                                                                    \
            SystemRegistry::instance().RegisterSystem({                                                 \
                #NAME, std::uint64_t(&NAME##_SYSTEM_FACTORY), READS, WRITES, UPDATE_PER_SEC, false, NAME##_SYSTEM_FACTORY                     \
            });                                                                                         \
            return 0;                                                                                   \
        }();                                                                                            \
    }


        
}

#endif