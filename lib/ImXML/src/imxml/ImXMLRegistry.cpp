#include "imxml/ImXMLRegistry.hpp"
#include "imxml/widget/IWidget.hpp"
#include <iostream>


ImXML::ImXMLRegistry& ImXML::ImXMLRegistry::GetInstance()
{
	static ImXMLRegistry instance;
	return instance;

}

void ImXML::ImXMLRegistry::LoadSchema(const char* schema_path)
{
	// <Tag name="window">
	//  <Property name="width" type="int"/>
	//  <Property name="height" type="int"/>
	//  <Property name="title" type="string"/>
	// </Tag>

	registry_loaded = false;

	pugi::xml_parse_result result = schema_doc.load_file(schema_path);
	if (!result)
	{
		// Handle error
		return;
	}

	// Check Schema validation (must have a root tag "Schema")
	// <schema version="1.0">
	if (std::string(schema_doc.first_child().name()) != "schema")
	{
		throw std::exception{ "Invalid schema: Root element must be <schema>" };

	}

	pugi::xml_node const& root = schema_doc.first_child();

	// Get version attribute
	std::string version = root.attribute("version").as_string();
	std::cout << "Schema version: " << version << std::endl;
	// Iterate over tags
	for (pugi::xml_node tag = root.child("tag"); tag; tag = tag.next_sibling("tag"))
	{
		for (pugi::xml_attribute attr = tag.first_attribute(); attr; attr = attr.next_attribute())
		{
			std::cout << "Tag attribute: " << attr.name() << " = " << attr.value() << std::endl;
		}
		std::string tag_name = tag.attribute("name").as_string();
		PROPERTY_LIST properties;
		for (pugi::xml_node property = tag.child("attribute"); property; property = property.next_sibling("attribute"))
		{
			std::string property_name = property.attribute("name").as_string();
			std::string property_type = property.attribute("type").as_string();
			PROPERTY_TYPE pt = PROPERTY_TYPE::PT_NONE;
			if (property_type == "int")
			{
				pt = PROPERTY_TYPE::PT_INT;
			}
			else if (property_type == "float")
			{
				pt = PROPERTY_TYPE::PT_FLOAT;
			}
			else if (property_type == "string")
			{
				pt = PROPERTY_TYPE::PT_STRING;
			}
			else if (property_type == "bool")
			{
				pt = PROPERTY_TYPE::PT_BOOL;
			}

			std::cout << "  Property: " << property_name << " Type: " << property_type << std::endl;
			properties[property_name] = pt;
		}

		TagSchema[tag_name] = properties;

	}

	registry_loaded = true;

}





ImXML::ImXMLRegistry::PROPERTY_TYPE ImXML::ImXMLRegistry::QueryProperty(const char* tag, const char* property)
{
	auto tag_it = TagSchema.find(tag);
	if (tag_it != TagSchema.end())
	{
		auto prop_it = tag_it->second.find(property);
		if (prop_it != tag_it->second.end())
		{
			return prop_it->second;
		}
	}
	return PROPERTY_TYPE::PT_NONE;
}


std::unordered_map<std::string, ImXML::ImXMLRegistry::PROPERTY_TYPE> ImXML::ImXMLRegistry::GetProperties(const char* tag) const
{
	auto tag_it = TagSchema.find(tag);
	if (tag_it != TagSchema.end())
	{
		return tag_it->second;
	}
	return {};
}

bool ImXML::ImXMLRegistry::RegistryLoaded() const
{
	return registry_loaded;
}

bool ImXML::ImXMLRegistry::WidgetExist(std::string name) const
{
	return Widgets.contains(name);
}


void ImXML::ImXMLRegistry::AddWidget(std::string name, IWidget* widget)
{
	Widgets[name] = widget;
}




