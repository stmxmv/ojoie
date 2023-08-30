using System.Drawing;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace AN;

[StructLayout(LayoutKind.Sequential)]
public class Window : RC
{

    public Window(Rect rect, bool wantsLayer = true) => Internal_CreateWindow(this, rect, wantsLayer);
    
    public Window() => Internal_CreateWindow(this, new Rect(), false);

    [MethodImpl(MethodImplOptions.InternalCall)]
    public extern void MakeKeyAndOrderFront();
    
    [MethodImpl(MethodImplOptions.InternalCall)]
    public extern void Center();

    [MethodImpl(MethodImplOptions.InternalCall)]
    private static extern void Internal_CreateWindow(Window self, Rect rect, bool wantsLayer); 

}