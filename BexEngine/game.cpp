#include "game.h"

// The primary class should inherit from Game class

//=============================================================================
// Constructor
//=============================================================================
Game::Game() : paused(false), initialized(false)
{
	// additional initialization is handled in later call to input.initialize()
}

//=============================================================================
// Destructor
//=============================================================================
Game::~Game()
{
	deleteAll();                // free all reserved memory
	ShowCursor(true);           // show cursor
}

//=============================================================================
// Window message handler
//=============================================================================
LRESULT Game::messageHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (initialized)     // do not process messages if not initialized
	{
		switch (msg)
		{
		case WM_DESTROY:
			PostQuitMessage(0);        //tell Windows to kill this program
			return 0;
		case WM_KEYDOWN: case WM_SYSKEYDOWN:    // key down
			input.keyDown(wParam);
			return 0;
		case WM_KEYUP: case WM_SYSKEYUP:        // key up
			input.keyUp(wParam);
			return 0;
		case WM_CHAR:                           // character entered
			input.keyIn(wParam);
			return 0;
		case WM_MOUSEMOVE:                      // mouse moved
			input.mouseIn(lParam);
			return 0;
		case WM_INPUT:                          // raw mouse data in
			input.mouseRawIn(lParam);
			return 0;
		case WM_LBUTTONDOWN:                    // left mouse button down
			input.setMouseLButton(true);
			input.mouseIn(lParam);             // mouse position
			return 0;
		case WM_LBUTTONUP:                      // left mouse button up
			input.setMouseLButton(false);
			input.mouseIn(lParam);             // mouse position
			return 0;
		case WM_MBUTTONDOWN:                    // middle mouse button down
			input.setMouseMButton(true);
			input.mouseIn(lParam);             // mouse position
			return 0;
		case WM_MBUTTONUP:                      // middle mouse button up
			input.setMouseMButton(false);
			input.mouseIn(lParam);             // mouse position
			return 0;
		case WM_RBUTTONDOWN:                    // right mouse button down
			input.setMouseRButton(true);
			input.mouseIn(lParam);             // mouse position
			return 0;
		case WM_RBUTTONUP:                      // right mouse button up
			input.setMouseRButton(false);
			input.mouseIn(lParam);             // mouse position
			return 0;
		case WM_XBUTTONDOWN: case WM_XBUTTONUP: // mouse X button down/up
			input.setMouseXButton(wParam);
			input.mouseIn(lParam);             // mouse position
			return 0;
		case WM_DEVICECHANGE:                   // check for controller insert
			input.checkControllers();
			return 0;
		}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);    // let Windows handle it
}

//=============================================================================
// Initializes the game
// throws GameError on error
//=============================================================================
void Game::initialize(HWND hw)
{
	// save window handle
	hwnd = hw;                                  

	// throws GameError
	graphics.initialize(hwnd, GAME_WIDTH, GAME_HEIGHT, FULLSCREEN);

	// initialize input, do not capture mouse
	// throws GameError
	input.initialize(hwnd, false);             

	// attempt to set up high resolution timer
	if (QueryPerformanceFrequency(&timerFreq) == false)
		throw(GameError(gameErrorNS::FATAL_ERROR, "Error initializing high resolution timer"));

	// get starting time
	QueryPerformanceCounter(&timeStart);        

	initialized = true;
}

//=============================================================================
// Render game items
//=============================================================================
void Game::renderGame()
{
	//start rendering
	if (SUCCEEDED(graphics.beginScene()))
	{
		// render is a pure virtual function that must be provided in the
		// inheriting class.
		// call render in derived class
		render();               

		//stop rendering
		graphics.endScene();
	}
	handleLostGraphicsDevice();

	//display the back buffer on the screen
	graphics.showBackbuffer();
}

//=============================================================================
// Handle lost graphics device
//=============================================================================
void Game::handleLostGraphicsDevice()
{
	// test for and handle lost device
	hr = graphics.getDeviceState();
	// if graphics device is not in a valid state
	if (FAILED(hr))                  
	{
		// if the device is lost and not available for reset
		if (hr == D3DERR_DEVICELOST)
		{
			// yield cpu time (100 mili-seconds)
			Sleep(100);             
			return;
		}
		// the device was lost but is now available for reset
		else if (hr == D3DERR_DEVICENOTRESET)
		{
			releaseAll();
			// attempt to reset graphics device
			hr = graphics.reset(); 
			// if reset failed
			if (FAILED(hr))          
				return;
			resetAll();
		}
		else
			// other device error
			return;                 
	}
}


WPARAM Game::gameLoop(HWND hwnd)
{
	MSG msg;
	// main message loop
	bool running = true;
	while (running)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			// look for quit message
			if (msg.message == WM_QUIT)
				running = false;

			// decode and pass messages on to WinProc
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			run(hwnd);
		}
	}
	return msg.wParam;
}

//=============================================================================
// Calls to run are repeatedly done by the game loop 
//=============================================================================
void Game::run(HWND hwnd) {
	// Check if its time to update, if not just return
	if (!timeToUpdate())
		return;

	// update(), ai(), and collisions() are pure virtual functions.
	// These functions must be provided in the class that inherits from Game.
	// if not paused
	if (!paused)                    
	{
		// update all game items
		update();
		// artificial intelligence                   
		ai(); 
		// handle collisions                      
		collisions();     
		// handle controller vibration          
		input.vibrateControllers(frameTime); 
	}
	// draw all game items
	renderGame();       
	// read state of controllers            
	input.readControllers();       

	// Clear input
	// Call this after all key checks are done
	input.clear(inputNS::KEYS_PRESSED);
}

bool Game::timeToUpdate() {
	// calculate elapsed time of last frame, save in frameTime
	QueryPerformanceCounter(&timeEnd);
	frameTime = (float)(timeEnd.QuadPart - timeStart.QuadPart) / (float)timerFreq.QuadPart;

	// Power saving code, requires winmm.lib
	// if not enough time has elapsed for desired frame rate
	// Will wastly decrease overall cpu cycles
	if (frameTime < MIN_FRAME_TIME)
	{
		sleepTime = (DWORD)((MIN_FRAME_TIME - frameTime) * 1000);
		timeBeginPeriod(1);         // Request 1mS resolution for windows timer
		Sleep(sleepTime);           // release cpu for sleepTime
		timeEndPeriod(1);           // End 1mS timer resolution
		return false;
	}

	if (frameTime > 0.0)
		// average fps
		fps = (fps*0.99f) + (0.01f / frameTime);  
	
	// if frame rate is very slow
	if (frameTime > MAX_FRAME_TIME) 
		// limit maximum frameTime
		frameTime = MAX_FRAME_TIME; 

	timeStart = timeEnd;
	return true;
}

//=============================================================================
// The graphics device was lost.
// Release all reserved video memory so graphics device may be reset.
//=============================================================================
void Game::releaseAll()
{}

//=============================================================================
// Recreate all surfaces and reset all entities.
//=============================================================================
void Game::resetAll()
{}

//=============================================================================
// Delete all reserved memory
//=============================================================================
void Game::deleteAll()
{
	releaseAll();               // call onLostDevice() for every graphics item
	initialized = false;
}