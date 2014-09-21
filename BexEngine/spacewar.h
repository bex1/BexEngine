#pragma once
#define WIN32_LEAN_AND_MEAN

#include "game.h"

//=============================================================================
// Create game class
//=============================================================================
class Spacewar : public Game
{
private:
	// variables

public:
	// Constructor
	Spacewar();

	// Destructor
	virtual ~Spacewar();

	// Initialize the game
	void initialize(HWND hwnd);
	void update();      // must override pure virtual from Game
	void ai();          // "
	void collisions();  // "
	void render();      // "
	void releaseAll();
	void resetAll();
};