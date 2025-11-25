#include"Screens/EditorMain.hpp"
#include "Manager/ResourceSystem.hpp"

void EditorMain::Render_AssetBrowser()
{
	// Padding Variables: Asset Grid/List
	static float thumbnailSize = 72.0f;
	static float padding = thumbnailSize * 0.1f; // Unity-like: padding is 10% of thumbnail size
	static bool initialized = false;
	if (!initialized) {
		padding = thumbnailSize * 0.1f;
		initialized = true;
	}

	ImGui::Begin("Asset Browser", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	// Calculate heights for all sections to ensure everything fits
	float toolbarHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y * 2;
	float statusBarHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y * 2;
	float itemSpacing = ImGui::GetStyle().ItemSpacing.y;

	// === TOOLBAR ===
	ImGui::BeginChild("AssetToolbar", ImVec2(0, toolbarHeight), true, ImGuiWindowFlags_NoScrollbar);
	{
		// Search bar
		ImGui::SetNextItemWidth(200.0f);
		ImGui::InputTextWithHint("##AssetSearch", "Search assets...", m_AssetSearchBuffer, sizeof(m_AssetSearchBuffer));

		ImGui::SameLine();

		// View mode toggle
		ImGui::Text("|");
		ImGui::SameLine();
		if (ImGui::Button(m_AssetViewMode == AssetViewMode::Grid ? "Grid View" : "List View"))
		{
			m_AssetViewMode = (m_AssetViewMode == AssetViewMode::Grid) ? AssetViewMode::List : AssetViewMode::Grid;
		}

		ImGui::SameLine();

		// Create menu
		if (ImGui::Button("Create"))
		{
			ImGui::OpenPopup("CreateAssetPopup");
		}

		if (ImGui::BeginPopup("CreateAssetPopup"))
		{
			if (ImGui::MenuItem("Material"))
			{
				m_ShowCreateMaterialDialog = true;
			}
			ImGui::EndPopup();
		}
	}
	ImGui::EndChild();

	// === MAIN CONTENT AREA (Split panel) ===
	static float folderTreeWidth = 200.0f;

	// Leave space for status bar and spacing
	ImGui::BeginChild("AssetContent", ImVec2(0, -(statusBarHeight + itemSpacing)));
	{
		// LEFT PANEL: Folder Tree
		ImGui::BeginChild("FolderTree", ImVec2(m_FolderTreeWidth, 0), true);
		{
			ImGui::Text("Folders");
			ImGui::Separator();

			// Recursive folder tree helper
			std::function<void(const std::string&)> RenderFolderTree = [&](const std::string& path)
				{
					std::filesystem::path fsPath(path);
					std::string folderName = (path == m_AssetManager->GetRootPath()) ? "Assets" : fsPath.filename().string();

					bool isCurrentPath = (path == m_AssetManager->GetCurrentPath());

					// Get subdirectories for this path
					std::vector<std::string> subdirs;
					try {
						for (const auto& entry : std::filesystem::directory_iterator(path))
						{
							if (entry.is_directory())
							{
								subdirs.push_back(entry.path().string());
							}
						}
					}
					catch (...) {}

					ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
					if (subdirs.empty())
						flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
					if (isCurrentPath)
						flags |= ImGuiTreeNodeFlags_Selected;

					bool nodeOpen = ImGui::TreeNodeEx(folderName.c_str(), flags);

					// Click to navigate
					if (ImGui::IsItemClicked())
					{
						m_AssetManager->SetCurrentPath(path);
						m_SelectedAssetPath = "";
					}

					if (nodeOpen && !subdirs.empty())
					{
						for (const auto& subdir : subdirs)
						{
							RenderFolderTree(subdir);
						}
						ImGui::TreePop();
					}
				};

			RenderFolderTree(m_AssetManager->GetRootPath());
		}
		ImGui::EndChild();

		// Splitter
		ImGui::SameLine();
		ImGui::Button("##splitter", ImVec2(4.0f, -1));
		if (ImGui::IsItemActive())
		{
			m_FolderTreeWidth += ImGui::GetIO().MouseDelta.x;
			m_FolderTreeWidth = glm::clamp(m_FolderTreeWidth, 100.0f, 400.0f);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
		}
		ImGui::SameLine();

		// RIGHT PANEL: Asset Grid/List
		ImGui::BeginChild("AssetGrid", ImVec2(0, 0), true);
		{

			// === BREADCRUMB NAVIGATION ===
			std::filesystem::path currentPath = m_AssetManager->GetCurrentPath();
			std::filesystem::path rootPath = m_AssetManager->GetRootPath();

			// Build path segments
			std::vector<std::filesystem::path> pathSegments;
			std::filesystem::path tempPath = currentPath;

			while (tempPath != rootPath && !tempPath.empty())
			{
				pathSegments.insert(pathSegments.begin(), tempPath);
				tempPath = tempPath.parent_path();
			}
			pathSegments.insert(pathSegments.begin(), rootPath);

			// Render clickable breadcrumbs
			for (size_t i = 0; i < pathSegments.size(); ++i)
			{
				if (i > 0)
				{
					ImGui::SameLine();
					ImGui::TextDisabled(">");
					ImGui::SameLine();
				}

				std::string segmentName = (i == 0) ? "Assets" : pathSegments[i].filename().string();

				if (ImGui::SmallButton(segmentName.c_str()))
				{
					// Navigate to this path
					m_AssetManager->SetCurrentPath(pathSegments[i].string());
				}
			}

			std::vector<std::string> subdirs = m_AssetManager->GetSubDirectories();
			auto files = m_AssetManager->GetFiles(m_AssetManager->GetCurrentPath());

			// Filter by search
			std::string searchFilter = m_AssetSearchBuffer;
			std::transform(searchFilter.begin(), searchFilter.end(), searchFilter.begin(), ::tolower);

			bool mClickDouble = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

			if (m_AssetViewMode == AssetViewMode::Grid)
			{
				// === GRID VIEW ===
				float cellSize = thumbnailSize + padding;
				float panelWidth = ImGui::GetContentRegionAvail().x;
				int columns = static_cast<int>(panelWidth / cellSize);
				if (columns <= 0) columns = 1;

				ImGui::Columns(columns, 0, false);

				// Render folders first
				for (const std::string& subd : subdirs)
				{
					std::filesystem::path subpath{ subd };
					std::string folderName = subpath.filename().string();

					// Search filter
					if (!searchFilter.empty())
					{
						std::string lowerName = folderName;
						std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
						if (lowerName.find(searchFilter) == std::string::npos)
							continue;
					}

					ImGui::PushID(folderName.c_str());

					bool isSelected = (m_SelectedAssetPath == subd);

					// Draw selection border if selected
					if (isSelected)
					{
						ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.7f, 1.0f, 1.0f));
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
					}

					// Use icon if available, otherwise fallback to button with text
					if (m_AssetIcons.folderIcon != 0)
					{
						// Draw icon as button
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0)); // Transparent button
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.3f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));

						if (ImGui::ImageButton(folderName.c_str(), (ImTextureID)(uintptr_t)m_AssetIcons.folderIcon,
							ImVec2(thumbnailSize, thumbnailSize), ImVec2(0, 0), ImVec2(1, 1)))
						{
							m_SelectedAssetPath = subd;
						}

						ImGui::PopStyleColor(3);
					}
					else
					{
						// Fallback: colored button with text
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.7f, 0.3f, 0.4f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.8f, 0.4f, 0.6f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.9f, 0.5f, 0.8f));

						if (ImGui::Button("[ FOLDER ]", { thumbnailSize, thumbnailSize }))
						{
							m_SelectedAssetPath = subd;
						}

						ImGui::PopStyleColor(3);
					}

					if (isSelected)
					{
						ImGui::PopStyleVar();
						ImGui::PopStyleColor();
					}

					// Double-click to open
					if (ImGui::IsItemHovered() && mClickDouble)
					{
						m_AssetManager->GoToSubDirectory(subd);
						m_SelectedAssetPath = "";
					}

					// Right-click context menu
					if (ImGui::BeginPopupContextItem())
					{
						if (ImGui::MenuItem("Import All"))
						{
							auto biguids{ m_AssetManager->ImportAssetDirectory(subd) };
							engineService.ExecuteOnEngineThread([biguids] {
								std::for_each(biguids.begin(), biguids.end(), [](rp::BasicIndexedGuid biguid) {
									ResourceRegistry::Instance().Unload(biguid);
									});
								});
						}
						ImGui::EndPopup();
					}

					// Clip filename if too long instead of wrapping
					ImGui::Text("%s", folderName.c_str());
					ImGui::NextColumn();
					ImGui::PopID();
				}

				// Render files
				static bool ShowImportSettingsMenu = false;
				for (auto it = files.first; it != files.second; ++it)
				{
					std::filesystem::path filepath{ it->second };
					std::string filename = filepath.filename().string();

					if (filename.empty()) continue;

					// Search filter
					if (!searchFilter.empty())
					{
						std::string lowerName = filename;
						std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
						if (lowerName.find(searchFilter) == std::string::npos)
							continue;
					}

					ImGui::PushID(filename.c_str());

					// Skip descriptor files (.Desc extension)
					std::string ext = filepath.extension().string();

					bool isSelected = (m_SelectedAssetPath == it->second);

					// Draw selection border if selected
					if (isSelected)
					{
						ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.7f, 1.0f, 1.0f));
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
					}

					// Get appropriate icon for file type
					unsigned int fileIcon = GetIconForFile(ext);

					// Use icon if available, otherwise fallback to button with text
					if (fileIcon != 0)
					{
						// Draw icon as button
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0)); // Transparent button
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.3f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));

						if (ImGui::ImageButton(filename.c_str(), (ImTextureID)(uintptr_t)fileIcon,
							ImVec2(thumbnailSize, thumbnailSize), ImVec2(0, 0), ImVec2(1, 1)))
						{
							m_SelectedAssetPath = it->second;
						}

						ImGui::PopStyleColor(3);
					}
					else
					{
						// Fallback: button with file extension as text
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.4f, 0.4f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.6f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 0.6f, 0.8f));

						std::string buttonLabel = ext.empty() ? "[ FILE ]" : ext;
						if (ImGui::Button(buttonLabel.c_str(), { thumbnailSize, thumbnailSize }))
						{
							m_SelectedAssetPath = it->second;
						}

						ImGui::PopStyleColor(3);
					}

					if (isSelected)
					{
						ImGui::PopStyleVar();
						ImGui::PopStyleColor();
					}

					// Right-click context menu
					if (ImGui::BeginPopupContextItem())
					{
						if (ImGui::MenuItem("Import Asset"))
						{
							rp::BasicIndexedGuid biguid{ m_AssetManager->ImportAsset(it->second) };
							engineService.ExecuteOnEngineThread([biguid] {
								ResourceRegistry::Instance().Unload(biguid);
								});
						}
						if (ImGui::MenuItem("Import Settings"))
						{
							m_AssetManager->LoadImportSettings(it->second);
							ShowImportSettingsMenu = true;
						}
						ImGui::EndPopup();
					}

					if (ShowImportSettingsMenu)
					{
						ImGui::OpenPopup("DescriptorInspector");
						ShowImportSettingsMenu = false;
					}
					Render_ImporterSettings();

					// Drag and drop
					if (ImGui::BeginDragDropSource())
					{
						std::string itemPath = filepath.string();
						char AssetPayload[] = "AssetDrop";
						ImGui::SetDragDropPayload(AssetPayload, itemPath.c_str(), strlen(itemPath.c_str()) + 1);
						ImGui::EndDragDropSource();
					}

					// Clip filename if too long instead of wrapping
					ImGui::Text("%s", filename.c_str());
					ImGui::NextColumn();
					ImGui::PopID();
				}

				ImGui::Columns(1);
			}
			else
			{
				// === LIST VIEW ===
				ImGui::Columns(3);
				ImGui::Separator();
				ImGui::Text("Name"); ImGui::NextColumn();
				ImGui::Text("Type"); ImGui::NextColumn();
				ImGui::Text("Path"); ImGui::NextColumn();
				ImGui::Separator();

				// Folders
				for (const std::string& subd : subdirs)
				{
					std::filesystem::path subpath{ subd };
					std::string folderName = subpath.filename().string();

					if (!searchFilter.empty())
					{
						std::string lowerName = folderName;
						std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
						if (lowerName.find(searchFilter) == std::string::npos)
							continue;
					}

					ImGui::PushID(folderName.c_str());

					bool isSelected = (m_SelectedAssetPath == subd);
					if (ImGui::Selectable(folderName.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
					{
						m_SelectedAssetPath = subd;
					}

					if (ImGui::IsItemHovered() && mClickDouble)
					{
						m_AssetManager->GoToSubDirectory(subd);
						m_SelectedAssetPath = "";
					}

					if (ImGui::BeginPopupContextItem())
					{
						if (ImGui::MenuItem("Import All"))
						{
							auto biguids{ m_AssetManager->ImportAssetDirectory(subd) };
							engineService.ExecuteOnEngineThread([biguids] {
								std::for_each(biguids.begin(), biguids.end(), [](rp::BasicIndexedGuid biguid) {
									ResourceRegistry::Instance().Unload(biguid);
									});
								});
						}
						ImGui::EndPopup();
					}

					ImGui::NextColumn();
					ImGui::Text("Folder"); ImGui::NextColumn();
					ImGui::TextDisabled("%s", subpath.parent_path().string().c_str()); ImGui::NextColumn();

					ImGui::PopID();
				}

				// Files
				static bool ShowImportSettingsMenu = false;
				for (auto it = files.first; it != files.second; ++it)
				{
					std::filesystem::path filepath{ it->second };
					std::string filename = filepath.filename().string();

					if (filename.empty()) continue;

					if (!searchFilter.empty())
					{
						std::string lowerName = filename;
						std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
						if (lowerName.find(searchFilter) == std::string::npos)
							continue;
					}

					ImGui::PushID(filename.c_str());

					bool isSelected = (m_SelectedAssetPath == it->second);
					if (ImGui::Selectable(filename.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
					{
						m_SelectedAssetPath = it->second;
					}

					if (ImGui::BeginPopupContextItem())
					{
						if (ImGui::MenuItem("Import Asset"))
						{
							rp::BasicIndexedGuid biguid{ m_AssetManager->ImportAsset(it->second) };
							engineService.ExecuteOnEngineThread([biguid] {
								ResourceRegistry::Instance().Unload(biguid);
								});
						}
						if (ImGui::MenuItem("Import Settings"))
						{
							m_AssetManager->LoadImportSettings(it->second);
							ShowImportSettingsMenu = true;
						}
						ImGui::EndPopup();
					}

					if (ShowImportSettingsMenu)
					{
						ImGui::OpenPopup("DescriptorInspector");
						ShowImportSettingsMenu = false;
					}
					Render_ImporterSettings();

					if (ImGui::BeginDragDropSource())
					{
						std::string itemPath = filepath.string();
						char AssetPayload[] = "AssetDrop";
						ImGui::SetDragDropPayload(AssetPayload, itemPath.c_str(), strlen(itemPath.c_str()) + 1);
						ImGui::EndDragDropSource();
					}

					ImGui::NextColumn();
					std::string ext = filepath.extension().string();
					ImGui::Text("%s", ext.c_str()); ImGui::NextColumn();
					ImGui::TextDisabled("%s", filepath.parent_path().string().c_str()); ImGui::NextColumn();

					ImGui::PopID();
				}

				ImGui::Columns(1);
			}

			// Context menu for empty space
			if (ImGui::BeginPopupContextWindow("AssetBrowserContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
			{
				if (ImGui::BeginMenu("Create"))
				{
					if (ImGui::MenuItem("Material"))
					{
						m_ShowCreateMaterialDialog = true;
					}
					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}
		}
		ImGui::EndChild();
	}
	ImGui::EndChild();

	// === STATUS BAR ===
	ImGui::BeginChild("StatusBar", ImVec2(0, statusBarHeight), true, ImGuiWindowFlags_NoScrollbar);
	{
		// Left side: Item count or selection info
		if (!m_SelectedAssetPath.empty())
		{
			std::filesystem::path selectedPath(m_SelectedAssetPath);
			ImGui::Text("Selected: %s", selectedPath.filename().string().c_str());
		}
		else
		{
			auto files = m_AssetManager->GetFiles(m_AssetManager->GetCurrentPath());
			int fileCount = static_cast<int>(std::distance(files.first, files.second));
			int folderCount = static_cast<int>(m_AssetManager->GetSubDirectories().size());
			ImGui::Text("%d items (%d folders, %d files)", fileCount + folderCount, folderCount, fileCount);
		}

		// Right side: Thumbnail size slider (only in Grid view)
		if (m_AssetViewMode == AssetViewMode::Grid)
		{
			ImGui::SameLine(ImGui::GetWindowWidth() - 250); // Position on right side
			ImGui::SetNextItemWidth(200);
			if (ImGui::SliderFloat("##ThumbnailSize", &thumbnailSize, 32.0f, 256.0f, "Size: %.0f"))
			{
				// Auto-adjust padding based on thumbnail size (Unity-like behavior)
				// Padding is approximately 10% of thumbnail size
				padding = thumbnailSize * 0.1f;
			}
		}
	}
	ImGui::EndChild();

	// === MATERIAL CREATION DIALOG ===
	if (m_ShowCreateMaterialDialog)
	{
		ImGui::OpenPopup("Create Material");
		m_ShowCreateMaterialDialog = false;
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Create Material", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Enter material name:");
		ImGui::Separator();

		ImGui::InputText("##MaterialName", m_NewMaterialNameBuffer, sizeof(m_NewMaterialNameBuffer));

		ImGui::Separator();

		if (ImGui::Button("Create", ImVec2(120, 0)))
		{
			if (strlen(m_NewMaterialNameBuffer) > 0)
			{
				m_AssetManager->CreateMaterialDescriptor(std::string(m_NewMaterialNameBuffer));
				ImGui::CloseCurrentPopup();
				strcpy(m_NewMaterialNameBuffer, "NewMaterial");
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
			strcpy(m_NewMaterialNameBuffer, "NewMaterial");
		}

		ImGui::EndPopup();
	}

	ImGui::End();

	// === RESOURCES WINDOW (Imported Assets) ===
	ImGui::Begin("Resources");
	{
		static float resPadding = 8.0f;
		static float resThumbnailSize = 72.0f;
		float resCellSize = resThumbnailSize + resPadding;
		float resPanelWidth = ImGui::GetContentRegionAvail().x;
		int resColumns = static_cast<int>(resPanelWidth / resCellSize);
		if (resColumns <= 0) resColumns = 1;

		ImGui::Columns(resColumns, 0, false);

		for (auto [assetname, guid] : m_AssetManager->m_AssetNameGuid)
		{
			ImGui::PushID(assetname.c_str());
			ImGui::Button(assetname.c_str(), { resThumbnailSize, resThumbnailSize });
			ImGui::TextWrapped("%s", assetname.c_str());
			ImGui::NextColumn();
			ImGui::PopID();
		}

		ImGui::Columns(1);
	}
	ImGui::End();
}
