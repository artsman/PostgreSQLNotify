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


#include "Queue.h"
#include "Logging.he"

#include <boost/format.hpp>

using boost::format;
using boost::shared_ptr;

// Description of object used for logging
std::string Queue::desc() {
    if (!queueName.empty()) {
        return str(format("Queue (%s)") % queueName);
    } else {
        return "Queue";
    }  
}

void Queue::add(boost::shared_ptr<Worker> w) { 
    _queue.push_back(w); 
    
    LOG_INFO << "Adding " << w->desc() << " to " << desc();
    
    // Init object prior to performing threading (for objects that need allocation on main thread)
    w->init();
    
    checkQueue(); 
}

// Check queue for new items
void Queue::checkQueue() {
    
    // The queue will be checked frequently, so if we can't get the lock, we will try again later
    if( _queueMutex.try_lock() == false ) {
        return;
    }
    
    int runSize = 0;
    WorkQueueIterator it = _queue.begin();
    shared_ptr<Worker> worker;
    while( it != _queue.end() && runSize < _size ) {
        
        worker = (*it);
        
        if( worker->complete() == true) {
            // Item is complete, pop it off
            _queue.erase(it);
        } else {
            // Start Workers until the queue size is hit
            if( worker->running() == false) {
                LOG_INFO << "Starting " << worker->desc() << " in " << desc();
                worker->start(shared_from_this());
            }
            
            ++runSize;
        }
        ++it;
    }
}

// Mark worker as completed and move it into completed vector
void Queue::workerDone(boost::weak_ptr<Worker> worker) {
    
    {
        boost::mutex::scoped_lock lock(_finishedMutex);
        
        // Convert weak ptr back to shared_ptr
        boost::shared_ptr<Worker> completedWorker;
        try {
            completedWorker = worker.lock();
        } catch (boost::bad_weak_ptr bwp) {
            LOG_ERROR << "Unable to get reference to completed worker in " << desc();
        }
        
        if(completedWorker != boost::shared_ptr<Worker>()) {
            LOG_INFO << completedWorker->desc() << " marked as completed in " << desc();
            _completed.push_back(completedWorker); 
        }
    }
    
    // When worker completes check the queue
    checkQueue();
}