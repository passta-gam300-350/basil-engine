#ifndef MANAGEDSCENE_HPP
#define MANAGEDSCENE_HPP
#include <string>
#include <memory>
typedef struct _MonoString MonoString;

class ManagedScene
{
public:
	static void LoadSceneIndex(int32_t index);
	static void LoadSceneName(MonoString* name);

};

#endif //!MANAGEDSCENE_HPP