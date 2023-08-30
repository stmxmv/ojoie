using System.Runtime.CompilerServices;

namespace AN;

public class IMGUI : Component
{

    public IMGUI() : base(ConstructType.NotInheritConstructor)
    {
        Internal_AllocInitIMGUI(this);
    }
    
    
    [MethodImpl(MethodImplOptions.InternalCall)]
    private static extern void Internal_AllocInitIMGUI(IMGUI actor); 
}