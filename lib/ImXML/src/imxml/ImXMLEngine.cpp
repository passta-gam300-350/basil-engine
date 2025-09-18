#include "imxml/ImXMLEngine.hpp"
#include "imxml/ImXMLDoc.hpp"
ImXML::ImXMLEngine& ImXML::ImXMLEngine::GetInstance()
{
	static ImXML::ImXMLEngine instance;
	return instance;
}

void ImXML::ImXMLEngine::Load(const char* filename)
{
	document = new ImXMLDoc{ filename };
}


void ImXML::ImXMLEngine::Render()
{
	if (document)
	{
		document->Render();
	}
}

ImXML::ImXMLEngine::~ImXMLEngine()
{
	if (document)
	{
		delete document;
		document = nullptr;
	}
}
