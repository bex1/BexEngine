#include "graphics.h"

//=============================================================================
// Constructor
//=============================================================================
GraphicsSystem::GraphicsSystem()
{
	direct3d = nullptr;
	device3d = nullptr;
	fullscreen = false;
	// width & height are replaced in initialize()
	width = GAME_WIDTH;    
	height = GAME_HEIGHT;
	// dark blue
	backColor = SETCOLOR_ARGB(255, 0, 0, 128); 
}

//=============================================================================
// Destructor
//=============================================================================
GraphicsSystem::~GraphicsSystem()
{
	releaseAll();
}

//=============================================================================
// Release all
//=============================================================================
void GraphicsSystem::releaseAll()
{
	SAFE_RELEASE(device3d);
	SAFE_RELEASE(direct3d);
}

//=============================================================================
// Initialize DirectX graphics
// throws GameError on error
//=============================================================================
void GraphicsSystem::initialize(HWND hw, int w, int h, bool full)
{
	hwnd = hw;
	width = w;
	height = h;
	fullscreen = full;

	//initialize Direct3D
	direct3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (!direct3d)
		throw(GameError(gameErrorNS::FATAL_ERROR, "Error initializing Direct3D"));

	// init D3D presentation parameters
	initD3DPresentaionParameters();

	// handle any graphics compatibility issues and return concluded device behavior
	DWORD behavior = handleGraphicsCompatibility();

	//create Direct3D device
	result = direct3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, behavior, &d3dpp, &device3d);

	if (FAILED(result))
		throw(GameError(gameErrorNS::FATAL_ERROR, "Error creating Direct3D device"));

}

//=============================================================================
// Initialize D3D presentation parameters
//=============================================================================
void GraphicsSystem::initD3DPresentaionParameters()
{
	try{
		// fill the structure with 0
		ZeroMemory(&d3dpp, sizeof(d3dpp));
		// fill in the parameters we need
		d3dpp.BackBufferWidth = width;
		d3dpp.BackBufferHeight = height;
		// if fullscreen
		if (fullscreen)
			// 24 bit color                                 
			d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
		else
			// use desktop setting
			d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
		d3dpp.BackBufferCount = 1;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.hDeviceWindow = hwnd;
		d3dpp.Windowed = (!fullscreen);
		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	}
	catch (...)
	{
		throw(GameError(gameErrorNS::FATAL_ERROR,
			"Error initializing D3D presentation parameters"));

	}
}

//=============================================================================
// Display the backbuffer
//=============================================================================
HRESULT GraphicsSystem::showBackbuffer()
{
	// default to fail, replace on success
	result = E_FAIL;
	// Display backbuffer to screen
	result = device3d->Present(nullptr, nullptr, nullptr, nullptr);
	return result;
}

//=============================================================================
// Checks the adapter to see if it is compatible with the BackBuffer height,
// width and refresh rate specified in d3dpp. Fills in the pMode structure with
// the format of the compatible mode, if found.
// Pre: d3dpp is initialized.
// Post: Returns true if compatible mode found and pMode structure is filled.
//       Returns false if no compatible mode found.
//=============================================================================
bool GraphicsSystem::isAdapterCompatible()
{
	UINT modes = direct3d->GetAdapterModeCount(D3DADAPTER_DEFAULT, d3dpp.BackBufferFormat);
	for (UINT i = 0; i < modes; ++i)
	{
		result = direct3d->EnumAdapterModes(D3DADAPTER_DEFAULT, d3dpp.BackBufferFormat, i, &pMode);
		if (pMode.Height == d3dpp.BackBufferHeight && pMode.Width == d3dpp.BackBufferWidth && pMode.RefreshRate >= d3dpp.FullScreen_RefreshRateInHz)
			return true;
	}
	return false;
}

//=============================================================================
// Test for lost device
//=============================================================================
HRESULT GraphicsSystem::getDeviceState()
{
	// default to fail, replace on success
	result = E_FAIL;    
	if (device3d == nullptr)
		return  result;
	result = device3d->TestCooperativeLevel();
	return result;
}

//=============================================================================
// Reset the graphics device
//=============================================================================
HRESULT GraphicsSystem::reset()
{
	// default to fail, replace on success
	result = E_FAIL;    
	// init D3D presentation parameters
	initD3DPresentaionParameters();                        
	// attempt to reset graphics device
	result = device3d->Reset(&d3dpp);   
	return result;
}

//=============================================================================
// Handle any graphics compatibility issues and return concluded device behavior
//=============================================================================
DWORD GraphicsSystem::handleGraphicsCompatibility()
{
	// if full-screen mode
	if (fullscreen)
	{
		// is the adapter compatible
		if (isAdapterCompatible())
			// set the refresh rate with a compatible one
			d3dpp.FullScreen_RefreshRateInHz = pMode.RefreshRate;
		else
			throw(GameError(gameErrorNS::FATAL_ERROR,
			"The graphics device does not support the specified resolution and/or format."));
	}

	// determine if graphics card supports hardware texturing and lighting and vertex shaders
	D3DCAPS9 caps;
	result = direct3d->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
	// If device doesn't support HW T&L or doesn't support 1.1 vertex 
	// shaders in hardware, then switch to software vertex processing.
	if ((caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 || caps.VertexShaderVersion < D3DVS_VERSION(1, 1))
		// use software only processing
		 return D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	else
		// use hardware only processing
		return D3DCREATE_HARDWARE_VERTEXPROCESSING;
}

