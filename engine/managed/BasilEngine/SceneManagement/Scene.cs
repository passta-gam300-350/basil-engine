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


    /// <summary>
    /// Provides scene loading helpers that forward to the native scene manager.
    /// </summary>
    public class Scene
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("LoadSceneName")]
        [StaticAccessor("ManagedScene", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Loads a scene by name.
        /// </summary>
        /// <param name="sceneName">Name of the scene asset.</param>
        public extern static void LoadSceneByName(string sceneName);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("LoadSceneIndex")]
        [StaticAccessor("ManagedScene", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Loads a scene by build index.
        /// </summary>
        /// <param name="index">Scene index.</param>
        public extern static void LoadSceneByIndex(int index);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("ExitApplication")]
        [StaticAccessor("ManagedScene", StaticAccessorType.DoubleColon)]
        public extern static void ExitApplication();

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetCurrentSceneIndex")]
        [StaticAccessor("ManagedScene", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Gets the build index of the currently active scene.
        /// </summary>
        /// <returns>Build index of the currently active scene.</returns>   
        public extern static int GetCurrentSceneIndexInternal();

        /// <summary>
        /// Loads a scene by build index.
        /// </summary>
        /// <param name="index">Scene index.</param>
        

        public static int CurrentIndex => GetCurrentSceneIndexInternal();
        public static void LoadScene(int index)
        {
            LoadSceneByIndex(index);
        }

        /// <summary>
        /// Loads a scene by name.
        /// </summary>
        /// <param name="name">Scene name.</param>
        public static void LoadScene(string name)
        {
            LoadSceneByName(name);
        }

        public static void Quit()
        {
            ExitApplication();
        }


        public static void Restart()
        {
            LoadSceneByIndex(GetCurrentSceneIndexInternal());
        }



    }
}
