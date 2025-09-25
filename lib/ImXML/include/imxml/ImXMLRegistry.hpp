#ifndef ImXML_REGISTRY_HPP
#define ImXML_REGISTRY_HPP
#include <string>
#include <unordered_map>

#include "pugixml.hpp"


class IWidget;

namespace ImXML
{
	class ImXMLRegistry
	{
	public:
		enum PROPERTY_TYPE
		{
			PT_INT,
			PT_FLOAT,
			PT_STRING,
			PT_BOOL,
			PT_NONE
		};
		using PROPERTY_LIST = std::unordered_map<std::string, PROPERTY_TYPE>;
		using WIDGET_LIST = std::unordered_map<std::string, IWidget*>;

		std::unordered_map<std::string, PROPERTY_LIST> TagSchema;

		WIDGET_LIST Widgets;



		static  ImXMLRegistry& GetInstance();


		void LoadSchema(const char* schema_path);
		PROPERTY_TYPE QueryProperty(const char* tag, const char* property);
		std::unordered_map<std::string, PROPERTY_TYPE> GetProperties(const char* tag) const;
		bool RegistryLoaded() const;


		void AddWidget(std::string, IWidget*);
		bool WidgetExist(std::string) const;
	private:
		pugi::xml_document schema_doc;
		bool registry_loaded = false;



	};

}


#endif // IMXML_REGISTRY_HPP