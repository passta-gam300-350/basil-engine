/******************************************************************************/
/*!
\file   MonoImGuiRenderer.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the declaration for the MonoImGuiRenderer class, which
provides an interface for rendering ImGui elements for managed types in the editor.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef EDITOR_MANAGER_MONO_IMGUI_RENDERER_HPP
#define EDITOR_MANAGER_MONO_IMGUI_RENDERER_HPP

#include <string>


#include "Screens/EditorMain.hpp"

struct FieldNode;
struct MonoTypeDescriptor;
struct CSKlass;
struct CSKlassInstance;

class MonoImGuiRenderer
{
public:
	static void RenderBehaviourFields(const std::string& managedName, rp::Guid scriptGuid);

private:
	static bool RenderField(const FieldNode& fieldNode, CSKlass* klass, CSKlassInstance* instance);
	static CSKlass* ResolveManagedClass(const std::string& fullName);
	static void SplitManagedName(const std::string& fullName, std::string& namespaceName, std::string& className);
};

#endif // EDITOR_MANAGER_MONO_IMGUI_RENDERER_HPP
