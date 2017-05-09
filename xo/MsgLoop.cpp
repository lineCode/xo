#include "pch.h"
#include "Defs.h"
#include "DocGroup.h"
#include "SysWnd.h"
#include "Event.h"
#include "Doc.h"

namespace xo {

#if XO_PLATFORM_WIN_DESKTOP

static bool AnyDocsDirty() {
	for (size_t i = 0; i < Global()->Docs.size(); i++) {
		if (Global()->Docs[i]->IsDirty())
			return true;
	}
	return false;
}

static void SetupTimerMessagesForAllDocs() {
	for (DocGroup* dg : Global()->Docs)
		dg->SetSysWndTimer(dg->Doc->FastestTimerMS());
}

XO_API void RunWin32MessageLoop() {
	while (true) {
		SetupTimerMessagesForAllDocs();

		// Dispatch messages until queue is empty. This should work, because a message
		// dispatch is not actually running much code. It only generates our own
		// platform-agnostic event and places that event in our own queue. The actual
		// handling of those events occur on our UI thread. When the UI thread
		// changes our document state, so that it needs to be rendered, then we invalidate
		// our window using InvalidateRect, and we rely on a WM_PAINT message to wake
		// us up from the GetMessage() sleep. We don't really need any of the windows
		// WM_PAINT/invalidation infrastructure. All we really need is a jolt that causes
		// GetMessage() to return control to us, so that we can go ahead and draw ourselves.
		bool mustQuit = false;
		while (true) {
			MSG msg;
			if (!GetMessage(&msg, NULL, 0, 0)) {
				mustQuit = true;
				break;
			}
			if (msg.message == WM_KEYDOWN)
				int abc = 123;
			XOTRACE_OS_MSG_QUEUE("msg start: %x\n", msg.message);
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			XOTRACE_OS_MSG_QUEUE("msg end: %x. AnyDocsDirty() ? %s\n", msg.message, AnyDocsDirty() ? "yes" : "no");
			if (!PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE))
				break;
		}
		if (mustQuit)
			break;

		for (DocGroup* dg : Global()->Docs) {
			if (dg->IsDirty()) {
				XOTRACE_OS_MSG_QUEUE("Render enter (%p)\n", dg);
				RenderResult rr = dg->Render();
				if (rr == RenderResultNeedMore) {
					dg->Wnd->PostRepaintMessage();
				} else {
					dg->Wnd->ValidateWindow();
				}
			}
		}

		// Add/remove items from the global list of windows. This only happens at Doc creation/destruction time.
		AddOrRemoveDocsFromGlobalList();
	}

	//timeEndPeriod( 5 );
}

#elif XO_PLATFORM_LINUX_DESKTOP

extern SysWnd* SingleMainWnd;

// This implementation is a super-quick get-something-on-the-screen kinda thing.
// To be consistent with Windows Desktop we should process events on a different
// thread to the render thread. Also, we blindly send events to all docs, which
// is absurd.
XO_API void RunXMessageLoop() {
	AddOrRemoveDocsFromGlobalList();

	while (1) {
		XEvent xev;
		XNextEvent(SingleMainWnd->XDisplay, &xev);

		if (xev.type == Expose) {
			XWindowAttributes wa;
			XGetWindowAttributes(SingleMainWnd->XDisplay, SingleMainWnd->XWindow, &wa);
			/*
			glXMakeCurrent( SingleMainWnd->XDisplay, SingleMainWnd->Window, SingleMainWnd->GLContext );
			glViewport( 0, 0, gwa.width, gwa.height );
			//DrawAQuad();
			//glXSwapBuffers(dpy, win);
			glXMakeCurrent( SingleMainWnd->XDisplay, None, Null );
			*/
			Event nev;
			nev.Type        = EventWindowSize;
			nev.PointsAbs[0].x = wa.width;
			nev.PointsAbs[0].y = wa.height;
			for (int i = 0; i < Global()->Docs.size(); i++)
				Global()->Docs[i]->ProcessEvent(nev);

		} else if (xev.type == KeyPress) {
			XOTRACE_OS_MSG_QUEUE("key = %d\n", xev.xkey.keycode);
			if (xev.xkey.keycode == 24) // 'q'
				break;
		} else if (xev.type == MotionNotify) {
			//XOTRACE_OS_MSG_QUEUE( "x,y = %d,%d\n", xev.xmotion.x, xev.xmotion.y );
			Event nev;
			nev.Type        = EventMouseMove;
			nev.PointsAbs[0].x = xev.xmotion.x;
			nev.PointsAbs[0].y = xev.xmotion.y;
			for (int i = 0; i < Global()->Docs.size(); i++)
				Global()->Docs[i]->ProcessEvent(nev);
		}

		for (int i = 0; i < Global()->Docs.size(); i++) {
			RenderResult rr = Global()->Docs[i]->Render();
			//XOTRACE_OS_MSG_QUEUE( "rr = %d\n", rr );
			//if ( rr != RenderResultDone )
			//	renderIdle = false;
		}

		AddOrRemoveDocsFromGlobalList();
	}
}

#endif
}
