#ifndef IMXMLENGINE_HPP
#define IMXMLENGINE_HPP

namespace ImXML
{
	class ImXMLDoc;
}

namespace ImXML
{
	class ImXMLEngine
	{
		ImXMLDoc* document = nullptr;
	public:
		static ImXMLEngine& GetInstance();

		void Load(const char* filename);


		void Render();

		~ImXMLEngine();


	};
}

#endif // IMXMLENGINE_HPP