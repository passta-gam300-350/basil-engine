using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using Engine.Bindings;

namespace BasilEngine.SceneManagement
{
    [NativeHeader("Bindings/ManagedScene.hpp")]
    [NativeClass("ManagedScene")]


    public class Scene
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("LoadSceneName")]
        [StaticAccessor("ManagedScene", StaticAccessorType.DoubleColon)]
        public extern static void LoadSceneByName(string sceneName);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("LoadSceneIndex")]
        [StaticAccessor("ManagedScene", StaticAccessorType.DoubleColon)]
        public extern static void LoadSceneByIndex(int index);



        public static void LoadScene(int index)
        {
            LoadSceneByIndex(index);
        }

        public static void LoadScene(string name)
        {
            LoadSceneByName(name);
        }

    }
}
