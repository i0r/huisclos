#include "DragDropWatchdog.h"

HRESULT STDMETHODCALLTYPE DragDropWatchdog::DragEnter(
    /* [unique][in] */ __RPC__in_opt IDataObject *pDataObj,
    /* [in] */ DWORD grfKeyState,
    /* [in] */ POINTL pt,
    /* [out][in] */ __RPC__inout DWORD *pdwEffect )
{
    FORMATETC fmtetc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

    isDroppable = SUCCEEDED( pDataObj->QueryGetData( &fmtetc ) );

    if ( isDroppable ) {
        *pdwEffect = DropEffect( grfKeyState, pt, *pdwEffect );
    } else {
        *pdwEffect = DROPEFFECT_NONE;
    }    return S_OK;
}

HRESULT STDMETHODCALLTYPE DragDropWatchdog::DragOver(
    /* [in] */ DWORD grfKeyState,
    /* [in] */ POINTL pt,
    /* [out][in] */ __RPC__inout DWORD *pdwEffect )
{
    if ( isDroppable ) {
        *pdwEffect = DropEffect( grfKeyState, pt, *pdwEffect );
    } else {
        *pdwEffect = DROPEFFECT_NONE;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DragDropWatchdog::DragLeave( void )
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DragDropWatchdog::Drop(
    /* [unique][in] */ __RPC__in_opt IDataObject *pDataObj,
    /* [in] */ DWORD grfKeyState,
    /* [in] */ POINTL pt,
    /* [out][in] */ __RPC__inout DWORD *pdwEffect )
{
    FORMATETC fmtetc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

    if ( isDroppable ) {
        STGMEDIUM stgmed = {};

        if ( SUCCEEDED( pDataObj->QueryGetData( &fmtetc ) ) ) {
            if ( SUCCEEDED( pDataObj->GetData( &fmtetc, &stgmed ) ) ) {
                static char lpszFileName[MAX_PATH] = {};

                HDROP data = ( HDROP )GlobalLock( stgmed.hGlobal );
                DragQueryFileA( data, 0, lpszFileName, MAX_PATH );
                GlobalUnlock( stgmed.hGlobal );

                ReleaseStgMedium( &stgmed );

                // TODO: filter by extension?
                defaultDrop( lpszFileName );
            }
        }

        *pdwEffect = DropEffect( grfKeyState, pt, *pdwEffect );
    } else {
        *pdwEffect = DROPEFFECT_NONE;
    }
    return S_OK;
}

DWORD DragDropWatchdog::DropEffect( DWORD grfKeyState, POINTL pt, DWORD dwAllowed )
{
    DWORD dwEffect = 0;

    // 1. check "pt" -> do we allow a drop at the specified coordinates?
    // 2. work out that the drop-effect should be based on grfKeyState  
    if ( grfKeyState & MK_CONTROL ) {
        dwEffect = dwAllowed & DROPEFFECT_COPY;
    } else if ( grfKeyState & MK_SHIFT ) {
        dwEffect = dwAllowed & DROPEFFECT_MOVE;
    }

    // 3. no key-modifiers were specified (or drop effect not allowed), so
    //    base the effect on those allowed by the dropsource
    if ( dwEffect == 0 ) {
        if ( dwAllowed & DROPEFFECT_COPY ) dwEffect = DROPEFFECT_COPY;
        if ( dwAllowed & DROPEFFECT_MOVE ) dwEffect = DROPEFFECT_MOVE;
    }

    return dwEffect;
}