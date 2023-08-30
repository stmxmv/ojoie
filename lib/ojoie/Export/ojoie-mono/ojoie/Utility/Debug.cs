using System.Runtime.CompilerServices;

namespace AN;

public class Debug
{
    [MethodImpl(MethodImplOptions.InternalCall)]
    public static extern void Log(String msg);

    [MethodImpl(MethodImplOptions.InternalCall)]
    public static extern void Assert(bool assertion);
}