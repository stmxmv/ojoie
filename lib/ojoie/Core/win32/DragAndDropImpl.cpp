#include "Core/private/win32/DragAndDropImpl.hpp"

#include <ObjIdl.h>
#include <ShlObj.h>

/// note: when use c++ overload operator == (IsEqualGUID), it cause unresolved symbol =, I don't know why
/// so I just don't use that

// Implementation mostly based on OLE Drag and Drop tutorials from Catch22:
// http://catch22.net/tuts/dragdrop.asp


// ----------------------------------------------------------------------

static UInt32 s_ClipFormatPPtr = 0;
static UInt32 s_ClipFormatPaths = 0;

void InitializeDragAndDrop()
{
    OleInitialize(nullptr);
    s_ClipFormatPPtr = RegisterClipboardFormatA( "ANIDPtrFormat" );
    s_ClipFormatPaths = RegisterClipboardFormatA( "ANPathsFormat" );
}

void ShutdownDragAndDrop()
{
    OleUninitialize();
}

UInt32 GetClipboardFormatPPtr() { return s_ClipFormatPPtr; }
UInt32 GetClipboardFormatPaths() { return s_ClipFormatPaths; }
UInt32 GetClipboardFormatFiles() { return CF_HDROP; }


// ----------------------------------------------------------------------


// IDataObject implementation for OLE.
// Note that all __stdcall functions must appear in this order!
class CDataObject : public IDataObject
{
public:
    // IUnknown members
    HRESULT __stdcall QueryInterface (REFIID iid, void ** ppvObject);
    ULONG   __stdcall AddRef (void);
    ULONG   __stdcall Release (void);

    // IDataObject members
    HRESULT __stdcall GetData               (FORMATETC *pFormatEtc,  STGMEDIUM *pmedium);
    HRESULT __stdcall GetDataHere           (FORMATETC *pFormatEtc,  STGMEDIUM *pmedium);
    HRESULT __stdcall QueryGetData          (FORMATETC *pFormatEtc);
    HRESULT __stdcall GetCanonicalFormatEtc (FORMATETC *pFormatEct,  FORMATETC *pFormatEtcOut);
    HRESULT __stdcall SetData               (FORMATETC *pFormatEtc,  STGMEDIUM *pMedium,  BOOL fRelease);
    HRESULT __stdcall EnumFormatEtc         (DWORD      dwDirection, IEnumFORMATETC **ppEnumFormatEtc);
    HRESULT __stdcall DAdvise               (FORMATETC *pFormatEtc,  DWORD advf, IAdviseSink *, DWORD *);
    HRESULT __stdcall DUnadvise             (DWORD      dwConnection);
    HRESULT __stdcall EnumDAdvise           (IEnumSTATDATA **ppEnumAdvise);

    // Constructor / Destructor
    CDataObject(FORMATETC *fmtetc, STGMEDIUM *stgmed, int count);
    ~CDataObject();

private:
    LONG m_lRefCount;
    int	m_nNumFormats;
    FORMATETC* m_pFormatEtc;
    STGMEDIUM* m_pStgMedium;

    int LookupFormatEtc(FORMATETC *pFormatEtc);
};

CDataObject::CDataObject(FORMATETC *fmtetc, STGMEDIUM *stgmed, int count)
{
    // reference count must ALWAYS start at 1
    m_lRefCount    = 1;
    m_nNumFormats  = count;

    m_pFormatEtc   = new FORMATETC[count];
    m_pStgMedium   = new STGMEDIUM[count];

    for(int i = 0; i < count; i++)
    {
        m_pFormatEtc[i] = fmtetc[i];
        m_pStgMedium[i] = stgmed[i];
    }
}

CDataObject::~CDataObject()
{
    if(m_pFormatEtc) delete[] m_pFormatEtc;
    if(m_pStgMedium) delete[] m_pStgMedium;
}

ULONG __stdcall CDataObject::AddRef(void)
{
    return InterlockedIncrement(&m_lRefCount);
}
ULONG __stdcall CDataObject::Release(void)
{
    LONG count = InterlockedDecrement(&m_lRefCount);

    if(count == 0)
    {
        delete this;
        return 0;
    }
    else
    {
        return count;
    }
}

HRESULT __stdcall CDataObject::QueryInterface(REFIID iid, void **ppvObject)
{
    if(!!IsEqualGUID(iid, IID_IDataObject) || !!IsEqualGUID(iid,IID_IUnknown))
    {
        AddRef();
        *ppvObject = this;
        return S_OK;
    }
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }
    return S_OK;
}


HRESULT CreateDataObject(FORMATETC *fmtetc, STGMEDIUM *stgmeds, UINT count, IDataObject **ppDataObject)
{
    if(ppDataObject == 0)
        return E_INVALIDARG;
    *ppDataObject = new CDataObject(fmtetc, stgmeds, count);
    return (*ppDataObject) ? S_OK : E_OUTOFMEMORY;
}

