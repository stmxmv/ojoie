using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;   

namespace AN;

[StructLayout(LayoutKind.Sequential)]
public class Object
{
    private IntPtr m_Impl;

    public enum ConstructType
    {
        Base,
        NotInheritConstructor
    }

    public Object() : this(ConstructType.Base) {}
    
    protected Object(ConstructType type)
    {
        if (type == ConstructType.Base)
        {
            Internal_AllocInitObject(this); 
        }
    }
    
    [MethodImpl(MethodImplOptions.InternalCall)]
    public extern String DebugDescription();

    [MethodImpl(MethodImplOptions.InternalCall)]
    public static extern void DestroyImmediate(Object obj);
    
    [MethodImpl(MethodImplOptions.InternalCall)]
    private static extern void Internal_AllocInitObject(Object obj); 
    
}