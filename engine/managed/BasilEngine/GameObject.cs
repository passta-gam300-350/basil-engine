using Engine.Bindings;
using BasilEngine.Components;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.CompilerServices;
using System.Reflection.Emit;

namespace BasilEngine
{
    /// <summary>
    /// Represents a game object in the Basil Engine.
    /// </summary>
    //[Disabled]
    [NativeHeader("Bindings/ManagedGameObjects.hpp")]
    [NativeClass("ManagedGameObjects")]
    public class GameObject : NativeObject
    {
        // TODO: Add Glue Gen Attribues
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("FindByName")]
        [StaticAccessor("ManagedGameObjects", StaticAccessorType.DoubleColon)]
        private extern static UInt64 internal_Find(string name);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("DeleteGameObject")]
        [StaticAccessor("ManagedGameObjects", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Removes a native game object instance by handle.
        /// </summary>
        /// <param name="handle">Handle of the game object to delete.</param>
        public extern static void DeleteGameObjectInternal(UInt64 handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("SetVisible")]
        [StaticAccessor("ManagedGameObjects", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Sets the visibility flag for the game object.
        /// </summary>
        /// <param name="handle">Handle of the game object.</param>
        /// <param name="visible">True to render the object, false to hide it.</param>
        public extern static void SetVisibleInternal(UInt64 handle, bool visible);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetVisible")]
        [StaticAccessor("ManagedGameObjects", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Gets the current visibility state of the game object.
        /// </summary>
        /// <param name="handle">Handle of the game object.</param>
        /// <returns>True if the object is visible.</returns>
        public extern static bool GetVisibleInternal(UInt64 handle);






        private Transform transformComponent = null;

        /// <summary>
        /// Cached <see cref="Components.Transform"/> for this game object.
        /// </summary>
        public Transform transform
        {
            get
            {
                if (transformComponent == null)
                {
                    transformComponent = new Transform(NativeID);
                }

                return transformComponent;
            }
        }

        /// <summary>
        /// Controls whether the object is rendered.
        /// </summary>
        public bool visibility
        {
            get => GetVisibleInternal(NativeID);
            set => SetVisibleInternal(NativeID, value);
        }

        /// <summary>
        /// Indicates whether the object is marked active in the engine.
        /// </summary>
        public bool activeSelf
        {
            //TODO: Implement activeSelf getter
            get => false;
        }

        //TODO: Implement activeInHierarchy getter
        /// <summary>
        /// Indicates whether the object is active considering its parent hierarchy.
        /// </summary>
        public bool activeInHierarchy
        {
            get => false;
        }

        /// <summary>
        /// Indicates whether the object is treated as static by the engine.
        /// </summary>
        public bool isStatic
        {
            get => false;

        }

        //TODO: Implement layer getter and setter
        /// <summary>
        /// Rendering or collision layer assigned to this object.
        /// </summary>
        public int layer
        {
            get => 0;
            set { layer = value; }

        }





        /// <summary>
        /// Creates a managed wrapper for an existing native game object.
        /// </summary>
        /// <param name="handle">Native handle supplied by the engine.</param>
        public GameObject(UInt64 handle)
        {
            this.NativeID = handle;
            Console.WriteLine("GameObject created with handle: " + handle);
            transformComponent = new Transform(handle);
        }

        /// <summary>
        /// Finds a game object by name.
        /// </summary>
        /// <param name="name">Name of the game object to locate.</param>
        /// <returns>A managed <see cref="GameObject"/> wrapper or null if no match is found.</returns>
        public static GameObject Find(string name)
        {
            UInt64 handle = internal_Find(name);
            if (handle == 0)
            {
                return null;
            }

            return new GameObject(handle);


        }

        /// <summary>
        /// Destroys a game object instance.
        /// </summary>
        /// <param name="obj">Object to delete.</param>
        public static void Destroy(GameObject obj)
        {
            if (obj != null)
            {
                DeleteGameObjectInternal(obj.NativeID);
            }
        }
    }
}
         
