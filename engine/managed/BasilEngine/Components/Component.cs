using Engine.Bindings;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace BasilEngine.Components
{
    [NativeClass("Component", "Engine::Mono")]
    [NativeHeader("Engine/Component.h")]
    [NativeHeader("Engine/GameObject.h")]

    public class Component : NativeObject
    {



        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetGameObject")]
        [StaticAccessor("ComponentManager", StaticAccessorType.DoubleColon)]
        private extern GameObject Get_GO_Internal();
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("GetTransform")]
        [StaticAccessor("ComponentManager", StaticAccessorType.DoubleColon)]
        private extern Transform Get_Transform_Internal();

        public GameObject gameObject
        {
            get => Get_GO_Internal();
        }
        public Transform transform
        {
            get => Get_Transform_Internal();
        }

        // TODO: Implement tag system
        public string tag { get; set; } = "Untagged"; 



    }
}
