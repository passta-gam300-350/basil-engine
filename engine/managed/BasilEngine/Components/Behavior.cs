using Engine.Bindings;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BasilEngine.Components
{
    [Disabled]
    public class Behavior : Component
    {
        public bool IsEnabled { get; set; } = true;

    }
}
