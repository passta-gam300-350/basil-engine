namespace EngineAPI;
using Engine.Bindings;
using System.Runtime.CompilerServices;

[NativeHeader("Runtime/Engine/Public/EngineAPI/Class1.h")]
public class Class1
{
    [StaticAccessor("Class1CPP", StaticAccessorType.DoubleColon)]
    [NativeMethod("Adder")]
    [MethodImpl(MethodImplOptions.InternalCall)]
    public static extern int Add(int a, int b);

    public int AddNat(int a, int b)
    {
        return 100;
    }


}
