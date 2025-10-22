#ifndef THREAD_WORKER_H
#define THREAD_WORKER_H

#include <napi.h>
#include <thread>
#include <atomic>
#include <functional>

/**
 * Simple Thread Worker for Node.js
 * 
 * This class demonstrates the core concepts of threading in Node.js addons:
 * 1. ThreadSafeFunction - allows safe communication between C++ threads and JavaScript
 * 2. Atomic variables - thread-safe boolean flags
 * 3. RAII (Resource Acquisition Is Initialization) - automatic cleanup
 */

class ThreadWorker {
private:
    std::thread worker_thread;           // The actual C++ thread
    Napi::ThreadSafeFunction tsfn;       // Safe way to call JavaScript from C++ thread
    std::atomic<bool> should_stop;      // Thread-safe flag to stop the worker
    
public:
    // Constructor: starts the worker thread
    ThreadWorker(Napi::Env env, Napi::Function callback);
    
    // Destructor: automatically stops and cleans up the thread
    ~ThreadWorker();
    
    // Public method to stop the worker
    void stop();
    
    // Check if worker is running
    bool isRunning() const;
};

// Global worker instance (simple approach for demonstration)
extern ThreadWorker* g_worker;

// Node.js function exports
Napi::Value StartWorker(const Napi::CallbackInfo& info);
Napi::Value StopWorker(const Napi::CallbackInfo& info);
Napi::Value IsWorkerRunning(const Napi::CallbackInfo& info);

#endif // THREAD_WORKER_H
