#pragma once

#include <OleIdl.h>
#include <functional>

using dropCallback = std::function<void( char* )>;

class DragDropWatchdog : public IDropTarget
{
public:
    inline void SetDropCallback( dropCallback dropCallback ) { defaultDrop = dropCallback; }

public:
    DragDropWatchdog() = default;
    DragDropWatchdog( DragDropWatchdog& ) = delete;
    ~DragDropWatchdog() = default;

    HRESULT STDMETHODCALLTYPE DragEnter(
        /* [unique][in] */ __RPC__in_opt IDataObject *pDataObj,
        /* [in] */ DWORD grfKeyState,
        /* [in] */ POINTL pt,
        /* [out][in] */ __RPC__inout DWORD *pdwEffect );

    HRESULT STDMETHODCALLTYPE DragOver(
        /* [in] */ DWORD grfKeyState,
        /* [in] */ POINTL pt,
        /* [out][in] */ __RPC__inout DWORD *pdwEffect );

    HRESULT STDMETHODCALLTYPE DragLeave( void );

    HRESULT STDMETHODCALLTYPE Drop(
        /* [unique][in] */ __RPC__in_opt IDataObject *pDataObj,
        /* [in] */ DWORD grfKeyState,
        /* [in] */ POINTL pt,
        /* [out][in] */ __RPC__inout DWORD *pdwEffect );

    ULONG   AddRef( void ) override { return 0; };
    ULONG   Release( void ) override { return 0; };
    HRESULT QueryInterface( const IID& riid, void** ppvObject ) override { return S_OK; };

private:
    bool    isDroppable;
    dropCallback defaultDrop;

private:
    DWORD   DropEffect( DWORD grfKeyState, POINTL pt, DWORD dwAllowed );
};
