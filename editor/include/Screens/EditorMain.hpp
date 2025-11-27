/******************************************************************************/
/*!
\file   EditorMain.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)
		Eirwen (c.lau\@digipen.edu)
		Hai Jie (haijie.w\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contain the declaration for the EditorMain class, which is the
main editor screen handling the viewport, entity management, and various editor panels.
It integrates with the rendering system and ECS to provide a functional editor environment.
It includes features like scene exploration, entity inspection, and camera controls. It allows
for creating, selecting, and manipulating entities within the scene.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef EDITORMAIN_HPP
#define EDITORMAIN_HPP

#include "imgui.h"
#include "ImGuizmo.h"
#include "Screens/Screen.hpp"
#include "Camera/EditorCamera.hpp"
#include "ecs/fwd.h"
#include "Manager/AssetManager.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <components/behaviour.hpp>
#include "Render/Camera.h"
#include "Service/FileService.hpp"
#include "Service/EngineService.hpp"
#include <rsc-ext/rp.hpp>
#include "Physics/Physics_Components.h"
#include "Prefab/PrefabData.hpp"
#include "Component/PrefabComponent.hpp"
#include "Messaging/Messaging_System.h"

struct AudioComponent; // Forward declaration

// Prefab override detection context (global state for rendering)
struct PrefabOverrideContext {
	bool isPrefabInstance = false;
	const PrefabComponent* prefabComponent = nullptr;
	const PrefabData* prefabData = nullptr;
	std::uint32_t currentComponentTypeHash = 0;
	std::string currentComponentTypeName = "";
};

class EditorMain : public Screen
{
	// ========================================================================
	// SERVICES
	// ========================================================================
	FileService fileService;
	EngineContainerService engineService;

public:
	// ========================================================================
	// PANEL VISIBILITY FLAGS
	// ========================================================================
	bool showAboutModal = false;
	bool showInspector = true;
	bool showSceneExplorer = true;
	bool showProfiler = true;
	bool showConsole = true;
	bool showSkyboxSettings = true;  // Unity-style skybox settings panel
	bool showPhysicsDebug = true;  // Toggle for Jolt Physics debug rendering (from m3-physics)


	// ========================================================================
	// PLAY MODE STATE
	// ========================================================================
	bool isPlaying = false; // Gameplay is enabled (not paused)
	bool isPaused = false;  // Gameplay is paused

	// ========================================================================
	// GIZMO STATE
	// ========================================================================
	ImGuizmo::OPERATION mode = (ImGuizmo::OPERATION)0;
	glm::mat4 GuizmoViewMec4;
	glm::mat4 GuizmoprojectionMat4;
	TransformComponent* GuizmoEntityTransform;
	TransformMtxComponent* GuizmoEntityTransformMTX;
	TransformMtxComponent* GuizmoEntityParentTransformMTX;

	// ========================================================================
	// LIFECYCLE
	// ========================================================================
	EditorMain(GLFWwindow* window);
	~EditorMain() override = default;

	void init() override;
	void update() override;
	void render() override;
	void cleanup() override;
	void Show() override;
	void Close() override;
	virtual bool isWindowClosed() override;
	bool Activate() override;

	// ========================================================================
	// EDITOR UI SETUP
	// ========================================================================
	void Setup_Dockspace(unsigned id);
	void SetupUnityStyle();

	// ========================================================================
	// MENU AND TOOLBAR RENDERING
	// ========================================================================
	void Render_MenuBar();
	void Render_StartStop();
	void Render_AboutUI();

	// ========================================================================
	// PANEL RENDERING
	// ========================================================================
	void Render_SceneExplorer();    // Hierarchy panel
	void Render_Inspector();         // Inspector panel
	void Render_Console();           // Console panel
	void Render_Profiler();          // Profiler panel
	void Render_PhysicsDebugPanel(); // Physics debug panel

	// ========================================================================
	// ASSET BROWSER
	// ========================================================================
	void Render_AssetBrowser();
	void Render_AssetBrowser_Old();
	void Render_ImporterSettings();

	// ========================================================================
	// VIEWPORT RENDERING
	// ========================================================================
	void Render_Scene();           // Scene viewport
	void Render_Game();            // Game viewport
	void Render_CameraControls();  // Camera control overlay
	void Gizmos(ImVec2 viewportPos, ImVec2 viewportSize);

	void Render_SkyboxSettings();

	// ========================================================================
	// INSPECTOR COMPONENT RENDERING
	// ========================================================================
	void Render_Components();
	void Render_Component_Member(auto&, bool& is_dirty);
	void Render_Behaviour_Component(behaviour& component);
	void Add_Script_Menu();
	void Render_Add_Component_Menu();

	// Physics component rendering
	bool Render_RigidBody_Component(RigidBodyComponent& component);
	bool Render_BoxCollider_Component(BoxCollider& component);
	bool Render_SphereCollider_Component(SphereCollider& component);
	bool Render_CapsuleCollider_Component(CapsuleCollider& component);

	// ========================================================================
	// ENTITY CREATION
	// ========================================================================
	void CreateObjectHelper(); // Helper menu for creating objects

	// ========================================================================
	// ACCESSORS
	// ========================================================================
	AssetManager* GetAssetManager() { return m_AssetManager.get(); }

private:
	// ========================================================================
	// ENTITY CREATION (PRIVATE)
	// ========================================================================
	void CreateDefaultEntity();
	void CreatePlaneEntity();
	void CreateLightEntity();
	void CreateCameraEntity();
	void CreateDemoScene();
	void CreatePhysicsDemoScene();
	void CreatePhysicsCube();

	// Demo scene utilities
	void CreateCube(const glm::vec3& position = glm::vec3(0.0f),
	                const glm::vec3& scale = glm::vec3(1.0f),
	                const glm::vec3& color = glm::vec3(1.0f, 0.0f, 0.0f));
	void CreateCubeGrid(int gridSize = 3, float spacing = 3.0f);

	// ========================================================================
	// CAMERA AND VIEWPORT
	// ========================================================================
	std::shared_ptr<EditorCamera> m_EditorCamera;


	// Physics debug rendering state
	bool m_PhysicsDebugRenderingEnabled = true;

	// Viewport size tracking for aspect ratio
	float m_ViewportWidth = 1280.0f;
	float m_ViewportHeight = 720.0f;

	// ========================================================================
	// ASSET MANAGEMENT
	// ========================================================================
	std::unique_ptr<AssetManager> m_AssetManager;

	// Prefab override detection
	PrefabOverrideContext m_PrefabContext;
	std::unique_ptr<PrefabData> m_LoadedPrefabData;  // Currently loaded prefab for comparison

	// ========================================================================
	// PLAY MODE
	// ========================================================================
	bool m_IsPlayMode = false;

	// ========================================================================
	// ENTITY SELECTION
	// ========================================================================
	uint32_t m_SelectedEntityID = 0;  // Currently selected entity's object ID (0 = none)
	uint32_t m_SelectedNodeID = 0;    // Selected scene graph node ID
	bool m_ShowSelectionInfo = true;  // Show selection info in inspector
	std::unordered_set<uint32_t> m_EntitiesIDSelection; //all selected entities

	// Selection functions
	void HandleViewportPicking();
	void PerformEntityPicking(float mouseX, float mouseY, float viewportWidth, float viewportHeight);
	void SelectEntity(uint32_t objectID);
	void ClearEntitySelection();

	// ========================================================================
	// DEBUG RENDERING
	// ========================================================================
	bool m_ShowAABBs = false;
	void SetDebugVisualization(bool showAABBs);

	// ========================================================================
	// HIERARCHY PANEL STATE
	// ========================================================================
	char m_HierarchySearchBuffer[256] = "";

	// Inline rename state
	uint32_t m_RenamingEntityUID = UINT32_MAX;  // UINT32_MAX means not renaming
	char m_RenameBuffer[256] = "";
	bool m_RenameJustActivated = false;    // For auto-focus and select-all

	// ========================================================================
	// ASSET BROWSER STATE
	// ========================================================================
	enum class AssetViewMode { Grid, List };
	AssetViewMode m_AssetViewMode = AssetViewMode::Grid;
	char m_AssetSearchBuffer[256] = "";
	std::string m_SelectedAssetPath = "";
	float m_FolderTreeWidth = 200.0f;

	// Asset browser icon textures
	struct AssetIcons {
		unsigned int folderIcon = 0;
		unsigned int fileIcon = 0;
		unsigned int imageIcon = 0;
		unsigned int modelIcon = 0;
		unsigned int audioIcon = 0;
		unsigned int scriptIcon = 0;
		unsigned int materialIcon = 0;
		unsigned int shaderIcon = 0;
	} m_AssetIcons;

	void LoadAssetIcons();
	unsigned int GetIconForFile(const std::string& extension);

	// ========================================================================
	// MATERIAL CREATION
	// ========================================================================
	bool m_ShowCreateMaterialDialog = false;
	char m_NewMaterialNameBuffer[256] = "NewMaterial";

	// ========================================================================
	// PHYSICS DEBUG DATA
	// ========================================================================
	struct PhysicsDebugData {
		bool hasPhysicsBody = false;
		bool hasRigidBody = false;
		bool hasCollider = false;

		// Jolt Body Info
		uint32_t bodyID = 0;
		bool isBodyActive = false;
		bool isSleeping = false;
		std::string motionType = "None";
		std::string colliderType = "None";
		bool isTrigger = false;

		// Position/Rotation (from Jolt)
		glm::vec3 joltPosition = glm::vec3(0.0f);
		glm::vec3 joltRotation = glm::vec3(0.0f);

		// Velocities
		glm::vec3 linearVelocity = glm::vec3(0.0f);
		glm::vec3 angularVelocity = glm::vec3(0.0f);

		// Jolt Body Info
		float joltGravFactor = 0.0f;

		// RigidBody properties
		float mass = 0.0f;
		float friction = 0.0f;
		float restitution = 0.0f;
		float linearDamping = 0.0f;
		float angularDamping = 0.0f;
		float gravityFactor = 1.0f;
		bool useGravity = true;

		// Collider properties
		glm::vec3 colliderSize = glm::vec3(0.0f);
		float colliderRadius = 0.0f;
		float colliderHeight = 0.0f;
	} m_PhysicsDebugData;

	// ========================================================================
	// SCENE MANAGEMENT
	// ========================================================================
	void SaveScene(const char* path);
	void LoadScene(const char* name);
	void NewScene();
};

namespace rp {
	namespace serialization {
		template <>
		struct out_archive<"imgui"> {
			std::string m_tag_name;
			template <typename Type>
			void write(Type& v, std::string const& label) {
				const auto same_line_label{ [](auto& ss) {
					ImGui::Text(ss.c_str());
					ImGui::SameLine(250);
					ImGui::SetNextItemWidth(-1);
					} };
				auto strlabel{ ("##" + label) };
				const char* cstrlabel{ strlabel.c_str() };
				if constexpr (std::is_enum_v<Type>) {
					// Render enum as a combo box
					auto names = reflection::get_enum_list<Type>();
					int current = static_cast<int>(v);
					same_line_label(label);
					if (ImGui::BeginCombo(cstrlabel, std::string(reflection::map_enum_name(v)).c_str())) {
						for (auto [enum_name, enum_value] : names) {
							bool selected = (enum_value == v);
							if (ImGui::Selectable(std::string(enum_name).c_str(), selected)) {
								v = enum_value;
							}
							if (selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
				}
				else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, rp::Guid>) {
					// GUID as hex string
					std::string guid_str = v.to_hex();
					char buf[64];
					strncpy(buf, guid_str.c_str(), sizeof(buf));
					same_line_label(label);
					if (ImGui::InputText(cstrlabel, buf, sizeof(buf))) {
						v = rp::Guid::to_guid(buf);
					}
				}
				else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, std::string>) {
					char buf[512];
					strncpy(buf, v.c_str(), sizeof(buf));
					same_line_label(label);
					if (ImGui::InputText(cstrlabel, buf, sizeof(buf))) {
						v = buf;
					}
				}
				else if constexpr (std::is_same_v<Type, bool>) {
					same_line_label(label);
					ImGui::Checkbox(cstrlabel, &v);
				}
				else if constexpr (std::is_integral_v<Type>) {
					same_line_label(label);
					ImGui::InputInt(cstrlabel, reinterpret_cast<int*>(&v));
				}
				else if constexpr (std::is_floating_point_v<Type>) {
					same_line_label(label);
					if constexpr (std::is_same_v<Type, double>) {
						ImGui::InputDouble(cstrlabel, &v);
					}
					else {
						ImGui::InputFloat(cstrlabel, reinterpret_cast<float*>(&v));
					}
				}
				else if constexpr (std::is_same_v<Type, glm::vec2>) {
					same_line_label(label);
					ImGui::DragFloat2(cstrlabel, &v.x);
				}
				else if constexpr (std::is_same_v<Type, glm::vec3>) {
					same_line_label(label);
					ImGui::DragFloat3(cstrlabel, &v.x);
				}
				else if constexpr (std::is_same_v<Type, glm::vec4>) {
					same_line_label(label);
					ImGui::DragFloat4(cstrlabel, &v.x);
				}
				else if constexpr (std::is_same_v<Type, glm::ivec2>) {
					same_line_label(label);
					ImGui::DragInt2(cstrlabel, &v.x);
				}
				else if constexpr (std::is_same_v<Type, glm::ivec3>) {
					same_line_label(label);
					ImGui::DragInt3(cstrlabel, &v.x);
				}
				else if constexpr (std::is_same_v<Type, glm::ivec4>) {
					same_line_label(label);
					ImGui::DragInt4(cstrlabel, &v.x);
				}
				else if constexpr (reflection::is_associative_container_v<Type>) {
					ImGui::Text(label.c_str());
					for (auto& [key, val] : v) {
						ImGui::PushID(key.c_str());
						write(val, label + "." + key);
						ImGui::PopID();
					}
					if constexpr (std::is_same_v<typename Type::key_type, std::string>) {
						char buf[256]{};
						if (ImGui::InputText("##Add Item", buf, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
							v.emplace(std::string(buf), typename Type::mapped_type{});
						}
					}
				}
				else if constexpr (reflection::is_sequence_container_v<Type>) {
					int idx = 0;
					ImGui::Text(label.c_str());
					for (auto& elem : v) {
						ImGui::PushID(idx);
						write(elem, label + "[" + std::to_string(idx) + "]");
						ImGui::PopID();
						++idx;
					}
					if (ImGui::Button(("Add Item##" + label).c_str())) {
						v.emplace_back();
					}
				}
				else if constexpr (std::is_class_v<Type>) {
					if (ImGui::TreeNode(label.c_str())) {
						reflection::reflect(v).each([&](auto& field) {
							write(*field.m_field_ptr,
								std::string(field.m_field_name.begin(), field.m_field_name.end()));
							});
						ImGui::TreePop();
					}
				}
			}
			template <typename Type>
			void write(Type& v) {
				write(v, m_tag_name);
			}
		};


		template <>
		struct serializer<"imgui"> {
			using out_archive_type = out_archive<"imgui">;
			template <typename Type>
			static void present(Type& val, std::string const& name) {
				out_archive_type{ name }.write(val);
			}
			template <typename Type>
			static void present(Type& val, out_archive_type& arc) {
				arc.write(val);
			}
			template <typename Type>
			static void present(Type& val, out_archive_type&& arc) {
				arc.write(val);
			}
		};
	}
}

using ImguiInspectItem = rp::serialization::out_archive<"imgui">;
using ImguiInspectTypeRenderer = rp::serialization::serializer<"imgui">;

#define RegisterImguiDescriptorInspector(DESC) \
namespace {																															\
	inline static auto DESC##imgui_regi{ []{																						\
		constexpr auto type_hash{rp::utility::type_hash<DESC>::value()};															\
		rp::ResourceTypeImporterRegistry::RegisterSerializer(type_hash, "imgui", [](std::string const& str, std::byte* data) {		\
		DESC& desc{ *reinterpret_cast<DESC*>(data) };																				\
		ImguiInspectTypeRenderer::present(desc, str);																				\
		});																															\
		return 1u;																													\
	}() };																															\
}

#endif // EDITORMAIN_HPP
