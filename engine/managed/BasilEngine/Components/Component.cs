using Engine.Bindings;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace BasilEngine.Components
{
    [NativeHeader("Bindings/ManagedComponents.hpp")]


    public class Component : NativeObject
    {
        protected Component()
        {
           string typeName = this.GetType().Name;
           typeID = Internal_RegisterComponent(typeName);
        }

        protected Component(UInt64 handle)
        {
            NativeID = handle; // GameObject Handle
            
        }


        protected  UInt32 typeID; // Identifies a component type in the native side.

        private Transform _transform = null;

        private GameObject _gameObject = null;

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("ManagedRegisterComponent")]
        [StaticAccessor("ManagedComponents", StaticAccessorType.DoubleColon)]
        public extern static UInt32 Internal_RegisterComponent(string name);

        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("HasComponent")]
        [StaticAccessor("ManagedComponents", StaticAccessorType.DoubleColon)]
        public extern static bool Internal_HasComponent(UInt64 handle, UInt32 typeHandle);

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
            T obj = new T();
           bool res = Internal_HasComponent(NativeID, obj.typeID);
           if (res)
           {
               return new T { NativeID = NativeID };
           }
           return null;
        }


    }
}
