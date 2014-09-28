#include "pch.h"
#include "xoDoc.h"
#include "xoDocGroup.h"
#include "Layout/xoLayout.h"
#include "Layout/xoLayout2.h"
#include "xoSysWnd.h"
#include "Render/xoRenderer.h"
#include "Render/xoRenderDoc.h"
#include "Render/xoRenderDomEl.h"
#include "Render/xoRenderBase.h"
#include "Render/xoRenderGL.h"
#include "Render/xoStyleResolve.h"

xoDocGroup::xoDocGroup()
{
	AbcCriticalSectionInitialize( DocLock );
	DestroyDocWithGroup = false;
	Doc = NULL;
	Wnd = NULL;
	RenderDoc = new xoRenderDoc();
	RenderStats.Reset();
}

xoDocGroup::~xoDocGroup()
{
	delete RenderDoc;
	if ( DestroyDocWithGroup )
		delete Doc;
	AbcCriticalSectionDestroy( DocLock );
}

xoRenderResult xoDocGroup::Render()
{
	return RenderInternal( NULL );
}

xoRenderResult xoDocGroup::RenderToImage( xoImage& image )
{
	// The 10 here is an arbitrary thumbsuck. We'll see if we ever need a controllable limit.
	const int maxAttempts = 10;
	xoRenderResult res = xoRenderResultNeedMore;
	for ( int attempt = 0; res == xoRenderResultNeedMore && attempt < maxAttempts; attempt++ )
		res = RenderInternal( &image );
	return res;
}

// This is always called from the Render thread
xoRenderResult xoDocGroup::RenderInternal( xoImage* targetImage )
{
	bool haveLock = false;
	// I'm not quite sure how we should handle this. The idea is that you don't want to go without a UI update
	// for too long, even if the UI thread is taking its time, and being bombarded with messages.
	uint32 rDocAge = Doc->GetVersion() - RenderDoc->Doc.GetVersion(); 
	if ( rDocAge > 0 || targetImage != NULL )
	{
		// If UI thread has performed many updates since we last rendered,
		// then pause our thread until we can gain the DocLock
		haveLock = true;
		AbcCriticalSectionEnter( DocLock );
	}
	else
	{
		// The UI thread has not done much since we last rendered, so do not wait for the lock
		haveLock = AbcCriticalSectionTryEnter( DocLock );
	}

	if ( !haveLock )
	{
		NUTIME( "Render: Failed to acquire DocLock\n" );
		return xoRenderResultNeedMore;
	}

	// TODO: If AnyAnimationsRunning(), then we are not idle
	bool docValid = Doc->UI.GetViewportWidth() != 0 && Doc->UI.GetViewportHeight() != 0;
	bool docModified = Doc->GetVersion() != RenderDoc->Doc.GetVersion();
	
	if ( docModified && docValid )
	{
		//XOTRACE( "Render Version %u\n", Doc->GetVersion() );
		RenderDoc->CopyFromCanonical( *Doc, RenderStats );
		
		// Assume we are the only renderer of 'Doc'. If this assumption were not true, then you would need to update
		// all renderers simultaneously, so that you can guarantee that UsableIDs all go to FreeIDs atomically.
		//XOTRACE( "MakeFreeIDsUsable\n" );
		Doc->MakeFreeIDsUsable();
		Doc->ResetModifiedBitmap();			// AbcBitMap has an absolutely awful implementation of this (byte-filled vs SSE or at least pointer-word-size-filled)
	}
	AbcCriticalSectionLeave( DocLock );

	xoRenderResult rendResult = xoRenderResultIdle;

	if ( (docModified || targetImage != NULL) && docValid && Wnd != NULL )
	{
		//NUTIME( "Render start\n" );
		if ( !Wnd->BeginRender() )
		{
			NUTIME( "BeginRender failed\n" );
			return xoRenderResultNeedMore;
		}

		//NUTIME( "Render DO\n" );
		rendResult = RenderDoc->Render( Wnd->Renderer );

		if ( targetImage != NULL )
			Wnd->Renderer->ReadBackbuffer( *targetImage );

		//NUTIME( "Render Finish\n" );
		Wnd->EndRender();
	}

	return rendResult;
}

void xoDocGroup::ProcessEvent( xoEvent& ev )
{
	TakeCriticalSection lock( DocLock );

	if ( ev.Type != xoEventTimer )
		XOTRACE_LATENCY("ProcessEvent (not a timer)\n");

	xoLayoutResult* layout = RenderDoc->AcquireLatestLayout();
	
	Doc->UI.InternalProcessEvent( ev, layout );
	
	RenderDoc->ReleaseLayout( layout );
}

bool xoDocGroup::IsDocVersionDifferentToRenderer() const
{
	return Doc->GetVersion() != RenderDoc->Doc.GetVersion();
}