HRESULT __stdcall CDataObject::QueryGetData(FORMATETC *pFormatEtc)
{
    return (LookupFormatEtc(pFormatEtc) == -1) ? DV_E_FORMATETC : S_OK;
}

int CDataObject::LookupFormatEtc(FORMATETC *pFormatEtc)
{
    // check each of our formats in turn to see if one matches
    for(int i = 0; i < m_nNumFormats; i++)
    {
        if((m_pFormatEtc[i].tymed    &  pFormatEtc->tymed)   &&
            m_pFormatEtc[i].cfFormat == pFormatEtc->cfFormat &&
            m_pFormatEtc[i].dwAspect == pFormatEtc->dwAspect)
        {
            // return index of stored format
            if( m_pStgMedium[i].hGlobal != NULL )
                return i;
        }
    }

    // error, format not found
    return -1;
}

static HGLOBAL DupGlobalMem(HGLOBAL hMem)
{
    DWORD   len    = GlobalSize(hMem);
    PVOID   source = GlobalLock(hMem);

    PVOID   dest   = GlobalAlloc(GMEM_FIXED, len);

    memcpy(dest, source, len);

    GlobalUnlock(hMem);
    return dest;
}

HRESULT __stdcall CDataObject::GetData (FORMATETC *pFormatEtc, STGMEDIUM *pStgMedium)
{
    int idx;

    // try to match the specified FORMATETC with one of our supported formats
    if((idx = LookupFormatEtc(pFormatEtc)) == -1)
        return DV_E_FORMATETC;

    // found a match - transfer data into supplied storage medium
    pStgMedium->tymed           = m_pFormatEtc[idx].tymed;
    pStgMedium->pUnkForRelease  = 0;

    // copy the data into the caller's storage medium
    switch(m_pFormatEtc[idx].tymed)
    {
        case TYMED_HGLOBAL:

            pStgMedium->hGlobal     = DupGlobalMem(m_pStgMedium[idx].hGlobal);
            break;

        default:
            return DV_E_FORMATETC;
    }

    return S_OK;
}

HRESULT __stdcall CDataObject::EnumFormatEtc (DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc)
{
    // only the get direction is supported for OLE
    if(dwDirection == DATADIR_GET)
    {
        return SHCreateStdEnumFmtEtc(m_nNumFormats, m_pFormatEtc, ppEnumFormatEtc);
    }
    else
    {
        // the direction specified is not supported for drag+drop
        return E_NOTIMPL;
    }
}

HRESULT CDataObject::DAdvise (FORMATETC *pFormatEtc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}
HRESULT CDataObject::DUnadvise (DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}
HRESULT CDataObject::EnumDAdvise (IEnumSTATDATA **ppEnumAdvise)
{
    return OLE_E_ADVISENOTSUPPORTED;
}
HRESULT CDataObject::GetDataHere (FORMATETC *pFormatEtc, STGMEDIUM *pMedium)
{
    return DATA_E_FORMATETC;
}
HRESULT CDataObject::GetCanonicalFormatEtc (FORMATETC *pFormatEct, FORMATETC *pFormatEtcOut)
{
    // Apparently we have to set this field to NULL even though we don't do anything else
    pFormatEtcOut->ptd = NULL;
    return E_NOTIMPL;
}
HRESULT CDataObject::SetData (FORMATETC *pFormatEtc, STGMEDIUM *pMedium,  BOOL fRelease)
{
    return E_NOTIMPL;
}


// ----------------------------------------------------------------------


// IDropSource implementation for OLE.
// Note that all __stdcall functions must appear in this order!
class CDropSource : public IDropSource
{
public:
    // IUnknown members
    HRESULT __stdcall QueryInterface    (REFIID iid, void ** ppvObject);
    ULONG   __stdcall AddRef            (void);
    ULONG   __stdcall Release           (void);

    // IDropSource members
    HRESULT __stdcall QueryContinueDrag (BOOL fEscapePressed, DWORD grfKeyState);
    HRESULT __stdcall GiveFeedback      (DWORD dwEffect);

    // Constructor / Destructor
    CDropSource();
    ~CDropSource();

private:
    LONG	   m_lRefCount;
};

CDropSource::CDropSource()
    :	m_lRefCount(1)
{
}
CDropSource::~CDropSource()
{
}

ULONG __stdcall CDropSource::AddRef(void)
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG __stdcall CDropSource::Release(void)
{
    LONG count = InterlockedDecrement(&m_lRefCount);
    if(count == 0)
    {
        delete this;
        return 0;
    }
    else
    {
        return count;
    }
}

HRESULT __stdcall CDropSource::QueryInterface(REFIID iid, void **ppvObject)
{
    if (!!IsEqualGUID(iid, IID_IDropSource) || !!IsEqualGUID(iid, IID_IUnknown))
    {
        AddRef();
        *ppvObject = this;
        return S_OK;
    }
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }
}


