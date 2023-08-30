using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace AN;

[StructLayout(LayoutKind.Sequential)]
public class RC : IDisposable
{
    private IntPtr m_Impl;
    
    private bool disposed = false;


    public IntPtr GetImpl()
    {
        if (m_Impl == IntPtr.Zero)
        {
            throw new ObjectDisposedException("RC Object is not valid");
        }

        return m_Impl;
    }
    
    // Use C# destructor syntax for finalization code.
    // This destructor will run only if the Dispose method 
    // does not get called.
    // It gives your base class the opportunity to finalize.
    // Do not provide destructors in types derived from this class.
    ~RC()      
    {
        // Do not re-create Dispose clean-up code here.
        // Calling Dispose(false) is optimal in terms of
        // readability and maintainability.
        Dispose(false);
    }
    
    public void Dispose()
    {
        Dispose(true);
        // Take yourself off the Finalization queue 
        // to prevent finalization code for this object
        // from executing a second time.
        GC.SuppressFinalize(this);
    }
    
    // Dispose(bool disposing) executes in two distinct scenarios.
    // If disposing equals true, the method has been called directly
    // or indirectly by a user's code. Managed and unmanaged resources
    // can be disposed.
    // If disposing equals false, the method has been called by the 
    // runtime from inside the finalizer and you should not reference 
    // other objects. Only unmanaged resources can be disposed.
    protected virtual void Dispose(bool disposing)
    {
        // Check to see if Dispose has already been called.
        if (!this.disposed)
        {
            // If disposing equals true, dispose all managed 
            // and unmanaged resources.
            if(disposing)
            {
                // Dispose managed resources.
            }
            // Release unmanaged resources. If disposing is false, 
            // only the following code is executed.
            
            // CloseHandle(handle);
            // handle = IntPtr.Zero;
            Release();
            
            // Note that this is not thread safe.
            // Another thread could start disposing the object
            // after the managed resources are disposed,
            // but before the disposed flag is set to true.
            // If thread safety is necessary, it must be
            // implemented by the client.
        }
        disposed = true;         
    }

    public void Retain() => Internal_RCRetain(this);
    public void Release() => Internal_RCRelease(this);
    

    [MethodImpl(MethodImplOptions.InternalCall)]
    private static extern void Internal_RCRetain(RC self); 
    
    [MethodImpl(MethodImplOptions.InternalCall)]
    private static extern void Internal_RCRelease(RC self); 
}