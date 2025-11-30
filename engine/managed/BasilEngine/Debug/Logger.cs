using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.CompilerServices;
using System.Runtime.Remoting;
using Engine.Bindings;
namespace BasilEngine.Debug
{

    [NativeHeader("Bindings/MANAGED_CONSOLE.hpp")]
    [NativeClass("ManagedConsole")]
    /// <summary>
    /// Managed logging facade that forwards messages to the native console.
    /// </summary>
    public static class Logger
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("LogInfo")]
        [StaticAccessor("ManagedConsole", StaticAccessorType.DoubleColon)]


        private extern static void internal_Log(string message);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("LogWarning")]
        [StaticAccessor("ManagedConsole", StaticAccessorType.DoubleColon)]
        private extern static void internal_Warn(string message);
        [MethodImpl(MethodImplOptions.InternalCall)]
        [NativeMethod("LogError")]
        [StaticAccessor("ManagedConsole", StaticAccessorType.DoubleColon)]
        private extern static void internal_Error(string message);


        /// <summary>
        /// Logs an informational message.
        /// </summary>
        /// <param name="message">Message to record.</param>
        public static void Log(Object message)
        {
            internal_Log(message.ToString());
        }
        /// <summary>
        /// Logs a warning message.
        /// </summary>
        /// <param name="message">Message to record.</param>
        public static void Warn(Object message)
        {
            internal_Warn(message.ToString());
        }
        /// <summary>
        /// Logs an error message.
        /// </summary>
        /// <param name="message">Message to record.</param>
        public static void Error(Object message)
        {
            internal_Error(message.ToString());
        }


    }
}
