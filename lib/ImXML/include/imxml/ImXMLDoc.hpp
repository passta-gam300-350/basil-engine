#ifndef IMXML_DOC_HPP
#define IMXML_DOC_HPP
#include <unordered_map>

#include "pugixml.hpp"

namespace ImXML
{
	class ImXMLNode;
}

namespace ImXML
{

	class ImXMLDoc
	{
		pugi::xml_document doc;

		//std::unordered_map<pugi::xml_node, ImXMLNode> nodes;

		std::vector<ImXMLNode> node_storage;
		ImXMLNode* root;


		void ParseChild(size_t root_index, std::vector<ImXMLNode>& storage, size_t& count, size_t& child_cnt);
		void RenderLevel(ImXMLNode* node, int level);
	public:
		ImXMLDoc() = default;
		ImXMLDoc(const char* path);
		void Parse(const char* path);

		pugi::xml_document& GetDoc();
		pugi::xml_document const& GetDoc() const;



		std::vector<ImXMLNode*> GetChild(ImXMLNode& parent);

		ImXMLNode& GetNode(int index);

		ImXMLNode CreateNode(pugi::xml_node const& node);

		ImXMLNode* GetRoot();

		~ImXMLDoc();


		void Render();

	};

}

#endif // IMXML_DOC_HPP