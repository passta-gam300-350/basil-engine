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
        public extern static void DeleteGameObjectInternal(UInt64 handle);





        private Transform transformComponent = null;

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

        public bool activeSelf
        {
            //TODO: Implement activeSelf getter
            get => false;
        }

        //TODO: Implement activeInHierarchy getter
        public bool activeInHierarchy
        {
            get => false;
        }

        public bool isStatic
        {
            get => false;

        }

        //TODO: Implement layer getter and setter
        public int layer
        {
            get => 0;
            set { layer = value; }

        }





        public GameObject(UInt64 handle)
        {
            this.NativeID = handle;
            Console.WriteLine("GameObject created with handle: " + handle);
            transformComponent = new Transform(handle);
        }

        public static GameObject Find(string name)
        {
            UInt64 handle = internal_Find(name);
            if (handle == 0)
            {
                return null;
            }

            return new GameObject(handle);


        }

        public static void Destroy(GameObject obj)
        {
            if (obj != null)
            {
                DeleteGameObjectInternal(obj.NativeID);
            }
        }
    }
}
         
