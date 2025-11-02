#ifndef EDITOR_MANAGER_MONO_IMGUI_RENDERER_HPP
#define EDITOR_MANAGER_MONO_IMGUI_RENDERER_HPP

#include <string>
#include <serialisation/guid.h>

struct FieldNode;
struct MonoTypeDescriptor;
struct CSKlass;
struct CSKlassInstance;

class MonoImGuiRenderer
{
public:
	static void RenderBehaviourFields(const std::string& managedName, Resource::Guid scriptGuid);

private:
	static bool RenderField(const FieldNode& fieldNode, CSKlass* klass, CSKlassInstance* instance);
	static CSKlass* ResolveManagedClass(const std::string& fullName);
	static void SplitManagedName(const std::string& fullName, std::string& namespaceName, std::string& className);
};

#endif // EDITOR_MANAGER_MONO_IMGUI_RENDERER_HPP
