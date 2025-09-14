using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EngineAPI
{
    public class NativeObject
    {
        public IntPtr m_Handler { get; private set; } = IntPtr.Zero;
    }
}
