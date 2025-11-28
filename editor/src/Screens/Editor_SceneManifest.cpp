#include "Editor.hpp"
#include "Scene/Scene.hpp"
#include "Screens/EditorMain.hpp"

struct SceneManifest
{
	std::string sceneName;
	std::string path;
	int index;
};

namespace
{
	std::vector<SceneManifest> sceneManifests = {};
	static bool onFirstRun = true;

}

void PopulateList(EngineContainerService& engineService)
{
	if (!onFirstRun) return;
	onFirstRun = false;
	
	auto fn = []()
	{
		std::vector<std::string> populationList;
		auto& Engine = Engine::Instance();
		Engine.GetSceneRegistry().GetManifest(populationList);
		if (populationList.empty()) return;
		for (size_t i = 0; i < populationList.size(); ++i)
		{
			std::filesystem::path fsPath(std::string{ Engine.getWorkingDir() } + "/" + populationList[i]);
			std::string abs_path = std::filesystem::absolute(fsPath).string();
			sceneManifests.push_back({ fsPath.stem().string(), abs_path, static_cast<int>(i) });
		}
	};

	engineService.ExecuteOnEngineThread(fn);
	
}

void EditorMain::Render_SceneManifest()
{
	PF_EDITOR_SCOPE("SceneManifest");
	PopulateList(engineService);



	ImGui::Begin("Scene Manifest");
	ImGui::Text("Settings to modify scene order and index");

	ImGui::PushID("ADD_SCENE_ID");
	if (ImGui::Button("Add Scene"))
	{
		std::string path;
		if (fileService.OpenFileDialog(Editor::GetInstance().GetConfig().project_workingDir.c_str(), path, FileService::FILE_TYPE_LIST{ {L"Scene Files", L"*.scene"} }))
		{
			std::filesystem::path fsPath(path);
			sceneManifests.push_back({ fsPath.stem().string(), path, static_cast<int>(sceneManifests.size()) });

		}

		//sceneManifests.push_back({"NewScene", "path/to/newscene", static_cast<int>(sceneManifests.size())});
	}
	ImGui::PopID();
	ImGui::SameLine();
	ImGui::PushID("CLEAR_SCENE_ID");
	if (ImGui::Button("Clear All"))
	{
		sceneManifests.clear();
	}
	ImGui::PopID();


	ImGui::BeginChild("SceneManifestChild", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

	for (size_t i = 0; i < sceneManifests.size(); ++i)
	{
		ImGui::PushID(static_cast<int>(i));
		ImGui::Text(sceneManifests[i].sceneName.c_str());
		// Tooltip for scene name
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Path: %s", sceneManifests[i].path.c_str());
		}
		ImGui::SameLine();
		if (ImGui::InputInt("Index", &sceneManifests[i].index))
		{
			auto& index = sceneManifests[i].index;
			if (index <= 0)
			{
				index = i;
			} else if (index >= static_cast<int>(sceneManifests.size()))
			{
				index = static_cast<int>(sceneManifests.size()) - 1;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove"))
		{
			sceneManifests.erase(sceneManifests.begin() + i);
			ImGui::PopID();
			break;
		}
		ImGui::PopID();
	}

	ImGui::EndChild();


	bool okPressed = ImGui::Button("OK");
	ImGui::SameLine();
	bool cancelPressed = ImGui::Button("Cancel");

	if (okPressed)
	{
		auto fn = []()
		{
			auto& Engine = Engine::Instance();

			std::vector<std::string> sceneOrder(sceneManifests.size());

			for (const auto& manifest : sceneManifests)
			{
				if (manifest.index >= 0 && manifest.index < static_cast<int>(sceneOrder.size()))
				{
					sceneOrder[manifest.index] = manifest.path;
				}
			}
			if (sceneOrder.empty()) return;
			Engine.GetSceneRegistry().GenerateManifest(sceneOrder);
		};
		engineService.ExecuteOnEngineThread(fn);
	}
	if (cancelPressed)
	{
		// Handle Cancel action
		// e.g., revert any changes made
	}

	ImGui::End();

}
