#ifndef CSKlass_HPP
#define CSKlass_HPP

#include <string>
#include <string_view>
#include <unordered_map>

#include <mono/utils/mono-forward.h>
#include <mono/metadata/image.h>
#include <mono/metadata/class.h>
#include <mono/metadata/object.h>

struct CSKlassInstance;

struct CSKlass
{
	struct FieldInfo
	{
		MonoClassField* field;
		MonoType* type;
		const char* name;
		unsigned offset;

	};


	std::vector<FieldInfo> m_FieldInfos;

	CSKlass() = default;
	CSKlass(MonoImage* image, std::string_view namespaceName, std::string_view className);

	bool Bind(MonoImage* image, std::string_view namespaceName, std::string_view className);
	void Reset() noexcept;

	[[nodiscard]] bool IsValid() const noexcept;
	[[nodiscard]] MonoClass* Klass() const noexcept;
	[[nodiscard]] MonoImage* Image() const noexcept;
	[[nodiscard]] std::string_view Namespace() const noexcept;
	[[nodiscard]] std::string_view Name() const noexcept;
	[[nodiscard]] bool IsDerivedFrom(const char* baseClassFullName) const;
	[[nodiscard]] bool IsDerivedFrom(const CSKlass& baseClass) const;
	MonoObject* Invoke(const char* methodName, void** args = nullptr, MonoObject** exception = nullptr, int paramCount = -1) const;

	CSKlassInstance CreateInstance(MonoDomain* domain = nullptr, void* args[] = nullptr) const;
	CSKlassInstance CreateNativeInstance(MonoDomain* domain = nullptr) const;


	MonoMethod* GetMethod(const char* methodName, int paramCount = -1) const;

	FieldInfo* ResolveField(const char* fieldName);
	FieldInfo* MakeField(MonoClassField* field);
	
private:
	MonoMethod* ResolveMethod(const char* methodName, int paramCount) const;
	std::string BuildCacheKey(const char* methodName, int paramCount) const;

	MonoImage* m_image{ nullptr };
	MonoClass* m_class{ nullptr };
	std::string m_namespace;
	std::string m_name;
	mutable std::unordered_map<std::string, MonoMethod*> m_methodCache;
};

struct CSKlassInstance
{
	CSKlassInstance() = default;
	explicit CSKlassInstance(const CSKlass* klass, bool isNative=false);
	CSKlassInstance(const CSKlass* klass, MonoObject* instance, bool isNative=false);

	void Attach(const CSKlass* klass, MonoObject* instance, bool isNative=false);
	void Reset() noexcept;

	[[nodiscard]] bool IsValid() const noexcept;
	[[nodiscard]] const CSKlass* Klass() const noexcept;
	[[nodiscard]] MonoObject* Object() const noexcept;
	 void UpdateManaged() noexcept;

	MonoObject* Invoke(const char* methodName, void** args = nullptr, MonoObject** exception = nullptr, int paramCount = -1) const;

	
private:

	const CSKlass* m_klass{ nullptr };
	uint32_t m_instanceHandle{ 0 };
	MonoObject* m_instance{ nullptr };
};


#endif
