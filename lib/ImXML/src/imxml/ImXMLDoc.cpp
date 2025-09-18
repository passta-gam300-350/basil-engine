#include "imxml/ImXMLNode.hpp"
#include "imxml/ImXMLDoc.hpp"

#include <iostream>
#include <queue>
#include <stack>

#include "imgui.h"
#include "imxml/ImXMLRegistry.hpp"
#include "imxml/widget/IWidget.hpp"



ImXML::ImXMLDoc::ImXMLDoc(const char* path) : root{}
{
	Parse(path);

	if (!root)
	{
		throw std::exception{ "Failed to parse XML document or document is empty" };
	}
	for (auto n = root->first_child(this); n != nullptr; n = n->next_sibling(this))
	{
		std::cout << "Child: " << n->name() << std::endl;
	}
}



void ImXML::ImXMLDoc::Parse(const char* path)
{
	pugi::xml_parse_result stats = doc.load_file(path);

	if (!stats)
	{
		printf("XML [%s] parsed with errors, attr value: [%s]\n", path, doc.child("node").attribute("attr").value());
		printf("Error description: %s\n", stats.description());
		printf("Error offset: %d\n", (int)stats.offset);
	}

	size_t counter = 0;


	ImXMLNode root_node = CreateNode(doc.first_child());

	node_storage.push_back(root_node);

	root = node_storage.data();

	counter++;
	size_t nChild{};
	ParseChild(0, node_storage, counter, nChild);

	root = node_storage.data();

}

void ImXML::ImXMLDoc::ParseChild(size_t root_index, std::vector<ImXMLNode>& storage, size_t& count, size_t& child_cnt)
{
	ImXMLNode& node = storage[root_index];
	size_t cnt{ };

	pugi::xml_node child_one = node.get_native_node().first_child();

	if (child_one)
	{
		node.set_child_index(count);
	}

	size_t num_siblings{  };
	for (pugi::xml_node siblings = child_one; siblings;)
	{
		num_siblings++;
		ImXMLNode sb = CreateNode(siblings);
		storage.emplace_back(sb);

		ImXMLNode& sb_ref = storage[count];


		size_t topIndex = count;
		child_cnt += cnt;

		ParseChild(topIndex, storage, ++count, child_cnt);

		/*
		 *<ROOT>
		 *	<Sibling1>
		 *		<Child1/>
		 *		<Child2/>
		 *	</Sibling1>
		 *	<Sibling2>
		 *		<Child1/>
		 *		<Child2/>
		 *	</Sibling2>
		 *</ROOT>
		 */


		pugi::xml_node next = siblings.next_sibling();

		if (next)
		{
			storage[topIndex].set_sibling_index(count);

		}

		siblings = next;



	}


	storage[root_index].set_child_count(num_siblings);


}




ImXML::ImXMLNode* ImXML::ImXMLDoc::GetRoot()
{
	return root;
}


ImXML::ImXMLNode ImXML::ImXMLDoc::CreateNode(pugi::xml_node const& node)
{
	ImXMLNode n(node);

	std::string TagName = n.name();

	ImXMLRegistry& registry = ImXMLRegistry::GetInstance();

	// Get Attributes
	for (pugi::xml_attribute attr = node.first_attribute(); attr; attr = attr.next_attribute())
	{
		ImXMLRegistry::PROPERTY_TYPE type = registry.QueryProperty(TagName.c_str(), attr.name());
		std::string attrName = attr.name();
		switch (type)
		{
		case ImXMLRegistry::PT_INT:
			n.AddAttribute(attrName, AttributeValue{ attr.as_int() });
			break;
		case ImXMLRegistry::PT_FLOAT:
			n.AddAttribute(attrName, AttributeValue{ attr.as_float() });
			break;
		case ImXMLRegistry::PT_STRING:
			n.AddAttribute(attrName, AttributeValue{ attr.value() });
			break;
		case ImXMLRegistry::PT_BOOL:
			n.AddAttribute(attrName, AttributeValue{ attr.as_bool() });
			break;
		case ImXMLRegistry::PT_NONE:
			// Unknown property
			break;

		}




	}

	return n;

}


void ImXML::ImXMLDoc::RenderLevel(ImXMLNode* node, int level)
{
	if (!node) return;


	ImXMLNode& node_ref = *node;

	std::string name = node->name() + "##" + std::to_string(level) + std::to_string(reinterpret_cast<uintptr_t>(node));


	//bool isPoppable = false;
	//bool isOpen = false;

	if (node->Tag == ImXMLTag::Label)
	{
		// Render label-specific UI elements
		ImGui::Text("%s", (*node)["text"].getString());

	} else if (node->Tag == ImXMLTag::Button)
	{
		// Render button-specific UI elements
		if (ImGui::Button(node_ref["text"].getString()))
		{
			std::cout << "Button " << name << " clicked!" << std::endl;
		}
	}
	else if (node->Tag == ImXMLTag::Input)
	{
		// Render input-specific UI elements
		static char buffer[256];
		ImGui::InputText(name.c_str(), buffer, sizeof(buffer));
	}
	else if (node->Tag == ImXMLTag::Checkbox)
	{
		// Render checkbox-specific UI elements
		static bool checked = false;
		ImGui::Checkbox(name.c_str(), &checked);
	}
	else if (node->Tag == ImXMLTag::Slider)
	{
		// Render slider-specific UI elements
		static float value = 0.0f;
		ImGui::SliderFloat(name.c_str(), &value, 0.0f, 100.0f);
	}
	else if (node->Tag == ImXMLTag::Window)
	{
		bool isOpen = true;

		/*ImGui::SetNextWindowSize(ImVec2((*node)["width"].getInt(), (*node)["height"].getInt()));*/
		ImGui::Begin(node_ref["name"].getString(), &isOpen);
		
		if (node->hasChild())
			RenderLevel(node->first_child(this), level + 1);
		ImGui::End();
		
	} else if (node->Tag == ImXMLTag::Widget)
	{
		bool widgetExist = ImXMLRegistry::GetInstance().WidgetExist(name);
		if (widgetExist)
		{
			IWidget* widget = ImXMLRegistry::GetInstance().Widgets[name];
			widget->render(node);
		}
		
	}
	else if (node->Tag == ImXMLTag::ChildWindow)
	{
		bool active = ImGui::BeginChild(name.c_str());

		bool isOpen = active;
		if (isOpen)
		{
			
			if (node->hasChild())
				RenderLevel(node->first_child(this), level + 1);
		}

		ImGui::EndChild();
	}
	else
	{
		bool isOpen = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
		bool isPoppable = isOpen;
		if (isOpen)
		{
			isPoppable = true;
			if (node->hasChild())
				RenderLevel(node->first_child(this), level + 1);
		}
		if (isOpen && isPoppable)
		{
			ImGui::TreePop();
		}
	}


	
	ImXMLNode* sibling = node->next_sibling(this);
	if (sibling)
	{
		RenderLevel(sibling, level);
		
	}

	
}



ImXML::ImXMLNode& ImXML::ImXMLDoc::GetNode(int index)
{
	return node_storage[index];
}

ImXML::ImXMLDoc::~ImXMLDoc()
{
	// Cleanup
}


void ImXML::ImXMLDoc::Render()
{
	std::string rootName = root->name();
	
	ImXMLNode* curr = root;
	RenderLevel(curr, 0);

	

}


