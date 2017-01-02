#include "pch.h"
#include "Doc.h"
#include "Style.h"
#include "CloneHelpers.h"
#include "Text/FontStore.h"

namespace xo {

#define EQ(a, b) (strcmp(a, b) == 0)

// Styles that are inherited by default
const StyleCategories InheritedStyleCategories[NumInheritedStyleCategories] = {
    CatFontFamily,
    CatFontSize,
    CatColor,
    CatText_Align_Vertical,
    CatCursor,
};

inline bool IsNumeric(char c) {
	return (c >= '0' && c <= '9') || (c == '.') || (c == '-');
}

// This is parsing whitespace, not DOM/textual whitespace
// In other words, it is the space between the comma and verdana in "font-family: verdana, arial",
inline bool IsWhitespace(char c) {
	return c == 32 || c == 9;
}

static bool MATCH(const char* s, size_t start, size_t end, const char* truth) {
	for (; start != end; truth++, start++) {
		if (s[start] != *truth)
			return false;
	}
	return true;
}

static uint8_t ParseHexChar(char ch) {
	if (ch >= 'A' && ch <= 'F')
		return 10 + ch - 'A';
	if (ch >= 'a' && ch <= 'f')
		return 10 + ch - 'a';
	else
		return ch - '0';
}

static uint8_t ParseHexCharPair(const char* ch) {
	return (ParseHexChar(ch[0]) << 4) | ParseHexChar(ch[1]);
}

static uint8_t ParseHexCharSingle(const char* ch) {
	uint8_t v = ParseHexChar(ch[0]);
	return (v << 4) | v;
}

static int FindSpaces(const char* s, size_t len, int (&spaces)[10]) {
	int nspaces = 0;
	for (size_t i = 0; i < len && nspaces < arraysize(spaces); i++) {
		if (IsWhitespace(s[i]))
			spaces[nspaces++] = (int) i;
	}
	return nspaces;
}

bool Size::Parse(const char* s, size_t len, Size& v) {
	// 1.23em
	// 1.23ep
	// 1.23ex
	// 1.23px
	// 1.23pt
	// 1.23%
	// 0
	char digits[100];
	if (len > 30) {
		ParseFail("Parse failed, size too big (>30 characters)\n");
		return false;
	}
	Size   x      = Size::Pixels(0);
	size_t nondig = 0;
	for (; nondig < len; nondig++) {
		digits[nondig] = s[nondig];
		if (!IsNumeric(s[nondig]))
			break;
	}
	digits[nondig] = 0;
	x.Val          = (float) atof(digits);
	if (nondig == len) {
		if (len == 1 && s[0] == '0') {
			// ok
		} else {
			ParseFail("Parse failed, invalid size: %.*s\n", (int) len, s);
			return false;
		}
	} else {
		if (s[nondig] == '%') {
			x.Type = Size::PERCENT;
		} else if (s[nondig] == 'p' && len - nondig >= 2) {
			if (s[nondig + 1] == 'x')
				x.Type = Size::PX;
			else if (s[nondig + 1] == 't')
				x.Type = Size::PT;
			else {
				ParseFail("Parse failed, invalid size: %.*s\n", (int) len, s);
				return false;
			}
		} else if (s[nondig] == 'e' && len - nondig >= 2 && s[nondig + 1] == 'p') {
			x.Type = Size::EP;
		} else if (s[nondig] == 'e' && len - nondig >= 2 && s[nondig + 1] == 'm') {
			x.Type = Size::EM;
		} else if (s[nondig] == 'e' && len - nondig >= 2 && s[nondig + 1] == 'x') {
			x.Type = Size::EX;
		} else {
			ParseFail("Parse failed, invalid size: %.*s\n", (int) len, s);
			return false;
		}
	}
	v = x;
	return true;
}

bool StyleBox::Parse(const char* s, size_t len, StyleBox& v) {
	int spaces[10];
	int nspaces = FindSpaces(s, len, spaces);

	// 20px
	StyleBox b;
	if (nspaces == 0) {
		Size one;
		if (Size::Parse(s, len, one)) {
			b.Left = b.Top = b.Bottom = b.Right = one;
			v                                   = b;
			return true;
		}
	}

	// 1px 2px 3px 4px
	if (nspaces == 3) {
		bool ok1 = Size::Parse(s, spaces[0], b.Left);
		bool ok2 = Size::Parse(s + spaces[0] + 1, spaces[1] - spaces[0] - 1, b.Top);
		bool ok3 = Size::Parse(s + spaces[1] + 1, spaces[2] - spaces[1] - 1, b.Right);
		bool ok4 = Size::Parse(s + spaces[2] + 1, (int) len - spaces[2] - 1, b.Bottom);
		if (ok1 && ok2 && ok3 && ok4) {
			v = b;
			return true;
		}
	}
	return false;
}

bool Color::Parse(const char* s, size_t len, Color& v) {
	Color c = Color::RGBA(0, 0, 0, 0);
	s++;
	// #rgb
	// #rgba
	// #rrggbb
	// #rrggbbaa
	if (len == 4) {
		c.r = ParseHexCharSingle(s + 0);
		c.g = ParseHexCharSingle(s + 1);
		c.b = ParseHexCharSingle(s + 2);
		c.a = 255;
		//Trace( "color %s -> %d\n", s, (int) c.r );
	} else if (len == 5) {
		c.r = ParseHexCharSingle(s + 0);
		c.g = ParseHexCharSingle(s + 1);
		c.b = ParseHexCharSingle(s + 2);
		c.a = ParseHexCharSingle(s + 3);
	} else if (len == 7) {
		c.r = ParseHexCharPair(s + 0);
		c.g = ParseHexCharPair(s + 2);
		c.b = ParseHexCharPair(s + 4);
		c.a = 255;
	} else if (len == 9) {
		c.r = ParseHexCharPair(s + 0);
		c.g = ParseHexCharPair(s + 2);
		c.b = ParseHexCharPair(s + 4);
		c.a = ParseHexCharPair(s + 6);
	} else {
		ParseFail("Parse failed, invalid color %.*s\n", (int) len, s);
		return false;
	}
	v = c;
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StyleAttrib::StyleAttrib() {
	memset(this, 0, sizeof(*this));
}

void StyleAttrib::SetU32(StyleCategories cat, uint32_t val) {
	Category = cat;
	ValU32   = val;
}

void StyleAttrib::SetWithSubtypeU32(StyleCategories cat, uint8_t subtype, uint32_t val) {
	Category = cat;
	SubType  = subtype;
	ValU32   = val;
}

void StyleAttrib::SetWithSubtypeF(StyleCategories cat, uint8_t subtype, float val) {
	Category = cat;
	SubType  = subtype;
	ValF     = val;
}

void StyleAttrib::SetString(StyleCategories cat, const char* str, Doc* doc) {
	Category = cat;
	ValU32   = doc->Strings.GetId(str);
}

void StyleAttrib::SetInherit(StyleCategories cat) {
	Category = cat;
	Flags    = FlagInherit;
}

const char* StyleAttrib::GetBackgroundImage(StringTable* strings) const {
	return strings->GetStr(ValU32);
}

FontID StyleAttrib::GetFont() const {
	return ValU32;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
static bool ParseSingleAttrib(const char* s, size_t len, bool (*parseFunc)(const char* s, size_t len, T& t), StyleCategories cat, Style& style) {
	T val;
	if (parseFunc(s, len, val)) {
		StyleAttrib attrib;
		attrib.Set(cat, val);
		style.Set(attrib);
		return true;
	} else {
		ParseFail("Parse failed, unknown value: '%.*s'\n", (int) len, s);
		return false;
	}
}

// This was added when font-family was stored as a string, but it is now stored as a FontID
static void ParseString(const char* s, size_t len, StyleCategories cat, Doc* doc, Style& style) {
	char        stat[64];
	StyleAttrib attrib;
	if (len < sizeof(stat)) {
		memcpy(stat, s, len);
		stat[len] = 0;
		attrib.Set(cat, stat, doc);
	} else {
		String copy;
		copy.Set(s, len);
		attrib.Set(cat, copy.Z, doc);
	}
	style.Set(attrib);
}

static bool ParseBool(const char* s, size_t len, StyleCategories cat, Style& style) {
	bool val;
	if (len == 4 && s[0] == 't')
		val = true;
	else if (len == 5 && s[0] == 'f')
		val = false;
	else {
		ParseFail("Parse failed, illegal boolean value: '%.*s'. Valid values are 'true' and 'false'.\n", (int) len, s);
		return false;
	}
	StyleAttrib attrib;
	attrib.SetBool(cat, val);
	style.Set(attrib);
	return true;
}

template <typename T>
static bool ParseCompound(const char* s, size_t len, bool (*parseFunc)(const char* s, size_t len, T& t), StyleCategories cat, Style& style) {
	T val;
	if (parseFunc(s, len, val)) {
		style.Set(cat, val);
		return true;
	} else {
		ParseFail("Parse failed, unknown value: '%.*s'\n", (int) len, s);
		return false;
	}
}

static bool ParseDirect(const char* s, size_t len, bool (*parseFunc)(const char* s, size_t len, Style& style), Style& style) {
	if (parseFunc(s, len, style)) {
		return true;
	} else {
		ParseFail("Parse failed on: '%.*s'\n", (int) len, s);
		return false;
	}
}

static bool ParseFontFamily(const char* s, size_t len, FontID& v) {
	bool   onFont = false;
	char   buf[64];
	size_t bufPos = 0;
	for (size_t i = 0; true; i++) {
		if (onFont) {
			if (s[i] == ',' || i == len) {
				buf[bufPos] = 0;
				v           = Global()->FontStore->InsertByFacename(buf);
				if (v != FontIDNull)
					return true;
				onFont = false;
				bufPos = 0;
			} else {
				buf[bufPos++] = s[i];
			}
			if (i == len)
				break;
		} else {
			if (i == len)
				break;
			if (IsWhitespace(s[i]))
				continue;
			onFont        = true;
			buf[bufPos++] = s[i];
		}
		if (bufPos >= arraysize(buf)) {
			ParseFail("Parse failed, font name too long (>63): '%*.s'\n", (int) len, s);
			return false;
		}
	}
	// not sure whether we should do this. One might want no font to be set instead.
	v = Global()->FontStore->GetFallbackFontID();
	return true;
}

bool Style::Parse(const char* t, Doc* doc) {
	return Parse(t, INT32_MAX, doc);
}

bool Style::Parse(const char* t, size_t maxLen, Doc* doc) {
// "background: #8f8; width: 100%; height: 100%;"
#define TSTART (t + startv)
#define TLEN (i - startv)
	size_t startk = -1;
	size_t eq     = -1;
	size_t startv = -1;
	int    nerror = 0;
	for (size_t i = 0; true; i++) {
		bool eof = t[i] == 0 || i == maxLen;
		if (t[i] == 32) {
		} else if (t[i] == ':')
			eq = i;
		else if (t[i] == ';' || (eof && startv != -1)) {
			// clang-format off
			bool ok = true;
			if (MATCH(t, startk, eq, "background"))		                { ok = ParseSingleAttrib(TSTART, TLEN, &Color::Parse, CatBackground, *this); }
			else if (MATCH(t, startk, eq, "color"))		                { ok = ParseSingleAttrib(TSTART, TLEN, &Color::Parse, CatColor, *this); }
			else if (MATCH(t, startk, eq, "width"))		                { ok = ParseSingleAttrib(TSTART, TLEN, &Size::Parse, CatWidth, *this); }
			else if (MATCH(t, startk, eq, "height"))		            { ok = ParseSingleAttrib(TSTART, TLEN, &Size::Parse, CatHeight, *this); }
			else if (MATCH(t, startk, eq, "padding"))		            { ok = ParseCompound(TSTART, TLEN, &StyleBox::Parse, CatPadding_Left, *this); }
			else if (MATCH(t, startk, eq, "margin"))		            { ok = ParseCompound(TSTART, TLEN, &StyleBox::Parse, CatMargin_Left, *this); }
			else if (MATCH(t, startk, eq, "position"))		            { ok = ParseSingleAttrib(TSTART, TLEN, &ParsePositionType, CatPosition, *this); }
			else if (MATCH(t, startk, eq, "border"))		            { ok = ParseDirect(TSTART, TLEN, &ParseBorder, *this); }
			else if (MATCH(t, startk, eq, "border-radius"))		        { ok = ParseSingleAttrib(TSTART, TLEN, &Size::Parse, CatBorderRadius, *this); }
			//else if ( MATCH(t, startk, eq, "left") )                  { ok = ParseSingleAttrib( TSTART, TLEN, &Size::Parse, CatLeft, *this ); }
			//else if ( MATCH(t, startk, eq, "right") )                 { ok = ParseSingleAttrib( TSTART, TLEN, &Size::Parse, CatRight, *this ); }
			//else if ( MATCH(t, startk, eq, "top") )                   { ok = ParseSingleAttrib( TSTART, TLEN, &Size::Parse, CatTop, *this ); }
			//else if ( MATCH(t, startk, eq, "bottom") )                { ok = ParseSingleAttrib( TSTART, TLEN, &Size::Parse, CatBottom, *this ); }
			else if (MATCH(t, startk, eq, "break"))                     { ok = ParseSingleAttrib(TSTART, TLEN, &ParseBreakType, CatBreak, *this); }
			else if (MATCH(t, startk, eq, "canfocus"))                  { ok = ParseBool(TSTART, TLEN, CatCanFocus, *this); }
			else if (MATCH(t, startk, eq, "cursor"))                    { ok = ParseSingleAttrib(TSTART, TLEN, &ParseCursor, CatCursor, *this); }
			else if (MATCH(t, startk, eq, "flow-context"))              { ok = ParseSingleAttrib(TSTART, TLEN, &ParseFlowContext, CatFlowContext, *this); }
			else if (MATCH(t, startk, eq, "flow-axis"))                 { ok = ParseSingleAttrib(TSTART, TLEN, &ParseFlowAxis, CatFlowAxis, *this); }
			else if (MATCH(t, startk, eq, "flow-direction-horizontal")) { ok = ParseSingleAttrib(TSTART, TLEN, &ParseFlowDirection, CatFlowDirection_Horizontal, *this); }
			else if (MATCH(t, startk, eq, "flow-direction-vertical"))   { ok = ParseSingleAttrib(TSTART, TLEN, &ParseFlowDirection, CatFlowDirection_Vertical, *this); }
			else if (MATCH(t, startk, eq, "box-sizing"))                { ok = ParseSingleAttrib(TSTART, TLEN, &ParseBoxSize, CatBoxSizing, *this); }
			else if (MATCH(t, startk, eq, "font-size"))                 { ok = ParseSingleAttrib(TSTART, TLEN, &Size::Parse, CatFontSize, *this); }
			else if (MATCH(t, startk, eq, "font-family"))               { ok = ParseSingleAttrib(TSTART, TLEN, &ParseFontFamily, CatFontFamily, *this); }
			else if (MATCH(t, startk, eq, "left"))                      { ok = ParseSingleAttrib(TSTART, TLEN, &ParseHorizontalBinding, CatLeft, *this); }
			else if (MATCH(t, startk, eq, "hcenter"))                   { ok = ParseSingleAttrib(TSTART, TLEN, &ParseHorizontalBinding, CatHCenter, *this); }
			else if (MATCH(t, startk, eq, "right"))                     { ok = ParseSingleAttrib(TSTART, TLEN, &ParseHorizontalBinding, CatRight, *this); }
			else if (MATCH(t, startk, eq, "top"))                       { ok = ParseSingleAttrib(TSTART, TLEN, &ParseVerticalBinding, CatTop, *this); }
			else if (MATCH(t, startk, eq, "vcenter"))                   { ok = ParseSingleAttrib(TSTART, TLEN, &ParseVerticalBinding, CatVCenter, *this); }
			else if (MATCH(t, startk, eq, "bottom"))                    { ok = ParseSingleAttrib(TSTART, TLEN, &ParseVerticalBinding, CatBottom, *this); }
			else if (MATCH(t, startk, eq, "baseline"))                  { ok = ParseSingleAttrib(TSTART, TLEN, &ParseVerticalBinding, CatBaseline, *this); }
			else if (MATCH(t, startk, eq, "bump"))                      { ok = ParseSingleAttrib(TSTART, TLEN, &ParseBump, CatBump, *this); }
			// clang-format on
			else {
				ok = false;
				ParseFail("Parse failed - unknown property: '%.*s'\n", int(eq - startk), t + startk);
			}
			if (!ok)
				nerror++;
			eq     = -1;
			startk = -1;
			startv = -1;
		} else {
			if (startk == -1)
				startk = i;
			else if (startv == -1 && eq != -1)
				startv = i;
		}

		if (eof)
			break;
	}
	return nerror == 0;
#undef TSTART
#undef TLEN
}

const StyleAttrib* Style::Get(StyleCategories cat) const {
	for (size_t i = 0; i < Attribs.size(); i++) {
		if (Attribs[i].Category == cat)
			return &Attribs[i];
	}
	return NULL;
}

void Style::GetBox(StyleCategories cat, StyleBox& box) const {
	StyleCategories base = CatMakeBaseBox(cat);
	for (size_t i = 0; i < Attribs.size(); i++) {
		uint32_t pindex = uint32_t(Attribs[i].Category - base);
		if (pindex < 4)
			box.All[pindex] = Attribs[i].GetSize();
	}
}

void Style::SetBox(StyleCategories cat, StyleBox val) {
	if (cat >= CatMargin_Left && cat <= CatBorder_Bottom) {
		SetBoxInternal(CatMakeBaseBox(cat), val);
	} else
		XO_ASSERT(false);
}

void Style::SetUniformBox(StyleCategories cat, StyleAttrib val) {
	cat          = CatMakeBaseBox(cat);
	val.Category = (StyleCategories)(cat + 0);
	Set(val);
	val.Category = (StyleCategories)(cat + 1);
	Set(val);
	val.Category = (StyleCategories)(cat + 2);
	Set(val);
	val.Category = (StyleCategories)(cat + 3);
	Set(val);
}

void Style::SetUniformBox(StyleCategories cat, Color color) {
	StyleAttrib val;
	val.SetColor(cat, color);
	SetUniformBox(cat, val);
}

void Style::SetUniformBox(StyleCategories cat, Size size) {
	StyleAttrib val;
	val.SetSize(cat, size);
	SetUniformBox(cat, val);
}

void Style::SetBoxInternal(StyleCategories catBase, StyleBox val) {
	StyleAttrib a;
	a.SetSize((StyleCategories)(catBase + 0), val.Left);
	Set(a);
	a.SetSize((StyleCategories)(catBase + 1), val.Top);
	Set(a);
	a.SetSize((StyleCategories)(catBase + 2), val.Right);
	Set(a);
	a.SetSize((StyleCategories)(catBase + 3), val.Bottom);
	Set(a);
}

void Style::Set(StyleAttrib attrib) {
	for (size_t i = 0; i < Attribs.size(); i++) {
		if (Attribs[i].Category == attrib.Category) {
			Attribs[i] = attrib;
			return;
		}
	}
	Attribs += attrib;
}

void Style::Set(StyleCategories cat, StyleBox val) {
	SetBox(cat, val);
}

void Style::Discard() {
	Attribs.clear_noalloc();
	//Name.Discard();
}

void Style::CloneSlowInto(Style& c) const {
	c.Attribs = Attribs;
}

void Style::CloneFastInto(Style& c, Pool* pool) const {
	//Name.CloneFastInto( c.Name, pool );
	ClonePodvecWithMemCopy(c.Attribs, Attribs, pool);
}

#define XX(name, type, setfunc, cat)   \
	\
void Style::Set##name(type value) \
{ \
		StyleAttrib a;                 \
		a.setfunc(cat, value);         \
		Set(a);                        \
	\
}
NUSTYLE_SETTERS_2P
#undef XX

#define XX(name, type, setfunc)        \
	\
void Style::Set##name(type value) \
{ \
		StyleAttrib a;                 \
		a.setfunc(value);              \
		Set(a);                        \
	\
}
NUSTYLE_SETTERS_1P
#undef XX

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StyleSet::StyleSet() {
	Reset();
}

StyleSet::~StyleSet() {
}

void StyleSet::Reset() {
	Lookup      = NULL;
	Attribs     = NULL;
	Count       = 0;
	Capacity    = 0;
	BitsPerSlot = 0;
	//SetSlotF = NULL;
	//GetSlotF = NULL;
}

void StyleSet::Grow(Pool* pool) {
	XO_ASSERT(BitsPerSlot != 8);
	uint32_t     newbits    = BitsPerSlot == 0 ? InitialBitsPerSlot : BitsPerSlot * 2;
	uint32_t     totalBits  = newbits * CatEND;
	void*        newlookup  = pool->Alloc((totalBits + 7) / 8, true);
	StyleAttrib* newattribs = pool->AllocNT<StyleAttrib>(CapacityAt(newbits), false);
	if (BitsPerSlot) {
		if (newbits == 4)
			MigrateLookup(Lookup, newlookup, &GetSlot2, &SetSlot4);
		else if (newbits == 8)
			MigrateLookup(Lookup, newlookup, &GetSlot4, &SetSlot8);

		memcpy(newattribs, Attribs, CapacityAt(BitsPerSlot) * sizeof(StyleAttrib));
	}
	Capacity    = CapacityAt(newbits);
	Lookup      = newlookup;
	Attribs     = newattribs;
	BitsPerSlot = newbits;
	//if (newbits == 2)		{ SetSlotF = &SetSlot2; GetSlotF = &GetSlot2; }
	//else if (newbits == 4)	{ SetSlotF = &SetSlot4; GetSlotF = &GetSlot4; }
	//else if (newbits == 8)	{ SetSlotF = &SetSlot8; GetSlotF = &GetSlot8; }
}

void StyleSet::Set(int n, const StyleAttrib* attribs, Pool* pool) {
	for (int i = 0; i < n; i++)
		Set(attribs[i], pool);
}

void StyleSet::Set(const StyleAttrib& attrib, Pool* pool) {
	int32_t slot = GetSlot(attrib.GetCategory());
	if (slot != 0) {
		Attribs[slot - 1] = attrib;
		return;
	}
	if (Count >= Capacity)
		Grow(pool);
	Attribs[Count] = attrib;
	SetSlot(attrib.GetCategory(), Count + SlotOffset);
	Count++;
	//DebugCheckSanity();
}

StyleAttrib StyleSet::Get(StyleCategories cat) const {
	int32_t slot = GetSlot(cat) - SlotOffset;
	if (slot == -1)
		return StyleAttrib();
	return Attribs[slot];
}

void StyleSet::EraseOrSetNull(StyleCategories cat) const {
	int32_t slot = GetSlot(cat) - SlotOffset;
	if (slot != -1)
		Attribs[slot] = StyleAttrib();
}

bool StyleSet::Contains(StyleCategories cat) const {
	return GetSlot(cat) != 0;
}

void StyleSet::MigrateLookup(const void* lutsrc, void* lutdst, GetSlotFunc getter, SetSlotFunc setter) {
	for (int i = CatFIRST; i < CatEND; i++) {
		int32_t slot = getter(lutsrc, (StyleCategories) i);
		if (slot != 0)
			setter(lutdst, (StyleCategories) i, slot);
	}
}

int32_t StyleSet::GetSlot(StyleCategories cat) const {
	if (Lookup == nullptr)
		return 0;
	GetSlotFunc f = (BitsPerSlot == 2) ? &GetSlot2 : (BitsPerSlot == 4 ? &GetSlot4 : &GetSlot8);
	return f(Lookup, cat);
}

void StyleSet::SetSlot(StyleCategories cat, int32_t slot) {
	SetSlotFunc f = (BitsPerSlot == 2) ? &SetSlot2 : (BitsPerSlot == 4 ? &SetSlot4 : &SetSlot8);
	f(Lookup, cat, slot);
}

void StyleSet::DebugCheckSanity() const {
	for (int i = CatFIRST; i < CatEND; i++) {
		StyleCategories cat = (StyleCategories) i;
		StyleAttrib     val = Get(cat);
		XO_DEBUG_ASSERT(val.IsNull() || val.Category == cat);
	}
}

template <uint32_t BITS_PER_SLOT>
void StyleSet::TSetSlot(void* lookup, StyleCategories cat, int32_t slot) {
	const uint32_t mask    = (1 << BITS_PER_SLOT) - 1;
	uint8_t*       lookup8 = (uint8_t*) lookup;

	if (BITS_PER_SLOT == 8) {
		lookup8[cat] = slot;
	} else {
		uint32_t intra_byte_mask;
		uint32_t ibyte;
		if (BITS_PER_SLOT == 2) {
			ibyte           = ((uint32_t) cat) >> 2;
			intra_byte_mask = 3;
		} else if (BITS_PER_SLOT == 4) {
			ibyte           = ((uint32_t) cat) >> 1;
			intra_byte_mask = 1;
		}

		uint8_t v           = lookup8[ibyte];
		uint8_t islotinbyte = ((uint32_t) cat) & intra_byte_mask;
		uint8_t ishift      = islotinbyte * BITS_PER_SLOT;
		uint8_t shiftedmask = mask << ishift;
		v                   = v & ~shiftedmask;
		v                   = v | (((uint32_t) slot) << ishift);
		lookup8[ibyte]      = v;
	}
}

template <uint32_t BITS_PER_SLOT>
int32_t StyleSet::TGetSlot(const void* lookup, StyleCategories cat) {
	const uint32_t mask    = (1 << BITS_PER_SLOT) - 1;
	const uint8_t* lookup8 = (const uint8_t*) lookup;

	if (BITS_PER_SLOT == 8) {
		return lookup8[cat];
	} else {
		uint32_t intra_byte_mask;
		uint32_t ibyte;
		if (BITS_PER_SLOT == 2) {
			ibyte           = ((uint32_t) cat) >> 2;
			intra_byte_mask = 3;
		} else if (BITS_PER_SLOT == 4) {
			ibyte           = ((uint32_t) cat) >> 1;
			intra_byte_mask = 1;
		}

		uint8_t v           = lookup8[ibyte];
		uint8_t islotinbyte = ((uint32_t) cat) & intra_byte_mask;
		uint8_t ishift      = islotinbyte * BITS_PER_SLOT;
		uint8_t shiftedmask = mask << ishift;
		v                   = (v & shiftedmask) >> ishift;
		return v;
	}
}

int32_t StyleSet::GetSlot2(const void* lookup, StyleCategories cat) { return TGetSlot<2>(lookup, cat); }
int32_t StyleSet::GetSlot4(const void* lookup, StyleCategories cat) { return TGetSlot<4>(lookup, cat); }
int32_t StyleSet::GetSlot8(const void* lookup, StyleCategories cat) { return TGetSlot<8>(lookup, cat); }

void StyleSet::SetSlot2(void* lookup, StyleCategories cat, int32_t slot) { TSetSlot<2>(lookup, cat, slot); }
void StyleSet::SetSlot4(void* lookup, StyleCategories cat, int32_t slot) { TSetSlot<4>(lookup, cat, slot); }
void StyleSet::SetSlot8(void* lookup, StyleCategories cat, int32_t slot) { TSetSlot<8>(lookup, cat, slot); }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StyleTable::StyleTable() {
}

StyleTable::~StyleTable() {
}

void StyleTable::AddDummyStyleZero() {
	GetOrCreate("");
}

void StyleTable::Discard() {
	Classes.discard();
	Names.discard();
}

const StyleClass* StyleTable::GetByID(StyleClassID id) const {
	return &Classes[id];
}

StyleClass* StyleTable::GetOrCreate(const char* name) {
	TempString n(name);
	// find existing
	int* pindex = NameToIndex.getp(n);
	if (pindex)
		return &Classes[*pindex];

	// create new
	int index = (int) Classes.size();
	Classes.add();
	Names.add();
	StyleClass* s = &Classes[index];
	NameToIndex.insert(n, index);
	Names[index] = String(name);
	return s;
}

StyleClassID StyleTable::GetClassID(const char* name) {
	TempString n(name);
	int*       pindex = NameToIndex.getp(n);
	if (pindex)
		return StyleClassID(*pindex);
	else
		return StyleClassID(0);
}

void StyleTable::CloneSlowInto(StyleTable& c) const {
	// The renderer doesn't need a Name -> ID table. That lookup table is only for end-user convenience.
	c.Classes = Classes;
}

void StyleTable::CloneFastInto(StyleTable& c, Pool* pool) const {
	// The renderer doesn't need a Name -> ID table. That lookup table is only for end-user convenience.
	ClonePodvecWithMemCopy(c.Classes, Classes, pool);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

XO_API bool ParsePositionType(const char* s, size_t len, PositionType& t) {
	if (MATCH(s, 0, len, "static")) {
		t = PositionStatic;
		return true;
	}
	if (MATCH(s, 0, len, "absolute")) {
		t = PositionAbsolute;
		return true;
	}
	if (MATCH(s, 0, len, "relative")) {
		t = PositionRelative;
		return true;
	}
	if (MATCH(s, 0, len, "fixed")) {
		t = PositionFixed;
		return true;
	}
	return false;
}

XO_API bool ParseBreakType(const char* s, size_t len, BreakType& t) {
	if (MATCH(s, 0, len, "none")) {
		t = BreakNULL;
		return true;
	}
	if (MATCH(s, 0, len, "before")) {
		t = BreakBefore;
		return true;
	}
	if (MATCH(s, 0, len, "after")) {
		t = BreakAfter;
		return true;
	}
	return false;
}

XO_API bool ParseCursor(const char* s, size_t len, Cursors& t) {
	if (MATCH(s, 0, len, "arrow")) {
		t = CursorArrow;
		return true;
	}
	if (MATCH(s, 0, len, "hand")) {
		t = CursorHand;
		return true;
	}
	if (MATCH(s, 0, len, "text")) {
		t = CursorText;
		return true;
	}
	if (MATCH(s, 0, len, "wait")) {
		t = CursorWait;
		return true;
	}
	return false;
}

XO_API bool ParseFlowContext(const char* s, size_t len, FlowContext& t) {
	if (MATCH(s, 0, len, "new")) {
		t = FlowContextNew;
		return true;
	}
	if (MATCH(s, 0, len, "inject")) {
		t = FlowContextInject;
		return true;
	}
	return false;
}

XO_API bool ParseFlowAxis(const char* s, size_t len, FlowAxis& t) {
	if (MATCH(s, 0, len, "horizontal")) {
		t = FlowAxisHorizontal;
		return true;
	}
	if (MATCH(s, 0, len, "vertical")) {
		t = FlowAxisVertical;
		return true;
	}
	return false;
}

XO_API bool ParseFlowDirection(const char* s, size_t len, FlowDirection& t) {
	if (MATCH(s, 0, len, "normal")) {
		t = FlowDirectionNormal;
		return true;
	}
	if (MATCH(s, 0, len, "reverse")) {
		t = FlowDirectionReversed;
		return true;
	}
	return false;
}

XO_API bool ParseBoxSize(const char* s, size_t len, BoxSizeType& t) {
	if (MATCH(s, 0, len, "content")) {
		t = BoxSizeContent;
		return true;
	}
	if (MATCH(s, 0, len, "border")) {
		t = BoxSizeBorder;
		return true;
	}
	if (MATCH(s, 0, len, "margin")) {
		t = BoxSizeMargin;
		return true;
	}
	return false;
}

XO_API bool ParseHorizontalBinding(const char* s, size_t len, HorizontalBindings& t) {
	if (MATCH(s, 0, len, "none")) {
		t = HorizontalBindingNULL;
		return true;
	}
	if (MATCH(s, 0, len, "left")) {
		t = HorizontalBindingLeft;
		return true;
	}
	if (MATCH(s, 0, len, "hcenter")) {
		t = HorizontalBindingCenter;
		return true;
	}
	if (MATCH(s, 0, len, "right")) {
		t = HorizontalBindingRight;
		return true;
	}
	return false;
}

XO_API bool ParseVerticalBinding(const char* s, size_t len, VerticalBindings& t) {
	if (MATCH(s, 0, len, "none")) {
		t = VerticalBindingNULL;
		return true;
	}
	if (MATCH(s, 0, len, "top")) {
		t = VerticalBindingTop;
		return true;
	}
	if (MATCH(s, 0, len, "vcenter")) {
		t = VerticalBindingCenter;
		return true;
	}
	if (MATCH(s, 0, len, "bottom")) {
		t = VerticalBindingBottom;
		return true;
	}
	if (MATCH(s, 0, len, "baseline")) {
		t = VerticalBindingBaseline;
		return true;
	}
	return false;
}

XO_API bool ParseBump(const char* s, size_t len, BumpStyle& t) {
	if (MATCH(s, 0, len, "regular")) {
		t = BumpRegular;
		return true;
	}
	if (MATCH(s, 0, len, "horizontal")) {
		t = BumpHorzOnly;
		return true;
	}
	if (MATCH(s, 0, len, "vertical")) {
		t = BumpVertOnly;
		return true;
	}
	if (MATCH(s, 0, len, "none")) {
		t = BumpNone;
		return true;
	}
	return false;
}

XO_API bool ParseBorder(const char* s, size_t len, Style& style) {
	int spaces[10];
	int nspaces = FindSpaces(s, len, spaces);

	if (nspaces == 0) {
		// 1px		OR
		// #000
		Color color;
		if (Color::Parse(s, len, color)) {
			style.SetUniformBox(CatBorderColor_Left, color);
			return true;
		}
		Size size;
		if (Size::Parse(s, len, size)) {
			style.SetBox(CatBorder_Left, StyleBox::MakeUniform(size));
			return true;
		}
	} else if (nspaces == 1) {
		// 1px #000
		Size  size;
		Color color;
		if (Size::Parse(s, spaces[0], size)) {
			if (Color::Parse(s + spaces[0] + 1, len - spaces[0] - 1, color)) {
				style.SetBox(CatBorder_Left, StyleBox::MakeUniform(size));
				style.SetUniformBox(CatBorderColor_Left, color);
				return true;
			}
		}
	} else if (nspaces == 3) {
		// 1px 2px 3px 4px
		StyleBox box;
		bool     s1 = Size::Parse(s, spaces[0], box.Left);
		bool     s2 = Size::Parse(s + spaces[0] + 1, spaces[1] - spaces[0] - 1, box.Top);
		bool     s3 = Size::Parse(s + spaces[1] + 1, spaces[2] - spaces[1] - 1, box.Right);
		bool     s4 = Size::Parse(s + spaces[2] + 1, len - spaces[2] - 1, box.Bottom);
		if (s1 && s2 && s3 && s4) {
			style.SetBox(CatBorder_Left, box);
			return true;
		}
	}

	return false;
}
}
