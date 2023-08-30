using AN;
using Object = AN.Object;


class MainIMGUI : IMGUI
{
    void OnGUI()
    {
        
    }
}



[ApplicationMain]
public class AppDelegate : IApplicationDelegate
{

    private Window? mainWindow;
    private Object? obj;
    
    public void ApplicationDidFinishLaunching(Application application)
    {
        mainWindow = new Window(new Rect(0, 0, 1920, 1080));
        mainWindow?.Center();
        mainWindow?.MakeKeyAndOrderFront();
        Debug.Log("App Launched");

        obj = new Object();
        Debug.Log(obj.DebugDescription());
    }

    public void ApplicationWillTerminate(Application application)
    {
        if (obj != null)
        {
            Object.DestroyImmediate(obj);
        }
        mainWindow?.Dispose();
    }

    public bool ApplicationShouldTerminateAfterLastWindowClosed(Application application)
    {
        return true;
    }

    public void Start()
    {
        Actor actor = new Actor("Main Actor");
        actor.AddComponent<MainIMGUI>();
    }
}