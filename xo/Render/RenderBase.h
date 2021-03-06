#pragma once
#include "../Defs.h"
#include "VertexTypes.h"

namespace xo {

// This is a mess -- ShaderPerFrame and ShaderPerObject are actually DirectX specific.
// We must investigate whether we can make them uniform across GL and DirectX, but that
// would necessitate having the same matrix layout (ie column or row major).
struct ShaderPerFrame {
	Mat4f MVProj;      // Transposed for DirectX
	Vec2f VPort_HSize; // Half the size of the viewport
	float Padding[2];
};
static_assert((sizeof(ShaderPerFrame) & 15) == 0, "ShaderPerFrame size must be a multiple of 16 bytes (This is a DirectX constant buffer requirement)");

struct ShaderPerObject {
	Vec4f Box;
	Vec4f Border;
	Vec4f BorderColor;
	float Radius;
	Vec2f Edges;
	float Padding[1];
	Vec4f ShadowColor;
	Vec2f ShadowOffset;
	float ShadowSizeInv;
	Vec2f OutVector;
	float Padding2[7];
};
static_assert((sizeof(ShaderPerObject) & 15) == 0, "ShaderPerFrame size must be a multiple of 16 bytes (This is a DirectX constant buffer requirement)");

enum GPUPrimitiveTypes {
	GPUPrimQuads,
	GPUPrimTriangles,
};

/* Base class of device-specific renderer (such as GL or DX).

Texture IDs
-----------
Texture IDs are unsigned 32-bit integers that start at 1 and go up to 2^32-1.
When we "lose" a graphics surface, then we continue to issue texture IDs in the same
sequence that we previously were issuing them. However, during a graphics device
lost event, we record the first ID that is valid. Any IDs lower than that are not
valid. This is not bullet-proof, because you could have some very very old ID
that is not detected as invalid.

For example, say the very first texture you allocate gets assigned ID 1. Then,
a long time goes on, and very many "lost" surface events occur, and eventually
you wrap back around to the start, and you issue texture ID 1 again to a new
caller. The original caller would now believe his old ID 1 is a real texture.
I am hoping that the probability of this occurring is next to zero. Perhaps
a wise thing to do is to force all resource holders to clean up their garbage
at the start of every frame - that would guarantee that such an unlikely
calamity becomes impossible.

*/
class XO_API RenderBase {
public:
	friend struct RenderBase_OnceOff;

	ShaderPerFrame  ShaderPerFrame; // This is a mess between DirectX and OpenGL. needs cleanup
	ShaderPerObject ShaderPerObject;
	Mat4f           MVProj;

	RenderBase();
	virtual ~RenderBase();

	// Setup a matrix equivalent to glOrtho. The matrix 'imat' is multiplied by the ortho matrix.
	static void Ortho(Mat4f& imat, double left, double right, double bottom, double top, double znear, double zfar);

	// Project a point 'v' through ShaderPerFrame.MVProj
	void  SetupToScreen(Mat4f mvproj);
	Vec2f ToScreen(Vec2f v);

	void      SurfaceLost_ForgetTextures();
	bool      IsTextureValid(TextureID texID) const;
	TextureID FirstTextureID() const { return TexIDOffset + TEX_OFFSET_ONE; }

	// Register a new texture. There is no "unregister".
	TextureID RegisterTexture(uintptr_t deviceTexID);
	TextureID RegisterTextureInt(uint32_t deviceTexID) { return RegisterTexture((uintptr_t) deviceTexID); }
	uintptr_t GetTextureDeviceHandle(TextureID texID) const;
	uint32_t  GetTextureDeviceHandleInt(TextureID texID) const { return (uint32_t)(uintptr_t)(GetTextureDeviceHandle(texID)); }

	virtual const char* RendererName() = 0;

	virtual bool InitializeDevice(SysWnd& wnd) = 0; // Initialize this device
	virtual void DestroyDevice(SysWnd& wnd)    = 0; // Destroy this device and all associated textures, etc
	virtual void SurfaceLost()                 = 0;

	virtual bool BeginRender(SysWnd& wnd)                        = 0; // Start of a frame
	virtual void EndRender(SysWnd& wnd, uint32_t endRenderFlags) = 0; // Frame is finished. Present it (or possibly not, depending on flags).

	virtual void PreRender()         = 0;
	virtual void PostRenderCleanup() = 0;

	virtual ProgBase* GetShader(Shaders shader)      = 0;
	virtual void      ActivateShader(Shaders shader) = 0;

	virtual void Draw(GPUPrimitiveTypes type, int nvertex, const void* v) = 0;

	virtual bool LoadTexture(Texture* tex, int texUnit) = 0;
	virtual bool ReadBackbuffer(Image& image)           = 0;

protected:
	static const TextureID TEX_OFFSET_ONE = 1; // This constant causes the TextureID that we expose to never be zero.
	TextureID              TexIDOffset;
	cheapvec<uintptr_t>    TexIDToNative; // Maps from TextureID to native device texture (eg. GLuint or ID3D11Texture2D*). We're wasting 4 bytes here on OpenGL.
	int                    FBWidth, FBHeight;

	void        EnsureTextureProperlyDefined(Texture* tex, int texUnit);
	std::string CommonShaderDefines();
};

// This reduces the amount of #ifdef-ing needed, so that on non-Windows platforms
// we can still have a RenderDX class defined.
class XO_API RenderDummy {
public:
	virtual bool InitializeDevice(SysWnd& wnd);
	virtual void DestroyDevice(SysWnd& wnd);
	virtual void SurfaceLost();

	virtual bool BeginRender(SysWnd& wnd);
	virtual void EndRender(SysWnd& wnd, uint32_t endRenderFlags);

	virtual void PreRender();
	virtual void PostRenderCleanup();

	virtual ProgBase* GetShader(Shaders shader);
	virtual void      ActivateShader(Shaders shader);

	virtual void Draw(GPUPrimitiveTypes type, int nvertex, const void* v);

	virtual bool LoadTexture(Texture* tex, int texUnit);
	virtual bool ReadBackbuffer(Image& image);
};
}
