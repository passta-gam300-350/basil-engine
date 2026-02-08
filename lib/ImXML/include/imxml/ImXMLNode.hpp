/******************************************************************************/
/*!
\file   ImXMLNode.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  ImXML node class

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef ImXML_NODE_HPP
#define ImXML_NODE_HPP

#include <unordered_map>

#include "pugixml.hpp"


namespace ImXML
{
	class ImXMLDoc;
}

namespace ImXML {

	struct AttributeValue
	{
		enum struct Type
		{
			None,
			Int,
			Float,
			String,
			Bool
		} type = Type::None;
		union Value
		{
			int i;
			float f;
			bool b;
			const char* s;
			Value() : i(0) {}
			Value(int val) : i(val) {}
			Value(float val) : f(val) {}
			Value(bool val) : b(val) {}
			Value(const char* val) : s(val) {}
		} value;
		AttributeValue() = default;
		AttributeValue(int val) : type(Type::Int), value(val) {}
		AttributeValue(float val) : type(Type::Float), value(val) {}
		AttributeValue(bool val) : type(Type::Bool), value(val) {}
		AttributeValue(const char* val) : type(Type::String), value(val) {}


		int getInt() const;
		float getFloat() const;
		bool getBool() const;
		const char* getString() const;


	};




	enum struct ImXMLTag
	{
		Window,
		Button,
		Label,
		Input,
		Checkbox,
		Slider,
		Widget,
		ChildWindow,
		None


	};

	class ImXMLNode
	{
		pugi::xml_node node;
		std::unordered_map<std::string, AttributeValue> attributes;

		uint32_t next_siblings_index= UINT32_MAX;
		uint32_t children_index = UINT32_MAX;
		uint32_t child_count = 0;

	public:
		ImXMLTag Tag = ImXMLTag::None;
		ImXMLNode() = default;
		ImXMLNode(pugi::xml_node n);


		bool hasChild();
		bool hasSibling();

		ImXMLNode* next_sibling(ImXMLDoc* document);
		ImXMLNode* first_child(ImXMLDoc* document);


		std::string name();
		std::string name() const;
		
		void AddAttribute(std::string name, AttributeValue val);

		pugi::xml_node get_native_node();


		void set_child_index(size_t index);
		void set_child_count(size_t index);
		void set_sibling_index(size_t index);

		void print() const;

		AttributeValue& operator[](const std::string& key);
		const AttributeValue& operator[](const std::string& key) const;

		std::unordered_map<std::string, AttributeValue>& GetAttributes();
	};




}

#endif // ImXML_NODE_HPP