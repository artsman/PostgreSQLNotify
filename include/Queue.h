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


#ifndef QUEUE_H_
#define QUEUE_H_

#include "Worker.h"
#include "DBPool.h"

#include <string>
#include <vector>
#include <map>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

class Queue : public boost::enable_shared_from_this<Queue> {
public:
    typedef std::vector<boost::shared_ptr<Worker> > WorkQueue;
    typedef std::vector<boost::shared_ptr<Worker> >::iterator WorkQueueIterator;
    
    Queue() {}
    ~Queue() {}
       
    void add(boost::shared_ptr<Worker>);
    
    void setSize(int size) { _size = size; }
    int size() { return _size; }
    
    void workerDone(boost::weak_ptr<Worker>);
    
    WorkQueue& completed() { return _completed; }
    void clearCompleted() { _completed.clear(); }
    
    void setQueueName(std::string qn) { queueName = qn; }
    virtual std::string desc();
    
private:
    std::map<std::string, boost::shared_ptr<DBPool> > pools;
public:
    // Create a pool for the given pool type T.  T must be inherited from DBPool
    template<class T>
    boost::shared_ptr<T> addPool(std::string name, const ConnInfoMap& ci) {
        pools[name] = boost::make_shared<T>(ci, shared_from_this());
        return pools[name];
    }
protected:
    std::string queueName;
private:
    void checkQueue();
    
    int _size;
    WorkQueue _queue;
    WorkQueue _completed;
           boost::mutex _queueMutex;
    boost::mutex _finishedMutex;
};


#endif // QUEUE_H_