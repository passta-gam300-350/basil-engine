using Engine.Bindings;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace BasilEngine.Components
{
    [Disabled]
    [NativeClass("Component", "Engine::Mono")]
    [NativeHeader("Engine/Component.h")]
    [NativeHeader("Engine/GameObject.h")]

    
    public class Component : NativeObject
    {
        private UInt32 typeID; // Identifies a component type in the native side.

        private Transform _transform = null;

        private GameObject _gameObject = null;

        [MethodImpl(MethodImplOptions.InternalCall)]
        // TODO: Implement Internal Checks
        public extern static UInt32 Internal_RegisterComponent(string typename);
        


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
        public string tag { get; set; } = "Untagged";


        public bool Enabled
        {
            get; set;
        } = true;

        public T GetComponent<T>() where T : Component, new()
        {
            T component = new T
            {
                NativeID = NativeID // Attach to the existing component
            };
            return component;
        }

        
    }
}
