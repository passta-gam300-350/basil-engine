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
        virtual void LoadConfig(YAML::Node&) {};
        virtual YAML::Node GetDefaultConfig() { return YAML::Node(); };
		virtual void Update(world&, float) {};
		virtual void FixedUpdate(world&) {};
        virtual void Exit() {};
		virtual ~SystemBase() = 0 {};
	};

	struct SystemDescriptor {
		std::string m_Name;
        std::uint64_t m_Id;
		std::unordered_set<entt::id_type> m_Reads;
		std::unordered_set<entt::id_type> m_Writes;
		float m_UpdateRate;							
        bool m_PreUpdate;
        bool m_Enabled;

        SystemDescriptor() = default;
        SystemDescriptor(std::string const& mname, std::uint64_t mid, std::unordered_set<entt::id_type> const& mr, std::unordered_set<entt::id_type> const& mw, float ur, bool e, bool preup, std::function<SystemBase* ()> fn)
            :m_Name{mname}, m_Id{mid}, m_Reads{mr}, m_Writes{mw}, m_UpdateRate{ur}, m_PreUpdate{preup}, m_Enabled{e}, m_Factory{fn} {
        }

        friend struct SystemRegistry;

    private:
        std::function<SystemBase*()> m_Factory;   //creates an instance of system
	};

    template <typename ...queries_t>
    struct QuerySetBasic {
        static std::unordered_set<entt::id_type> GetSet() {
            std::unordered_set<entt::id_type> qset{};
            ([](std::unordered_set<entt::id_type>& query_set){
                query_set.insert(entt::type_hash<queries_t>::value());
                }(qset),...);
            return qset;
        }
    };

    template <>
    struct QuerySetBasic<> {
        static std::unordered_set<entt::id_type> GetSet() {
            return std::unordered_set<entt::id_type>();
        }
    };

    struct SystemRegistry {
    public:
        static SystemRegistry& Instance() {
            static SystemRegistry inst;
            return inst;
        }

        static void LoadConfig(YAML::Node&);

        static void Exit();

        void RegisterSystem(SystemDescriptor desc) {
            m_AllSystems.push_back(std::move(desc));
        }

        static YAML::Node GetDefaultConfig();

        // Returns only enabled systems
        std::vector<SystemDescriptor> GetActiveSystems() const {
            std::vector<SystemDescriptor> out;
            for (auto& s : m_AllSystems)
                if (s.m_Enabled) out.push_back(s);
            return out;
        }

        static SystemBase* GetSystem(std::uint64_t index) {
            return Instance().m_ActiveSystems[index].get();
        }

    private:
        std::vector<SystemDescriptor> m_AllSystems; //do not delete systems
        std::unordered_map<std::uint64_t, std::unique_ptr<SystemBase>> m_ActiveSystems;

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

    template <typename ... ComponentTypes>
    using ReadSet = QuerySetBasic<ComponentTypes...>;

    template <typename ... ComponentTypes>
    using WriteSet = QuerySetBasic<ComponentTypes...>;
    
    using EmptySet = QuerySetBasic<>;
    constexpr EmptySet EmptySetV{};
}

//move this struct to utility lib (more general)
//default
template <typename, template <typename...> class>
struct is_specialization_of : std::false_type {};

//true type
template <template <typename...> class Template, typename... Args>
struct is_specialization_of<Template<Args...>, Template> : std::true_type {};

template <typename T, template <typename...> class Template>
inline constexpr bool is_specialization_of_v = is_specialization_of<T, Template>::value;

#define ESCAPE_PARENTHESIS(...) __VA_ARGS__

