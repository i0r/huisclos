#include "Shared.h"
#include "Timer.h"

void Timer_Start( timer_t* timer )
{
	Timer_Reset( timer );
}

double Timer_GetDelta( timer_t* timer )
{
	std::chrono::high_resolution_clock::time_point currTime = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double, std::milli> delta = currTime - timer->prevTime;
	timer->deltaTime = delta.count();
	timer->prevTime = currTime;

	return timer->deltaTime;
}

uint64_t Timer_GetElapsed( timer_t* timer )
{
	std::chrono::duration<double> addTime = std::chrono::high_resolution_clock::now() - timer->startTime;
	return static_cast<uint64_t>( timer->totalTime + addTime.count() );
}

void Timer_Reset( timer_t* timer )
{
	timer->startTime = std::chrono::high_resolution_clock::now();
	timer->prevTime = timer->startTime;
	timer->deltaTime = 0.0;
	timer->totalTime = 0;
}
