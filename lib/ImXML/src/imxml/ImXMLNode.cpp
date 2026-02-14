/******************************************************************************/
/*!
\file   ImXMLNode.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  ImXML node implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "imxml/ImXMLNode.hpp"

#include <iostream>

#include "imxml/ImXMLDoc.hpp"
ImXML::ImXMLNode::ImXMLNode(pugi::xml_node n)
{
	node = n;
	std::string node_name = node.name();

	if (node_name == "window") {
		Tag = ImXMLTag::Window;
	}
	else if (node_name == "button") {
		Tag = ImXMLTag::Button;
	}
	else if (node_name == "label") {
		Tag = ImXMLTag::Label;
	}
	else if (node_name == "input") {
		Tag = ImXMLTag::Input;
	}
	else if (node_name == "checkbox") {
		Tag = ImXMLTag::Checkbox;
	}
	else if (node_name == "slider") {
		Tag = ImXMLTag::Slider;
	}
	else if (node_name == "frame") {
		Tag = ImXMLTag::ChildWindow;
	}
	else
	{
		Tag = ImXMLTag::Widget;
	}

	// Get attributes
	for (pugi::xml_attribute attr = node.first_attribute(); attr; attr = attr.next_attribute())
	{
		
		
	}

	
}

std::string ImXML::ImXMLNode::name()
{
	return node.name();
}

std::string ImXML::ImXMLNode::name() const
{
	return node.name();
}


void ImXML::ImXMLNode::AddAttribute(std::string name, AttributeValue val)
{
	attributes[name] = val;
}





bool ImXML::AttributeValue::getBool() const
{
	return type == Type::Bool ? value.b : false;
}


const char* ImXML::AttributeValue::getString()const
{
	return type == Type::String ? value.s : "";
}

int ImXML::AttributeValue::getInt() const
{
	return type == Type::Int ? value.i : 0;
}

float ImXML::AttributeValue::getFloat() const
{
	return type == Type::Float ? value.f : 0.0f;
}

bool ImXML::ImXMLNode::hasChild()
{
	return children_index != UINT32_MAX || !node.first_child().empty();
}

bool ImXML::ImXMLNode::hasSibling()
{
	return next_siblings_index != UINT32_MAX || !node.next_sibling().empty();
}

ImXML::ImXMLNode* ImXML::ImXMLNode::next_sibling(ImXMLDoc* document)
{
	if (next_siblings_index == UINT32_MAX)
	{
		return nullptr;
	}
	return &document->GetNode(next_siblings_index);
}
ImXML::ImXMLNode* ImXML::ImXMLNode::first_child(ImXMLDoc* document)
{
	if (children_index == UINT32_MAX)
	{
		return nullptr;
	}
	return &document->GetNode(children_index);
}


pugi::xml_node ImXML::ImXMLNode::get_native_node()
{
	return node;
}


void ImXML::ImXMLNode::set_child_index(size_t index)
{
	children_index = static_cast<uint32_t>(index);
}

void ImXML::ImXMLNode::set_child_count(size_t index)
{
	child_count = static_cast<uint32_t>(index);
}


void ImXML::ImXMLNode::set_sibling_index(size_t index)
{
	next_siblings_index = static_cast<uint32_t>(index);
}


void ImXML::ImXMLNode::print() const
{
	node.print(std::cout);
}


ImXML::AttributeValue& ImXML::ImXMLNode::operator[](std::string const& key)
{
	return attributes[key];
}

const ImXML::AttributeValue& ImXML::ImXMLNode::operator[](std::string const& key) const
{
	return attributes.at(key);
}

std::unordered_map<std::string, ImXML::AttributeValue>& ImXML::ImXMLNode::GetAttributes()
{
	return attributes;
}

