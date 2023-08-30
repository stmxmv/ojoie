using System.Diagnostics.CodeAnalysis;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace AN;

/// <summary>
/// AppDelegate must mark with this attribute
/// </summary>
[System.AttributeUsage(System.AttributeTargets.Class |
                       System.AttributeTargets.Struct)
]
public class ApplicationMainAttribute : System.Attribute
{
}

/// <summary>
///  this interface is just for reference, AppDelegate may not inherit it
/// </summary>
public interface IApplicationDelegate
{
    void ApplicationWillFinishLaunching(Application application) {}
    void ApplicationDidFinishLaunching(Application application) {}
    bool ApplicationShouldTerminateAfterLastWindowClosed(Application application) => false;
    void ApplicationWillTerminate(Application application) {}

    /// <summary>
    /// game life cycle
    /// </summary>
    void Setup() {}
    void Start() {}
    void Stop() {}
}


[StructLayout(LayoutKind.Sequential)]
public class Application
{
    public IApplicationDelegate? AppDelegate { get; set; }

    public static Application GetSharedApplication() => Internal_GetSharedApplication_Injected();
    
    [MethodImpl(MethodImplOptions.InternalCall)]
    public extern void Run();

    [MethodImpl(MethodImplOptions.InternalCall)]
    private static extern Application Internal_GetSharedApplication_Injected();
    
    static Type? GetTypesWithApplicationMainAttribute(Assembly assembly) {
        foreach(Type type in assembly.GetTypes()) {
            if (type.GetCustomAttributes(typeof(ApplicationMainAttribute), true).Length > 0) {
                return type;
            }
        }
        return null;
    }
}