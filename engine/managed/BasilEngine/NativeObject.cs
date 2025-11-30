
using Engine.Bindings;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BasilEngine
{
    [Disabled]
    /// <summary>
    /// Base type for all managed engine objects that provides access to the native handle.
    /// </summary>
    public class NativeObject
    {
        /// <summary>
        /// Unique identifier assigned by the native engine for this object instance.
        /// </summary>
        public UInt64 NativeID = 0;
    }

}
