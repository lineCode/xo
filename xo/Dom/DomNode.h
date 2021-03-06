#pragma once
#include "DomEl.h"

namespace xo {

/* DOM node that is not text
It is vital that this data structure does not grow much bigger than this.
Right now it's 136 bytes on Windows x64.
*/
class XO_API DomNode : public DomEl {
	XO_DISALLOW_COPY_AND_ASSIGN(DomNode);

public:
	DomNode(xo::Doc* doc, xo::Tag tag, xo::InternalID parentID);
	virtual ~DomNode();

	void        SetText(const char* txt) override;
	const char* GetText() const override;
	void        CloneSlowInto(DomEl& c, uint32_t cloneFlags) const override;
	void        ForgetChildren() override;

	const cheapvec<DomEl*>& GetChildren() const { return Children; }
	cheapvec<StyleClassID>& GetClassesMutable() {
		IncVersion();
		return Classes;
	}
	const cheapvec<StyleClassID>& GetClasses() const { return Classes; }
	const Style&                  GetStyle() const { return Style; }
	const cheapvec<EventHandler>& GetHandlers() const { return Handlers; }
	void                          GetHandlers(const EventHandler*& handlers, size_t& count) const {
        handlers = &Handlers[0];
        count    = Handlers.size();
	}

	void           SetText(const std::string& txt) { SetText(txt.c_str()); }
	DomEl*         AddChild(xo::Tag tag, size_t position = -1);
	DomNode*       AddNode(xo::Tag tag, size_t position = -1);
	DomCanvas*     AddCanvas(size_t position = -1);
	DomText*       AddText(const char* txt = nullptr, size_t position = -1);
	DomText*       AddText(const std::string& txt, size_t position = -1);
	void           Delete(); // Remove from DOM, and delete self
	void           DeleteChild(DomEl* c);
	void           Clear(); // Delete all children
	size_t         ChildCount() const { return Children.size(); }
	DomEl*         ChildByIndex(size_t index);
	const DomEl*   ChildByIndex(size_t index) const;
	DomNode*       NodeByIndex(size_t index);
	const DomNode* NodeByIndex(size_t index) const;
	void           Discard();

	// Replace all child elements with the given xml-like string. Returns empty string on success, or error message.
	String   Parse(const char* src);
	DomEl*   ParseAppend(const char* src, String* error = nullptr); // Same as Parse, but append to node. Returns root of first element added.
	DomEl*   ParseAppend(const StringRaw& src, String* error = nullptr);
	DomEl*   ParseAppend(const std::string& src, String* error = nullptr);
	DomNode* ParseAppendNode(const char* src, String* error = nullptr);        // Logs a warning and returns null if resulting element is not a node
	DomNode* ParseAppendNode(const StringRaw& src, String* error = nullptr);   // Logs a warning and returns null if resulting element is not a node
	DomNode* ParseAppendNode(const std::string& src, String* error = nullptr); // Logs a warning and returns null if resulting element is not a node

	bool StyleParse(const char* s, size_t maxLen = -1);

	template <typename... Args>
	bool StyleParsef(const char* fs, const Args&... args);

	// TODO: This is here for experiments. Future work needs a better performing method for setting just one attribute of the style.
	void HackSetStyle(const Style& style);
	void HackSetStyle(StyleAttrib attrib); // TODO: This is also "Hack" because it doesn't work for attribute such as background-image

	// Classes
	void AddClass(const char* classes); // Add one more space-separated classes
	void RemoveClass(const char* klass);
	bool HasClass(const char* klass) const;

	// Events
	// An event handler has a 64-bit ID that is specific to that DOM node.
	// You can use that ID to remove the event handler again.
	uint64_t      AddHandler(Events ev, EventHandlerF func, void* context = NULL);
	uint64_t      AddHandler(Events ev, EventHandlerLambda0 lambda);
	uint64_t      AddHandler(Events ev, EventHandlerLambda1 lambda);
	void          RemoveHandler(uint64_t id);
	void          RemoveAllHandlers();
	EventHandler* HandlerByID(uint64_t id);
	bool          HandlesEvent(Events ev) const { return !!(AllEventMask & ev); }
	uint32_t      FastestTimerMS() const;                                               // Returns the period of our fastest ticking timer event (or zero if none)
	void          ReadyTimers(int64_t nowTicksMS, cheapvec<NodeEventIDPair>& handlers); // Fetch the list of timer events that are ready to run
	void          RenderHandlers(cheapvec<NodeEventIDPair>& handlers) const;            // Fetch the list of handlers for the Render event
	void          DocProcessHandlers(cheapvec<NodeEventIDPair>& handlers);              // Fetch the list of handlers for the DocProcess event
	bool          HasFocus() const;                                                     // Return true if this node has the keyboard focus
	void          SetCapture() const;                                                   // Captures input events so that they only fire on this node
	void          ReleaseCapture() const;                                               // Releases input capture

