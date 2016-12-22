#include "pch.h"
#include "Defs.h"
#include "DocGroup.h"
#include "SysWnd.h"
#include "Event.h"
#include "Doc.h"

namespace xo {

#if XO_PLATFORM_WIN_DESKTOP

static bool AnyDocsDirty() {
	for (int i = 0; i < Global()->Docs.size(); i++) {
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
	// Before HEAT_TIME_1, we sleep(1 ms), and call PeekMessage
	// Before HEAT_TIME_2, we sleep(5 ms), and call PeekMessage
	// After HEAT_TIME_2, we call GetMessage
	const double HEAT_TIME_1 = 0.020;
	const double HEAT_TIME_2 = 0.300;

	double lastFrameStart = TimeAccurateSeconds();
	double lastHeatAt     = TimeAccurateSeconds();

	// Toggled when all renderers report that they have no further work to do (ie no animations playing, now or any time in the future)
	// In that case, the only way we can have something happen is if we have an incoming message.
	// Of course, this is NOT true for IO that is busy occurring on the UI thread.
	// We'll have to devise a way (probably just a process-global custom window message) of causing the main loop to wake up.
	bool renderIdle = false;

	// Trying various things to get latency down to the same level as GDI, but I just can't do it.
	//timeBeginPeriod( 5 );

	while (true) {
		SetupTimerMessagesForAllDocs();

		// When idle, use GetMessage so that the OS can put us into a good sleep
		MSG  msg;
		bool haveMsg = true;
		if (renderIdle && !AnyDocsDirty() && TimeAccurateSeconds() - lastHeatAt > HEAT_TIME_2) {
			XOTRACE_OS_MSG_QUEUE("Render cold\n");
			if (!GetMessage(&msg, NULL, 0, 0))
				break;
			XOTRACE_OS_MSG_QUEUE("GetMessage returned\n");
		} else {
			XOTRACE_OS_MSG_QUEUE("Render hot\n");
			haveMsg = !!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
		}

		if (haveMsg) {
			XOTRACE_OS_MSG_QUEUE("msg start: %x\n", msg.message);
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			XOTRACE_OS_MSG_QUEUE("msg end: %x\n", msg.message);
			if (msg.message != WM_TIMER)
				lastHeatAt = TimeAccurateSeconds();
		}

		double now            = TimeAccurateSeconds();
		double nextFrameStart = lastFrameStart + 1.0 / Global()->TargetFPS;
		if (now >= nextFrameStart || AnyDocsDirty()) {
			XOTRACE_OS_MSG_QUEUE("Render enter\n");
			renderIdle     = true;
			lastFrameStart = now;
			for (DocGroup* dg : Global()->Docs) {
				if (dg->IsDirty()) {
					RenderResult rr = dg->Render();
					if (rr != RenderResultIdle)
						renderIdle = false;
					dg->Wnd->ValidateWindow();
				}
			}
		} else {
			if (TimeAccurateSeconds() - lastHeatAt > HEAT_TIME_1)
				SleepMS(5);
			else
				SleepMS(1);
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
			nev.Points[0].x = wa.width;
			nev.Points[0].y = wa.height;
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
			nev.Points[0].x = xev.xmotion.x;
			nev.Points[0].y = xev.xmotion.y;
			for (int i = 0; i < Global()->Docs.size(); i++)
				Global()->Docs[i]->ProcessEvent(nev);
		}

		for (int i = 0; i < Global()->Docs.size(); i++) {
			RenderResult rr = Global()->Docs[i]->Render();
			//XOTRACE_OS_MSG_QUEUE( "rr = %d\n", rr );
			//if ( rr != RenderResultIdle )
			//	renderIdle = false;
		}

		AddOrRemoveDocsFromGlobalList();
	}
}

#endif
}