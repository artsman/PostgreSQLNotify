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


#ifndef WORKER_H_
#define WORKER_H_

#include "Tools.h"
#include "Logging.he"

#include <string>
#include <queue>
#include <extcomp.he>

#include <boost/any.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/enable_shared_from_this.hpp>

class Queue;  // Forward declare to hold a pointer to the queue the object is in
class WorkerDelegate;

class Worker : public boost::enable_shared_from_this<Worker> {
public:    
    typedef std::map<std::string, boost::any, Tools::CaseInsensitiveLess> ParamMap;
    typedef std::map<std::string, boost::any, Tools::CaseInsensitiveLess>::iterator ParamMapIterator;
    
    Worker();
    Worker(const ParamMap& p, boost::shared_ptr<WorkerDelegate> d);
    Worker(const Worker& w);
    virtual ~Worker();
    
    // Starting worker
    void run();
    void start();
    void start(boost::shared_ptr<Queue>);
    
    bool running();
    void setRunning(bool r);
    
    bool complete();
    void setComplete(bool c);
    
    void cancel();
    bool cancelled();
    
    ParamMap result();
    void setResult(const ParamMap& pm);
    
    virtual void init(); // Code to be run prior to entering the thread
    virtual std::string desc();
    virtual void reset();
    
    void setWorkerName(std::string wn) { _workerName = wn; }
    
    boost::shared_ptr<WorkerDelegate> delegate();
    //void setDelegate(const boost::shared_ptr<WorkerDelegate>& d) { _delegate = d; }
    
protected:
    std::string _workerName;
    
    ParamMap _params;
    ParamMap _result;
private:
    
    class WorkerThread {
    public: 
        WorkerThread() {}
        WorkerThread(const boost::weak_ptr<Worker>& w, const boost::shared_ptr<WorkerDelegate>& wd) : _worker(w), _delegate(wd) {}
        
        void operator()();
    private:
        boost::weak_ptr<Worker> _worker;
        boost::shared_ptr<WorkerDelegate> _delegate;
    };
    
    bool _running;
    bool _complete;
    bool _cancelled;
    
    boost::shared_mutex _runMutex;
    boost::shared_mutex _completeMutex;
    boost::shared_mutex _cancelMutex;
    boost::shared_mutex _resultMutex;
    boost::mutex _workMutex;
    boost::thread _thread;
    
    boost::shared_ptr<Queue> _queue;
    boost::shared_ptr<WorkerDelegate> _delegate;
};

// Worker Delegate
class WorkerDelegate : public boost::enable_shared_from_this<WorkerDelegate> {
public:
    virtual void init(Worker::ParamMap&) = 0;
    virtual Worker::ParamMap run(Worker::ParamMap&) = 0;
    virtual void cancel() = 0;
};

// PostgreSQL Worker Delegate
class PostgreSQLDelegate : public WorkerDelegate {
public:
    virtual void init(Worker::ParamMap&);
    virtual Worker::ParamMap run(Worker::ParamMap&);
    virtual void cancel();
private:
    bool cancelled;
    boost::shared_ptr<EXTqlist> _listResult;
};

// PostgreSQL Listen/Notify Delegate
class PostgreSQLNotifyDelegate : public WorkerDelegate {
public:
    virtual void init(Worker::ParamMap&);
    virtual Worker::ParamMap run(Worker::ParamMap&);
    virtual void cancel();
    
    virtual void listen(std::string);
    virtual void listenFunction(std::string, std::string);
    virtual void unlisten(std::string);
    virtual void unlistenFunction(std::string, std::string);
    virtual void notify(std::string, std::string);
    
    virtual std::vector<std::pair<std::string, std::string> > popListens();
    virtual std::vector<std::pair<std::string, std::string> > popUnlistens();
    virtual std::vector<std::pair<std::string, std::string> > popNotifications();
    
    virtual void receive(Worker::ParamMap);
    virtual EXTqlist* popReceived();
    virtual bool hasReceived();
    
private:
    bool cancelled;
    
    boost::mutex _receivedMutex;
    std::vector<Worker::ParamMap> _received;
    
    boost::mutex _listenMutex;
    std::vector<std::pair<std::string, std::string> > _listens;
    boost::mutex _unlistenMutex;
    std::vector<std::pair<std::string, std::string> > _unlistens;
    boost::mutex _notificationMutex;
    std::vector<std::pair<std::string, std::string> > _notifications;
};

#endif // WORKER_H_



