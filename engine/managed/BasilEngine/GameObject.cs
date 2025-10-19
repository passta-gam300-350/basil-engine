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

    public class GameObject : NativeObject
    {
        // TODO: Add Glue Gen Attribute
        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern string internal_GetName(UInt32 handle);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void internal_SetName(UInt32 handle, string name);


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
            set
            {
                layer = value;
            }

        }

        public string name
        {
            get => internal_GetName(this.NativeID);
            set => internal_SetName(this.NativeID, value);
        }


    }
}
         
