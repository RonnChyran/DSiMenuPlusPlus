/*
    timer.cpp
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <nds.h>
#include "time/timer.h"

vu64 Timer::_overFlow = 0;
Timer::Timer()
{
    //initTimer();
}

void Timer::timerInterruptHandler()
{
    _overFlow += 65536;
}

void Timer::initTimer()
{
    _lastTime = 0;
    _currentTime = 0;
    _overFlow = 0;
    _fps = 0.f;
    _fpsCounter = 0;
    irqEnable( IRQ_TIMER2 );
    irqSet( IRQ_TIMER2, Timer::timerInterruptHandler );
    TIMER2_DATA = 0; // set reload value
    TIMER2_CR = TIMER_ENABLE | TIMER_DIV_1 | TIMER_IRQ_REQ;
}

double Timer::updateTimer()
{
    _currentTime = (_overFlow + TIMER2_DATA) * _factor;
    return _currentTime;
}

double Timer::updateFps()
{
    if( _fpsCounter++ > 60 )
    {
        double elapsedTime = _currentTime - _lastTime;
        _fps = _fpsCounter / elapsedTime;
        _fpsCounter = 0;
        _lastTime = _currentTime;
    }
    return _fps;
}

double Timer::getTime()
{
    return _currentTime;
}

vu64 Timer::getTick()
{
    irqDisable( IRQ_TIMER2 );
    DC_FlushAll();
    static vu64 lastTick = 0;
    vu64 tick = _overFlow + TIMER2_DATA;
    if( tick < lastTick )
        tick += 65536;
    lastTick = tick;
    irqEnable( IRQ_TIMER2 );
    return tick;
}

double Timer::tickToUs( u64 tick )
{
    return tick * 1.f/(33.514*1000000.f) * 1000 * 1000;
}

double Timer::getFps()
{
    return _fps;
}
