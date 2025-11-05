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


        private Transform _transform = null;

        private GameObject _gameObject = null;

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

    }
}
