#include "Screens/EditorMain.hpp"
#include "Editor.hpp"
#include "GLFW/glfw3.h"

#include "MonoResolver/MonoTypeDescriptor.hpp"
#include "Manager/MonoImGuiRenderer.hpp"
#include <Manager/MonoEntityManager.hpp>
#include <Manager/MonoReflectionRegistry.hpp>
#include <Manager/MonoTypeRegistry.hpp>
#include "System/BehaviourSystem.hpp"

#include "Physics/Physics_System.h"


/*
By Default the physics is disabled, only collision is checked and displayed, but no resolution should happen, logic should also be disabled
When we are playing, phyiscs and logic should be enabled fully.
When Paused, phyics and logic should be paused.
when Stepped, phyics and logic is stepped by one
*/
void EditorMain::Render_StartStop()
{
	// Toolbar background
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.157f, 0.157f, 0.157f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));

	ImGui::BeginChild("##StartStop", ImVec2(0, 40), true, ImGuiWindowFlags_NoScrollbar);

	// Center the buttons
	float windowWidth = ImGui::GetContentRegionAvail().x;
	float buttonWidth = 60.0f; // Square buttons like Unity
	float spacing = 4.0f;
	float totalWidth = (buttonWidth * 3) + (spacing * 2);
	float offsetX = (windowWidth - totalWidth) * 0.5f;

	if (offsetX > 0)
		ImGui::SetCursorPosX(offsetX);

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0));

	// Play Button
	if (isPlaying)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.337f, 0.612f, 0.839f, 1.0f)); // Unity blue when active
	else
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.235f, 0.235f, 0.235f, 1.0f));

	if (ImGui::Button(isPlaying ? "Stop" : "Play", ImVec2(buttonWidth, 28)))
	{
		isPlaying = !isPlaying;
		if (isPlaying) // Starts game
		{

			SaveScene("tmp.yaml");
			engineService.ExecuteOnEngineThread([&]() {
				PhysicsSystem::Instance().isActive = true;
				PhysicsSystem::Instance().DisableObservers();
				BehaviourSystem::Instance().isActive = true;
				spdlog::info("Physics Active");


				});

			// No longer need to switch cameras - dual rendering handles both viewports
		}
		else // Stops Game
		{
			engineService.ExecuteOnEngineThread([]() {
				PhysicsSystem::Instance().Reset();
				PhysicsSystem::Instance().isActive = false;
				BehaviourSystem::Instance().isActive = false;
				});
			LoadScene("tmp.yaml");
			engineService.ExecuteOnEngineThread([]() {
				PhysicsSystem::Instance().EnableObservers();
				PhysicsSystem::Instance().CreateAllBodiesForLoadedScene();
				spdlog::info("Physics Off");
				});

			// No longer need to switch cameras - dual rendering handles both viewports
			isPaused = false; // Resets paused game as we are stopping
		}

	}
	ImGui::PopStyleColor();

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(isPlaying ? "Stop" : "Play");

	ImGui::SameLine();

	// Pause Button
	if (!isPlaying)
		ImGui::BeginDisabled();

	if (isPaused)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
	else
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.235f, 0.235f, 0.235f, 1.0f));

	if (ImGui::Button("Pause", ImVec2(buttonWidth, 28)))
	{
		isPaused = !isPaused;
		//OnPauseToggle();
	}
	ImGui::PopStyleColor();

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(isPaused ? "Resume" : "Pause");

	if (!isPlaying)
		ImGui::EndDisabled();

	ImGui::SameLine();

	// Step Button
	if (!isPaused)
		ImGui::BeginDisabled();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.235f, 0.235f, 0.235f, 1.0f));
	if (ImGui::Button("Step", ImVec2(buttonWidth, 28)))
	{
		//OnStep();
	}
	ImGui::PopStyleColor();

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Step Frame");

	if (!isPaused)
		ImGui::EndDisabled();

	ImGui::PopStyleVar(2);

	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

}