#define RegisterSystemDerivedPreUpdate(NAME, TYPE, READS, WRITES, UPDATE_PER_SEC, ...)                           \
    namespace {                                                                                         \
        static_assert(std::is_base_of_v<ecs::SystemBase, TYPE> && #TYPE && "Type is not derived");                     \
        static_assert(!std::is_abstract_v<TYPE> && #TYPE && "Type should not be abstract");                    \
        static_assert(is_specialization_of_v<ESCAPE_PARENTHESIS READS, ecs::QuerySetBasic> && #READS && "Type should a query set");                    \
        static_assert(is_specialization_of_v<ESCAPE_PARENTHESIS WRITES, ecs::QuerySetBasic> && #WRITES && "Type should a query set");                    \
        inline static auto NAME##_SYSTEM_FACTORY = []()->ecs::SystemBase*{                                                 \
            return new TYPE{__VA_ARGS__};                                                               \
        };                                                                                              \
        inline static auto NAME##_SYSTEM_CONFIG_GENERATOR = []()->YAML::Node{                                                 \
            return YAML::Node{};                                                               \
        };                                                                                              \
        inline auto NAME##_SYSTEM_REG = [] {                                                                   \
            ecs::SystemRegistry::Instance().RegisterSystem({                                                 \
               #NAME, std::uint64_t(&NAME##_SYSTEM_FACTORY), ESCAPE_PARENTHESIS READS ::GetSet(), ESCAPE_PARENTHESIS WRITES ::GetSet(), UPDATE_PER_SEC, false, true, NAME##_SYSTEM_FACTORY                       \
                });                                                                                     \
            return 0;                                                                                   \
        }();                                                                                            \
    }

#define RegisterSystemDerived(NAME, TYPE, READS, WRITES, UPDATE_PER_SEC, ...)                           \
    namespace {                                                                                         \
        static_assert(std::is_base_of_v<ecs::SystemBase, TYPE> && #TYPE && "Type is not derived");                     \
        static_assert(!std::is_abstract_v<TYPE> && #TYPE && "Type should not be abstract");                    \
        static_assert(is_specialization_of_v<ESCAPE_PARENTHESIS READS, ecs::QuerySetBasic> && #READS && "Type should a query set");                    \
        static_assert(is_specialization_of_v<ESCAPE_PARENTHESIS WRITES, ecs::QuerySetBasic> && #WRITES && "Type should a query set");                    \
        inline static auto NAME##_SYSTEM_FACTORY = []()->ecs::SystemBase*{                                                 \
            return new TYPE{__VA_ARGS__};                                                               \
        };                                                                                              \
        inline static auto NAME##_SYSTEM_CONFIG_GENERATOR = []()->YAML::Node{                                                 \
            return YAML::Node{};                                                               \
        };                                                                                              \
        inline auto NAME##_SYSTEM_REG = [] {                                                                   \
            ecs::SystemRegistry::Instance().RegisterSystem({                                                 \
               #NAME, std::uint64_t(&NAME##_SYSTEM_FACTORY), ESCAPE_PARENTHESIS READS ::GetSet(), ESCAPE_PARENTHESIS WRITES ::GetSet(), UPDATE_PER_SEC, false, false, NAME##_SYSTEM_FACTORY                       \
                });                                                                                     \
            return 0;                                                                                   \
        }();                                                                                            \
    }

#define RegisterSystemGeneric(NAME, INIT_FN, LOAD_FN, UPDATE_FN, FIXED_UPDATE, EXIT_FN, UPDATE_PER_SEC, READS, WRITES)  \
    namespace {                                                                                         \
        static_assert(is_specialization_of_v<ESCAPE_PARENTHESIS READS, ecs::QuerySetBasic> && #READS && "Type should a query set");                    \
        static_assert(is_specialization_of_v<ESCAPE_PARENTHESIS WRITES, ecs::QuerySetBasic> && #WRITES && "Type should a query set");                    \
        inline static auto NAME##_SYSTEM_FACTORY = []()->ecs::SystemBase*{                                                 \
            return new ecs::GenericSystem{ INIT_FN, LOAD_FN, UPDATE_FN, FIXED_UPDATE, EXIT_FN };                      \
        };                                                                                              \
        inline auto NAME##_SYSTEM_REG = [&]{                                                                    \
            ecs::SystemRegistry::Instance().RegisterSystem({                                                 \
                #NAME, std::uint64_t(&NAME##_SYSTEM_FACTORY), ESCAPE_PARENTHESIS READS::GetSet(), ESCAPE_PARENTHESIS WRITES::GetSet(), UPDATE_PER_SEC, false, false, NAME##_SYSTEM_FACTORY                     \
            });                                                                                         \
            return 0;                                                                                   \
        }();                                                                                            \
    }

#endif