	// It is tempting to use macros to generate these event handler functions,
	// but the intellisense experience is so much worse that I avoid it.
	// These functions exist purely for discoverability, because one can already achieve
	// the same action by using the generic AddHandler().

	// Timer event handlers are special. If a timer event handler returns false, then the
	// timer event is cancelled.

	uint64_t OnWindowSize(EventHandlerF func, void* context) { return AddHandler(EventWindowSize, func, context); }
	uint64_t OnTimer(EventHandlerF func, void* context, uint32_t periodMS) { return AddHandler(EventTimer, func, EventHandlerFlags::EventHandlerFlag_None, context, periodMS); }
	uint64_t OnGetFocus(EventHandlerF func, void* context) { return AddHandler(EventGetFocus, func, context); }
	uint64_t OnLoseFocus(EventHandlerF func, void* context) { return AddHandler(EventLoseFocus, func, context); }
	uint64_t OnTouch(EventHandlerF func, void* context) { return AddHandler(EventTouch, func, context); }
	uint64_t OnClick(EventHandlerF func, void* context) { return AddHandler(EventClick, func, context); }
	uint64_t OnDblClick(EventHandlerF func, void* context) { return AddHandler(EventDblClick, func, context); }
	uint64_t OnMouseMove(EventHandlerF func, void* context) { return AddHandler(EventMouseMove, func, context); }
	uint64_t OnMouseEnter(EventHandlerF func, void* context) { return AddHandler(EventMouseEnter, func, context); }
	uint64_t OnMouseLeave(EventHandlerF func, void* context) { return AddHandler(EventMouseLeave, func, context); }
	uint64_t OnMouseDown(EventHandlerF func, void* context) { return AddHandler(EventMouseDown, func, context); }
	uint64_t OnMouseUp(EventHandlerF func, void* context) { return AddHandler(EventMouseUp, func, context); }
	uint64_t OnKeyDown(EventHandlerF func, void* context) { return AddHandler(EventKeyDown, func, context); }
	uint64_t OnKeyUp(EventHandlerF func, void* context) { return AddHandler(EventKeyUp, func, context); }
	uint64_t OnKeyChar(EventHandlerF func, void* context) { return AddHandler(EventKeyChar, func, context); }
	uint64_t OnDestroy(EventHandlerF func, void* context) { return AddHandler(EventDestroy, func, context); }
	uint64_t OnRender(EventHandlerF func, void* context) { return AddHandler(EventRender, func, context); }
	uint64_t OnDocProcess(EventHandlerF func, void* context) { return AddHandler(EventDocProcess, func, context); }

	uint64_t OnWindowSize(EventHandlerLambda0 lambda) { return AddHandler(EventWindowSize, lambda); }
	uint64_t OnTimer(EventHandlerLambda0 lambda, uint32_t periodMS) { return AddTimerHandler(EventTimer, lambda, periodMS); }
	uint64_t OnGetFocus(EventHandlerLambda0 lambda) { return AddHandler(EventGetFocus, lambda); }
	uint64_t OnLoseFocus(EventHandlerLambda0 lambda) { return AddHandler(EventLoseFocus, lambda); }
	uint64_t OnTouch(EventHandlerLambda0 lambda) { return AddHandler(EventTouch, lambda); }
	uint64_t OnClick(EventHandlerLambda0 lambda) { return AddHandler(EventClick, lambda); }
	uint64_t OnDblClick(EventHandlerLambda0 lambda) { return AddHandler(EventDblClick, lambda); }
	uint64_t OnMouseMove(EventHandlerLambda0 lambda) { return AddHandler(EventMouseMove, lambda); }
	uint64_t OnMouseEnter(EventHandlerLambda0 lambda) { return AddHandler(EventMouseEnter, lambda); }
	uint64_t OnMouseLeave(EventHandlerLambda0 lambda) { return AddHandler(EventMouseLeave, lambda); }
	uint64_t OnMouseDown(EventHandlerLambda0 lambda) { return AddHandler(EventMouseDown, lambda); }
	uint64_t OnMouseUp(EventHandlerLambda0 lambda) { return AddHandler(EventMouseUp, lambda); }
	uint64_t OnKeyDown(EventHandlerLambda0 lambda) { return AddHandler(EventKeyDown, lambda); }
	uint64_t OnKeyUp(EventHandlerLambda0 lambda) { return AddHandler(EventKeyUp, lambda); }
	uint64_t OnKeyChar(EventHandlerLambda0 lambda) { return AddHandler(EventKeyChar, lambda); }
	uint64_t OnDestroy(EventHandlerLambda0 lambda) { return AddHandler(EventDestroy, lambda); }
	uint64_t OnRender(EventHandlerLambda0 lambda) { return AddHandler(EventRender, lambda); }
	uint64_t OnDocProcess(EventHandlerLambda0 lambda) { return AddHandler(EventDocProcess, lambda); }

