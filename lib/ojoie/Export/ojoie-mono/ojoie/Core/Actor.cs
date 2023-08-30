using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace AN;

[StructLayout(LayoutKind.Sequential)]
public sealed class Actor : Object
{

    public Actor(string? name) : base(ConstructType.NotInheritConstructor)
    {
        Internal_AllocInitActor(this, name);
    }
    
    public T AddComponent<T>() where T : Component => (this.AddComponent(typeof (T)) as T)!;
    
    public Component AddComponent(System.Type componentType) => this.Internal_AddComponentWithType(componentType);
    
    
    [MethodImpl(MethodImplOptions.InternalCall)]
    private static extern void Internal_AllocInitActor(Actor actor, string? name); 
    
    [MethodImpl(MethodImplOptions.InternalCall)]
    private extern Component Internal_AddComponentWithType(System.Type componentType);
}