HRESULT __stdcall CDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    // if the Escape key has been pressed since the last call, cancel the drop
    if(fEscapePressed == TRUE)
        return DRAGDROP_S_CANCEL;

    // if the LeftMouse button has been released, then do the drop!
    if((grfKeyState & MK_LBUTTON) == 0)
        return DRAGDROP_S_DROP;

    // continue with the drag-drop
    return S_OK;
}

HRESULT __stdcall CDropSource::GiveFeedback(DWORD dwEffect)
{
    return DRAGDROP_S_USEDEFAULTCURSORS;
}

HRESULT CreateDropSource(IDropSource **ppDropSource)
{
    if(ppDropSource == 0)
        return E_INVALIDARG;

    *ppDropSource = new CDropSource();

    return (*ppDropSource) ? S_OK : E_OUTOFMEMORY;

}


// ----------------------------------------------------------------------


// IDropTarget implementation for OLE.
// Note that all __stdcall functions must appear in this order!
class CDropTarget : public IDropTarget
{
public:
    // IUnknown implementation
    HRESULT __stdcall QueryInterface (REFIID iid, void ** ppvObject);
    ULONG   __stdcall AddRef (void);
    ULONG   __stdcall Release (void);

    // IDropTarget implementation
    HRESULT __stdcall DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
    HRESULT __stdcall DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
    HRESULT __stdcall DragLeave(void);
    HRESULT __stdcall Drop(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);

    // Constructor
    CDropTarget( HWND hwnd, IWindowsDropTarget& dropInterface, void* userData );
    ~CDropTarget();

private:
    long   m_lRefCount;
    HWND   m_hWnd;
    bool   m_fAllowDrop;

    IWindowsDropTarget*	m_DropInterface;
    void*			m_UserData;
};

CDropTarget::CDropTarget( HWND hwnd, IWindowsDropTarget& dropInterface, void* userData )
    :	m_lRefCount(1)
      ,	m_hWnd(hwnd)
      ,	m_fAllowDrop(false)
      ,	m_DropInterface(&dropInterface)
      ,	m_UserData(userData)
{
}

CDropTarget::~CDropTarget()
{
}

HRESULT __stdcall CDropTarget::QueryInterface (REFIID iid, void ** ppvObject)
{
    if(!!IsEqualGUID(iid, IID_IDropTarget) || !!IsEqualGUID(iid, IID_IUnknown))
    {
        AddRef();
        *ppvObject = this;
        return S_OK;
    }
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }
}

ULONG __stdcall CDropTarget::AddRef(void)
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG __stdcall CDropTarget::Release(void)
{
    LONG count = InterlockedDecrement(&m_lRefCount);

    if(count == 0)
    {
        delete this;
        return 0;
    }
    else
    {
        return count;
    }
}


HRESULT __stdcall CDropTarget::DragEnter( IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect )
{
    // does the data object contain data we want?
    m_fAllowDrop = m_DropInterface->UnderstandsData( pDataObject );
    if( m_fAllowDrop )
    {
        SetFocus( m_hWnd );

        *pdwEffect = m_DropInterface->DragEntered( m_UserData, grfKeyState, pt );
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    return S_OK;
}

HRESULT __stdcall CDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
    if( m_fAllowDrop )
    {
        *pdwEffect = m_DropInterface->DragUpdated( m_UserData, grfKeyState, pt );
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    return S_OK;
}

HRESULT __stdcall CDropTarget::DragLeave(void)
{
    m_DropInterface->DragExited( m_UserData );
    return S_OK;
}

HRESULT __stdcall CDropTarget::Drop(IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    if( m_fAllowDrop )
    {
        *pdwEffect = m_DropInterface->DragUpdated( m_UserData, grfKeyState, pt );
        // If the dragging updated returns false we will not even try to perform the drag
        if( *pdwEffect != DROPEFFECT_NONE )
            m_DropInterface->DragPerformed( m_UserData, grfKeyState, pt );
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    return S_OK;
}


void RegisterDropWindow( HWND hwnd, IWindowsDropTarget& dropInterface, void* userData, IDropTarget **ppDropTarget )
{
    CDropTarget *pDropTarget = new CDropTarget(hwnd, dropInterface, userData);

    // acquire a strong lock
    CoLockObjectExternal(pDropTarget, TRUE, FALSE);

    // tell OLE that the window is a drop target
    RegisterDragDrop(hwnd, pDropTarget);

    *ppDropTarget = pDropTarget;
}

void UnregisterDropWindow(HWND hwnd, IDropTarget *pDropTarget)
{
    // remove drag+drop
    RevokeDragDrop(hwnd);

    // remove the strong lock
    CoLockObjectExternal(pDropTarget, FALSE, TRUE);

    // release our own reference
    pDropTarget->Release();
}

