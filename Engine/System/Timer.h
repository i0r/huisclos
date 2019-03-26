#pragma once

#include <chrono>

struct timer_t
{
	std::chrono::high_resolution_clock::time_point	startTime;
	std::chrono::high_resolution_clock::time_point	prevTime;
	uint64_t										totalTime;
	double											deltaTime;
};

static_assert( sizeof( timer_t ) == 32, "timer_t is NOT cache friendly" );

void		Timer_Start( timer_t* timer );
double		Timer_GetDelta( timer_t* timer );
uint64_t	Timer_GetElapsed( timer_t* timer );
void		Timer_Reset( timer_t* timer );