void EditorMain::Render_MenuBar()
{
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("File"))
	{
		ImGui::MenuItem("New Project", "Ctrl+Alt+N");
		ImGui::MenuItem("Open Project", "Ctrl+Alt+O");
		ImGui::MenuItem("Save Project", "Ctrl+Alt+S");
		ImGui::MenuItem("Save Project As", "Ctrl+Alt+Shift+S");
		ImGui::Separator();
		if (ImGui::MenuItem("Exit", "Alt+F4")) {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Scene"))
	{
		if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
			NewScene();
		}
		if (ImGui::MenuItem("Open Scene", "Ctrl+O"))
		{
			std::string path{};
			if (fileService.OpenFileDialog(Editor::GetInstance().GetConfig().project_workingDir.c_str(), path, FileService::FILE_TYPE_LIST{ {L"Scene Files", L"*.scene"} }))
			{
				LoadScene(path.c_str());
				glfwSetWindowTitle(window, (Editor::GetInstance().GetConfig().workspace_name + " | " + std::filesystem::path{ path }.filename().string()).c_str());


			}
		}
		if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
		{

			std::string currentPath = Editor::GetInstance().GetConfig().project_workingDir;
			if (currentPath.empty()) {
				spdlog::error("No project loaded, cannot save scene!");
				return;
			}
			std::string scenePath{};
			fileService.SaveFileDialog(Editor::GetInstance().GetConfig().project_workingDir.c_str(), scenePath, FileService::FILE_TYPE_LIST{ {L"Scene Files", L"*.scene"} });
			if (!scenePath.empty()) {

				SaveScene(scenePath.c_str());
			}
			else
				spdlog::warn("No scene path specified, not saving.");


		}
		ImGui::MenuItem("Save Scene As", "Ctrl+Shift+S");


		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Edit"))
	{
		ImGui::MenuItem("Undo", "Ctrl+Z");
		ImGui::MenuItem("Redo", "Ctrl+Y");
		ImGui::Separator();
		ImGui::MenuItem("Preferences");
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("View")) {
		ImGui::MenuItem("Inspector", nullptr, &showInspector);
		ImGui::MenuItem("Scene Explorer", nullptr, &showSceneExplorer);
		ImGui::MenuItem("Profiler", nullptr, &showProfiler);
		ImGui::MenuItem("Console", nullptr, &showConsole);
		ImGui::MenuItem("Skybox Settings", nullptr, &showSkyboxSettings);
		ImGui::MenuItem("View Cube", nullptr, &showViewCube);

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("GameObject")) {
		if (ImGui::MenuItem("Create Empty")) {
			engineService.create_entity();
		}

		ImGui::Separator();

		if (ImGui::BeginMenu("3D Object")) {
			if (ImGui::MenuItem("Cube")) {
				CreateCube();
			}
			if (ImGui::MenuItem("Physics Cube")) {
				CreatePhysicsCube();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Lights")) {
			if (ImGui::MenuItem("Directional Light")) {
				CreateLightEntity();
			}
			if (ImGui::MenuItem("Point Light")) {

			}
			if (ImGui::MenuItem("Spot Light")) {

			}

			ImGui::EndMenu();
		}
		if (ImGui::MenuItem("Create Camera")) {
			CreateCameraEntity();
		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Help"))
	{
		if (ImGui::MenuItem("Dump Mono Reflection Tree"))
		{
			MonoReflectionRegistry::Instance().VisitAssemblies([](const AssemblyNode& assembly)
				{
					const std::string assemblyName = assembly.name.empty() ? std::string("<Unnamed>") : assembly.name;
					SPDLOG_INFO("[Reflection] Assembly: {}", assemblyName);

					for (const auto& [className, classNodePtr] : assembly.classes)
					{
						if (!classNodePtr)
						{
							continue;
						}

						const ClassNode& classNode = *classNodePtr;
						const MonoTypeDescriptor* classDescriptor = classNode.descriptor;
						const std::string classManaged = classDescriptor ? classDescriptor->managed_name : className;
						const std::string classCpp = classDescriptor && !classDescriptor->cpp_name.empty()
							? classDescriptor->cpp_name
							: std::string("<cpp unknown>");
						std::string classGuidStr;
						if (classDescriptor && classDescriptor->guid != rp::null_guid)
						{
							classGuidStr = classDescriptor->guid.to_hex();
						}
						std::string classExtra;
						if (!classGuidStr.empty())
						{
							classExtra += " | guid=";
							classExtra += classGuidStr;
						}
						SPDLOG_INFO("[Reflection]   Class: {} | managed=\"{}\" | cpp=\"{}\"{}", className, classManaged, classCpp, classExtra);

						for (const auto& [fieldName, fieldNodePtr] : classNode.fields)
						{
							if (!fieldNodePtr)
							{
								continue;
							}

							const FieldNode& fieldNode = *fieldNodePtr;
							if (!fieldNode.isPublic)
							{
								continue;
							}

							const MonoTypeDescriptor* fieldDescriptor = fieldNode.descriptor;
							const std::string fieldManaged = fieldDescriptor ? fieldDescriptor->managed_name : fieldNode.type;
							const std::string fieldCpp = fieldDescriptor && !fieldDescriptor->cpp_name.empty()
								? fieldDescriptor->cpp_name
								: std::string("<cpp unknown>");
							std::string fieldGuidStr;
							if (fieldDescriptor && fieldDescriptor->guid != rp::null_guid)
							{
								fieldGuidStr = fieldDescriptor->guid.to_hex();
							}
							std::string fieldExtra;
							if (fieldNode.isStatic)
							{
								fieldExtra += " [static]";
							}
							if (!fieldGuidStr.empty())
							{
								if (!fieldExtra.empty())
								{
									fieldExtra += " ";
								}
								fieldExtra += "| guid=";
								fieldExtra += fieldGuidStr;
							}
							SPDLOG_INFO("[Reflection]     Field: {} -> managed=\"{}\" | cpp=\"{}\"{}", fieldName, fieldManaged, fieldCpp, fieldExtra);
							if (fieldDescriptor && fieldDescriptor->elementDescriptor)
							{
								const MonoTypeDescriptor* element = fieldDescriptor->elementDescriptor;
								const std::string elementManaged = element->managed_name;
								const std::string elementCpp = !element->cpp_name.empty() ? element->cpp_name : std::string("<cpp unknown>");
								std::string elementGuidStr;
								if (element->guid != rp::null_guid)
								{
									elementGuidStr = element->guid.to_hex();
								}
								std::string elementExtra;
								if (!elementGuidStr.empty())
								{
									elementExtra += " | guid=";
									elementExtra += elementGuidStr;
								}
								SPDLOG_INFO("[Reflection]       Element -> managed=\"{}\" | cpp=\"{}\"{}", elementManaged, elementCpp, elementExtra);
							}
						}
					}
				});
		}

		ImGui::MenuItem("Documentation");
		if (ImGui::MenuItem("About"))
		{
			showAboutModal = true;
		}
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();

	Render_AboutUI();


}
