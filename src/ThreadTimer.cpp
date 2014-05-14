// The MIT License (MIT)

// Copyright (c) 2014 Arts Management Systems Ltd.

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#include "ThreadTimer.he"
#include "NVObjBase.he"
#include <map>
#include <boost/foreach.hpp>

#define TIMER_MS 100

// Init static storage
FARPROC ThreadTimer::_omnisTimer = NULL;
std::map<NVObjBase*,ThreadTimer::nextTimer> subscribers;
bool timerProcessing = false;

ThreadTimer::ThreadTimer() : _timerID(0) {
    _omnisTimer = WNDmakeTimerProc((WNDtimerProc)ThreadTimer::timerMsgProc, gInstLib);
}

ThreadTimer::~ThreadTimer() {
    if ( _omnisTimer )
		WNDdisposeTimerProc ( _omnisTimer );
}

ThreadTimer& ThreadTimer::instance() {
    static ThreadTimer theInst;
    
    return theInst;    
}

void OMNISWNDPROC ThreadTimer::timerMsgProc( HWND hwnd, UINT Msg, UINT idTimer, qulong time )
{
    if (timerProcessing) {
        return;
    }
    
    timerProcessing = true;

    
    NVObjBase* objPointer;
    std::map<NVObjBase*,nextTimer>::iterator it = subscribers.begin();
    while( it != subscribers.end() ) {
        if( it->second == kTimerContinue) {
            objPointer = it->first;
            it->second = (ThreadTimer::nextTimer) objPointer->notify();  // Cast int return value to enum
        }
        
        if(it->second == kTimerStop) {
            subscribers.erase(it++);  // Move past current iterator position, but remove item
        } else {
            ++it;
        }
    }   
    
    timerProcessing = false;
}

// Subscribe an object to the timer
void ThreadTimer::subscribe(const NVObjBase* obj) {
    NVObjBase* objPointer = const_cast<NVObjBase*>(obj);
    subscribers[objPointer] = kTimerContinue;
    
    if(!_timerID) {
        _timerID = WNDsetTimer(NULL, 0, TIMER_MS, _omnisTimer);
    }
}

// Unsubscribe an object from the timer
void ThreadTimer::unsubscribe(const NVObjBase* obj) {
    NVObjBase* objPointer = const_cast<NVObjBase*>(obj);
    
    std::map<NVObjBase*,nextTimer>::iterator it;
    it = subscribers.find(objPointer);
    if(it != subscribers.end()) {
        subscribers.erase(it);
    }
    
    if(subscribers.empty()) {
        WNDkillTimer( NULL, _timerID );
        _timerID = 0;
    }
}

// TODO: Make subscribe method that uses 
//   WNDsetTimer( NULL, 0, milliSeconds, gTimerProc );
//       to create a timer on a short timeline
//
// TODO: Must also create unsubscribe that stops the timer when the last 
//       item has been unsubscribed