	uint64_t OnWindowSize(EventHandlerLambda1 lambda) { return AddHandler(EventWindowSize, lambda); }
	uint64_t OnTimer(EventHandlerLambda1 lambda, uint32_t periodMS) { return AddTimerHandler(EventTimer, lambda, periodMS); }
	uint64_t OnGetFocus(EventHandlerLambda1 lambda) { return AddHandler(EventGetFocus, lambda); }
	uint64_t OnLoseFocus(EventHandlerLambda1 lambda) { return AddHandler(EventLoseFocus, lambda); }
	uint64_t OnTouch(EventHandlerLambda1 lambda) { return AddHandler(EventTouch, lambda); }
	uint64_t OnClick(EventHandlerLambda1 lambda) { return AddHandler(EventClick, lambda); }
	uint64_t OnDblClick(EventHandlerLambda1 lambda) { return AddHandler(EventDblClick, lambda); }
	uint64_t OnMouseMove(EventHandlerLambda1 lambda) { return AddHandler(EventMouseMove, lambda); }
	uint64_t OnMouseEnter(EventHandlerLambda1 lambda) { return AddHandler(EventMouseEnter, lambda); }
	uint64_t OnMouseLeave(EventHandlerLambda1 lambda) { return AddHandler(EventMouseLeave, lambda); }
	uint64_t OnMouseDown(EventHandlerLambda1 lambda) { return AddHandler(EventMouseDown, lambda); }
	uint64_t OnMouseUp(EventHandlerLambda1 lambda) { return AddHandler(EventMouseUp, lambda); }
	uint64_t OnKeyDown(EventHandlerLambda1 lambda) { return AddHandler(EventKeyDown, lambda); }
	uint64_t OnKeyUp(EventHandlerLambda1 lambda) { return AddHandler(EventKeyUp, lambda); }
	uint64_t OnKeyChar(EventHandlerLambda1 lambda) { return AddHandler(EventKeyChar, lambda); }
	uint64_t OnDestroy(EventHandlerLambda1 lambda) { return AddHandler(EventDestroy, lambda); }
	uint64_t OnRender(EventHandlerLambda1 lambda) { return AddHandler(EventRender, lambda); }
	uint64_t OnDocProcess(EventHandlerLambda1 lambda) { return AddHandler(EventDocProcess, lambda); }

protected:
	uint64_t               NextEventHandlerID = 1;
	uint32_t               AllEventMask       = 0;
	xo::Style              Style; // Styles that override those referenced by the Tag and the Classes.
	cheapvec<EventHandler> Handlers;
	cheapvec<DomEl*>       Children;
	cheapvec<StyleClassID> Classes; // Classes of styles

	void     RecalcAllEventMask();
	uint64_t AddHandler(Events ev, EventHandlerF func, EventHandlerFlags flags, void* context, uint32_t timerPeriodMS);
	uint64_t AddTimerHandler(Events ev, EventHandlerLambda0 lambda, uint32_t periodMS);
	uint64_t AddTimerHandler(Events ev, EventHandlerLambda1 lambda, uint32_t periodMS);
	void     DeleteChildInternal(DomEl* c);
};

template <typename... Args>
bool DomNode::StyleParsef(const char* fs, const Args&... args) {
	char buf[1024];
	auto s  = tsf::fmt_buf(buf, sizeof(buf), fs, args...);
	bool ok = StyleParse(s.Str, s.Len);
	if (s.Str != buf)
		delete[] s.Str;
	return ok;
}
} // namespace xo
