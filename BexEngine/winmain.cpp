#pragma once
#define _CRTDBG_MAP_ALLOC       // for detecting memory leaks
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <stdlib.h>             // for detecting memory leaks
#include <crtdbg.h>             // for detecting memory leaks
#include "spaceWar.h"

// Function prototypes
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
bool CreateMainWindow(HWND &, HINSTANCE, int);
LRESULT WINAPI WinProc(HWND, UINT, WPARAM, LPARAM);
void SetupWindowStruct(WNDCLASSEX &wcx, HINSTANCE hInstance);
DWORD DetermineScreenMode(bool fullScreen);
void AdjustWindowedSize();
void MakeWindow(HWND &hwnd, DWORD style, HINSTANCE hInstance);

// Game pointer
Game *game = nullptr;
HWND hwnd = nullptr;

//=============================================================================
// Starting point for a Windows application
//=============================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	// Check for memory leak if debug build
	#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	#endif

	// Create the game
	game = new Spacewar;

	// Create the window
	if (!CreateMainWindow(hwnd, hInstance, nCmdShow))
		return 1;

	try{
		game->initialize(hwnd);     // throws GameError
		
		auto msgParam = game->gameLoop(hwnd); 
		
		SAFE_DELETE(game);     // free memory before exit
		return msgParam;
	}
	catch (const GameError &err)
	{
		game->deleteAll();
		DestroyWindow(hwnd);
		MessageBox(nullptr, err.getMessage(), "Error", MB_OK);
	}
	catch (...)
	{
		game->deleteAll();
		DestroyWindow(hwnd);
		MessageBox(nullptr, "Unknown error occured in game.", "Error", MB_OK);
	}
	SAFE_DELETE(game);     // free memory before exit
	return 0;
}

//=============================================================================
// window event callback function
//=============================================================================
LRESULT WINAPI WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return (game->messageHandler(hwnd, msg, wParam, lParam));
}




//=============================================================================
// Create the window
// Returns: false on error
//=============================================================================
bool CreateMainWindow(HWND &hwnd, HINSTANCE hInstance, int nCmdShow)
{
	WNDCLASSEX wcx;
	SetupWindowStruct(wcx, hInstance);
	// Register the window class. 
	// RegisterClassEx returns 0 on error.
	if (!RegisterClassEx(&wcx))    // if error
		return false;

	//set up the screen in windowed or fullscreen mode?
	DWORD style = DetermineScreenMode(FULLSCREEN);

	// Create window
	MakeWindow(hwnd, style, hInstance);

	// if there was an error creating the window
	if (!hwnd)
		return false;
	
	// if window the size has to be adjusted
	if (!FULLSCREEN)            
	{
		AdjustWindowedSize();
	}

	// Show the window
	ShowWindow(hwnd, nCmdShow);

	return true;
}

void SetupWindowStruct(WNDCLASSEX &wcx, HINSTANCE hInstance)
{
	// Fill in the window class structure with parameters 
	// that describe the main window. 
	wcx.cbSize = sizeof(wcx);           // size of structure 
	wcx.style = CS_HREDRAW | CS_VREDRAW;    // redraw if size changes 
	wcx.lpfnWndProc = WinProc;          // points to window procedure 
	wcx.cbClsExtra = 0;                 // no extra class memory 
	wcx.cbWndExtra = 0;                 // no extra window memory 
	wcx.hInstance = hInstance;          // handle to instance 
	wcx.hIcon = nullptr;
	wcx.hCursor = LoadCursor(nullptr, IDC_ARROW);   // predefined arrow 
	wcx.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);    // black background 
	wcx.lpszMenuName = nullptr;           // name of menu resource 
	wcx.lpszClassName = CLASS_NAME.c_str();     // name of window class 
	wcx.hIconSm = nullptr;                 // small class icon 
}

DWORD DetermineScreenMode(bool fullScreen)
{
	if (fullScreen)
		return WS_EX_TOPMOST | WS_VISIBLE | WS_POPUP;
	else
		return WS_OVERLAPPEDWINDOW;
}

void AdjustWindowedSize()
{
	// Adjust window size so client area is GAME_WIDTH x GAME_HEIGHT
	RECT clientRect;
	GetClientRect(hwnd, &clientRect);   // get size of client area of window
	MoveWindow(hwnd,
		0,                                           // Left
		0,                                           // Top
		GAME_WIDTH + (GAME_WIDTH - clientRect.right),    // Right
		GAME_HEIGHT + (GAME_HEIGHT - clientRect.bottom), // Bottom
		TRUE);                                       // Repaint the window
}

void MakeWindow(HWND &hwnd, DWORD style, HINSTANCE hInstance)
{
	hwnd = CreateWindow(
		CLASS_NAME.c_str(),             // name of the window class
		GAME_TITLE.c_str(),             // title bar text
		style,                  // window style
		CW_USEDEFAULT,          // default horizontal position of window
		CW_USEDEFAULT,          // default vertical position of window
		GAME_WIDTH,             // width of window
		GAME_HEIGHT,            // height of the window
		nullptr,            // no parent window
		nullptr,           // no menu
		hInstance,              // handle to application instance
		nullptr);         // no window parameters
}
