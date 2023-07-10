using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;

namespace HelloWorld;

public static class Debug
{
    [MethodImpl(MethodImplOptions.InternalCall)]
    public static extern void Log(string msg);
}


[StructLayout(LayoutKind.Sequential)]
public class Test
{

    private int m_value;

    public Test()
    {
        m_value = 24;
    }
    
    
    [DllImport ("__Internal", EntryPoint="NativeFunction")]
    static extern void NativeFunction();
    
    [MethodImpl(MethodImplOptions.InternalCall)]
    public extern string DoSomething();
    
    

    public void SomeTest()
    {
        Debug.Log("test");
        Debug.Log(DoSomething());
        NativeFunction();
        Debug.Log($"m_value is {m_value}");
        
        GC.Collect(); // manual trigger GC collect
    }

}

