using Engine.Bindings;
namespace BasilEngine;
using System.Runtime.CompilerServices;

    [Disabled]
[NativeHeader("Runtime/Engine/Public/EngineAPI/Class1.h")]
/// <summary>
/// Sample bindings class used to validate managed to native calls.
/// </summary>
public class Class1
{
    /// <summary>
    /// Adds two integers using a native implementation.
    /// </summary>
    /// <param name="a">First operand.</param>
    /// <param name="b">Second operand.</param>
    /// <returns>Sum of <paramref name="a"/> and <paramref name="b"/>.</returns>
    public static extern int Add(int a, int b);

    /// <summary>
    /// Adds two integers using a managed stub implementation.
    /// </summary>
    /// <param name="a">First operand.</param>
    /// <param name="b">Second operand.</param>
    /// <returns>Sum of the inputs.</returns>
    public int AddNat(int a, int b)
    {
        return 100;
    }


}
