using Engine.Bindings;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EngineAPI
{
    [NativeClass("GameObject", "Engine")]
    [NativeHeader("Engine/GameObject.h")]
   
    public class GameObject : NativeObject
    {
        [NativeMethod("GetName")]
        [StaticAccessor("GameObjectManager", StaticAccessorType.DoubleColon)]
        public extern string GetName();
        [NativeMethod("SetName")]
        [StaticAccessor("GameObjectManager", StaticAccessorType.DoubleColon)]
        public extern void SetName(string name);
        [NativeMethod("AddComponent")]
        [StaticAccessor("GameObjectManager", StaticAccessorType.DoubleColon)]
        public extern Component AddComponent(Type componentType);
        [NativeMethod("GetComponent")]
        [StaticAccessor("GameObjectManager", StaticAccessorType.DoubleColon)]
        public extern Component GetComponent(Type componentType);
    }
}
