#include "thread_worker.h"
#include <chrono>

// Global worker instance
ThreadWorker* g_worker = nullptr;

/**
 * Constructor: Initialize the thread worker
 * 
 * Why this is important:
 * 1. ThreadSafeFunction is the ONLY safe way to call JavaScript from a C++ thread
 * 2. Without it, calling JavaScript from another thread will crash Node.js
 * 3. The atomic<bool> ensures thread-safe communication between threads
 */
ThreadWorker::ThreadWorker(Napi::Env env, Napi::Function callback) 
    : should_stop(false) {
    
    // Create ThreadSafeFunction - this is the magic that makes it safe
    tsfn = Napi::ThreadSafeFunction::New(
        env,                    // Node.js environment
        callback,               // JavaScript function to call
        "ThreadWorker",         // Name for debugging
        0,                      // Max queue size (0 = unlimited)
        1                       // Initial thread count
    );
    
    // Start the worker thread
    // The lambda [this] captures 'this' pointer so we can access member variables
    worker_thread = std::thread([this]() {
        int counter = 0;
        
        // Main worker loop
        while (!should_stop.load()) {  // Atomic read operation
            // Simulate some work (sleep for 1 second)
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Capture counter value for the callback
            int current_count = counter++;
            
            // This is the crucial part: safely call JavaScript from C++ thread
            auto callback = [current_count](Napi::Env env, Napi::Function jsCallback) {
                // This runs on the JavaScript thread (main thread)
                jsCallback.Call({
                    Napi::String::New(env, "Tick"),           // First argument
                    Napi::Number::New(env, current_count)     // Second argument
                });
            };
            
            // Queue the callback to be executed on the JavaScript thread
            tsfn.BlockingCall(callback);
        }
        
        // Clean up: release the ThreadSafeFunction
        tsfn.Release();
    });
}

/**
 * Destructor: Automatic cleanup (RAII principle)
 * 
 * Why this is important:
 * 1. RAII ensures resources are always cleaned up, even if exceptions occur
 * 2. Prevents memory leaks and zombie threads
 * 3. Makes the code exception-safe
 */
ThreadWorker::~ThreadWorker() {
    stop();
}

/**
 * Stop the worker thread
 * 
 * Why this is important:
 * 1. Must properly signal the thread to stop
 * 2. Must wait for the thread to finish (join)
 * 3. Prevents crashes when Node.js shuts down
 */
void ThreadWorker::stop() {
    if (worker_thread.joinable()) {
        should_stop.store(true);        // Atomic write operation
        worker_thread.join();           // Wait for thread to finish
    }
}

/**
 * Check if worker is running
 */
bool ThreadWorker::isRunning() const {
    return worker_thread.joinable() && !should_stop.load();
}

/**
 * Node.js function: Start the background worker
 * 
 * This function is called from JavaScript like:
 * addon.startWorker((message, count) => console.log(message, count));
 */
Napi::Value StartWorker(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // Check if worker is already running
    if (g_worker != nullptr) {
        Napi::Error::New(env, "Worker already running")
            .ThrowAsJavaScriptException();
        return env.Null();
    }
    
    // Validate arguments
    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Expected a callback function")
            .ThrowAsJavaScriptException();
        return env.Null();
    }
    
    // Get the JavaScript callback function
    Napi::Function callback = info[0].As<Napi::Function>();
    
    // Create and start the worker
    g_worker = new ThreadWorker(env, callback);
    
    return env.Undefined();
}

/**
 * Node.js function: Stop the background worker
 */
Napi::Value StopWorker(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (g_worker == nullptr) {
        Napi::Error::New(env, "No worker running")
            .ThrowAsJavaScriptException();
        return env.Null();
    }
    
    // Stop and delete the worker
    g_worker->stop();
    delete g_worker;
    g_worker = nullptr;
    
    return env.Undefined();
}

/**
 * Node.js function: Check if worker is running
 */
Napi::Value IsWorkerRunning(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    bool running = (g_worker != nullptr) && g_worker->isRunning();
    return Napi::Boolean::New(env, running);
}

// Note: Module initialization is handled by napi_playground.cc
// This file only provides the ThreadWorker class and functions
