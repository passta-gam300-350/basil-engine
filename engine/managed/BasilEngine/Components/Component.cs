using Engine.Bindings;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using BasilEngine.Debug;

namespace BasilEngine.Components
{
    [NativeHeader("Bindings/ManagedComponents.hpp")]


    /// <summary>
    /// Base class for all attachable components that are driven by a native engine counterpart.
    /// </summary>
    public class Component : NativeObject
    {
        /// <summary>
        /// Creates a component wrapper and registers the component type with the native engine.
        /// </summary>
        protected Component()
        {
            string typeName = this.GetType().Name;
            typeID = Internal_RegisterComponent(typeName);
        }

        /// <summary>
        /// Creates a component wrapper bound to an existing native object.
        /// </summary>
        /// <param name="handle">Native object handle.</param>
        protected Component(UInt64 handle)
        {
            NativeID = handle; // GameObject Handle

        }


        /// <summary>
        /// Identifies a component type in the native engine.
        /// </summary>
        protected UInt32 typeID; // Identifies a component type in the native side.

        private Transform _transform = null;

        private GameObject _gameObject = null;

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("ManagedRegisterComponent")]
        [StaticAccessor("ManagedComponents", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Registers a managed component type with the native engine.
        /// </summary>
        /// <param name="name">Name of the component type.</param>
        /// <returns>Type identifier allocated by the engine.</returns>
        public extern static UInt32 Internal_RegisterComponent(string name);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("HasComponent")]
        [StaticAccessor("ManagedComponents", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Checks whether a specific component type exists on a game object.
        /// </summary>
        /// <param name="handle">Game object handle.</param>
        /// <param name="typeHandle">Registered component type handle.</param>
        /// <returns>True if the component is present.</returns>
        public extern static bool Internal_HasComponent(UInt64 handle, UInt32 typeHandle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetManagedComponent")]
        [StaticAccessor("ManagedComponents", StaticAccessorType.DoubleColon)]
        /// <summary>
        /// Retrieves a managed component instance given a game object and fully qualified type name.
        /// </summary>
        /// <param name="handle">Game object handle.</param>
        /// <param name="fullName">Fully qualified managed type name.</param>
        /// <returns>Component instance or null if not found.</returns>
        ///
        public extern static Object Internal_GetScriptableComponent(UInt64 handle, string fullName);


        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("DeleteComponent")]
        [StaticAccessor("ManagedComponents", StaticAccessorType.DoubleColon)]
        public extern static void Internal_DeleteComponent(UInt64 handle, UInt32 typeHandle);



        /// <summary>
        /// The <see cref="GameObject"/> this component is attached to.
        /// </summary>
        public GameObject gameObject
        {
            get
            {
                if (_gameObject == null)
                {
                    _gameObject = new GameObject(NativeID);
                }

                return _gameObject;

            }
        }

        /// <summary>
        /// Cached transform for the owning game object.
        /// </summary>
        public Transform transform
        {
            get
            {
                if (_transform == null)
                {
                    _transform = new Transform(NativeID);
                }

                return _transform;
            }
        }

        // TODO: Implement tag system
        /// <summary>
        /// User-defined tag string.
        /// </summary>
        public string tag { get; set; } = "Untagged";


        /// <summary>
        /// Indicates if the component is enabled and should execute in the engine.
        /// </summary>
        public bool Enabled { get; set; } = true;

        /// <summary>
        /// Retrieves a component of the specified type from the same game object.
        /// </summary>
        /// <typeparam name="T">Component type to query.</typeparam>
        /// <returns>An instance of <typeparamref name="T"/> if found; otherwise null.</returns>
        public T GetComponent<T>() where T : Component, new()
        {
            // Check if we getting a behavior component
            // Check if T derived from Behavior


            if (typeof(Behavior).IsAssignableFrom(typeof(T)))
            {
                // Type is a Behavior
                return (T)(Internal_GetScriptableComponent(NativeID, typeof(T).FullName) ?? null);

            }

            T obj = new T();
            bool res = Internal_HasComponent(NativeID, obj.typeID);
            if (res)
            {
                return new T { NativeID = NativeID };
            }

            return null;
        }

        public void DeleteComponent<T>() where T : Component, new()
        {
            T obj = new T();
            bool res = Internal_HasComponent(NativeID, obj.typeID);
            if (res)
            {
                Internal_DeleteComponent(NativeID, obj.typeID);

            }
            else throw new NullReferenceException("Component not found on Gameobject");






        }
    }
}
