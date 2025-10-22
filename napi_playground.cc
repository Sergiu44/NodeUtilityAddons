#include <napi.h>
#include <argon2.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include "thread/thread_worker.h"

class HashWorker : public Napi::AsyncWorker {
public:
    HashWorker(Napi::Env env,
               const std::string& password,
               const std::string& salt,
               uint32_t hashLen,
               Napi::Promise::Deferred deferred)
        : Napi::AsyncWorker(env),
          password_(password),
          salt_(salt),
          hashLen_(hashLen),
          deferred_(deferred) {}

    void Execute() override {
        if (hashLen_ == 0 || hashLen_ > 1024U) {
            SetError("hashLen must be between 1 and 1024 bytes");
            return;
        }
        if (salt_.size() < 8) {
            SetError("salt must be at least 8 bytes");
            return;
        }

        std::vector<uint8_t> hash(hashLen_);

        const uint32_t t_cost = 2;
        const uint32_t m_cost = 1 << 16; // 64MB
        const uint32_t parallelism = 1;

        int rc = argon2id_hash_raw(
            t_cost,
            m_cost,
            parallelism,
            password_.data(), password_.size(),
            salt_.data(), salt_.size(),
            hash.data(), hash.size()
        );

        if (rc != ARGON2_OK) {
            SetError(argon2_error_message(rc));
            return;
        }

        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (uint8_t b : hash) {
            oss << std::setw(2) << static_cast<int>(b);
        }
        resultHex_ = oss.str();
    }

    void OnOK() override {
        deferred_.Resolve(Napi::String::New(Env(), resultHex_));
    }

    void OnError(const Napi::Error& e) override {
        deferred_.Reject(e.Value());
    }

private:
    std::string password_;
    std::string salt_;
    uint32_t hashLen_;
    std::string resultHex_;
    Napi::Promise::Deferred deferred_;
};

// Exposed function: hello() -> "world"
Napi::Value Hello(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::String::New(env, "world");
}

Napi::Value Hello1(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected two numbers").ThrowAsJavaScriptException();
        return env.Null();
    }
    double a = info[0].As<Napi::Number>().DoubleValue();
    double b = info[1].As<Napi::Number>().DoubleValue();
    return Napi::Number::New(env, a + b);
}

Napi::Value HashPassword(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments
    if (info.Length() < 3 || !info[0].IsString() || !info[1].IsString() || !info[2].IsNumber()) {
        Napi::TypeError::New(env, "Expected (password: string, salt: string, hashLen: number)")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    const std::string password = info[0].As<Napi::String>().Utf8Value();
    const std::string salt = info[1].As<Napi::String>().Utf8Value();
    const uint32_t hashLen = info[2].As<Napi::Number>().Uint32Value();

    // Validate hashLen
    if (hashLen == 0 || hashLen > 1024U) {
        Napi::RangeError::New(env, "hashLen must be between 1 and 1024 bytes")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // Validate salt (your salt is 14 chars, so this passes)
    if (salt.size() < 8) {
        Napi::RangeError::New(env, "salt must be at least 8 bytes")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::vector<uint8_t> hash(hashLen);

    const uint32_t t_cost = 2;
    const uint32_t m_cost = 1 << 16; // 64MB
    const uint32_t parallelism = 1;

    // Call argon2 - use salt string directly as bytes
    int rc = argon2id_hash_raw(
        t_cost,
        m_cost,
        parallelism,
        password.data(), password.size(),
        salt.data(), salt.size(),
        hash.data(), hash.size()
    );

    if (rc != ARGON2_OK) {
        Napi::Error::New(env, argon2_error_message(rc))
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // Convert hash to hex string
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t b : hash) {
        oss << std::setw(2) << static_cast<int>(b);
    }

    return Napi::String::New(env, oss.str());
}

Napi::Value HashPasswordAsync(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 3 || !info[0].IsString() || !info[1].IsString() || !info[2].IsNumber()) {
        Napi::TypeError::New(env, "Expected (password: string, salt: string, hashLen: number)")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    const std::string password = info[0].As<Napi::String>().Utf8Value();
    const std::string salt = info[1].As<Napi::String>().Utf8Value();
    const uint32_t hashLen = info[2].As<Napi::Number>().Uint32Value();

    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
    (new HashWorker(env, password, salt, hashLen, deferred))->Queue();
    return deferred.Promise();
}

Napi::Value SleepThread(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if(info.Length() < 1) {
        Napi::TypeError::New(env, "Expected at least one argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    if(!info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected a number").ThrowAsJavaScriptException();
        return env.Null();
    }

    double ms = info[0].As<Napi::Number>().DoubleValue();

    if(ms < 0) {
        Napi::TypeError::New(env, "ms must be greater than 0").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(ms)));
    return env.Undefined();
}

Napi::Value BenchmarkSync(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if(info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Expected a function").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto start = std::chrono::high_resolution_clock::now();
    
    info[0].As<Napi::Function>().Call(env.Global(), {});

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    return Napi::Number::New(env, duration.count());
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("hello", Napi::Function::New(env, Hello));
    exports.Set("hello1", Napi::Function::New(env, Hello1));
    exports.Set("hashPassword", Napi::Function::New(env, HashPassword));
    exports.Set("hashPasswordAsync", Napi::Function::New(env, HashPasswordAsync));
    exports.Set("sleepThread", Napi::Function::New(env, SleepThread));
    exports.Set("benchmarkSync", Napi::Function::New(env, BenchmarkSync));
    
    // Thread worker functions
    exports.Set("startWorker", Napi::Function::New(env, StartWorker));
    exports.Set("stopWorker", Napi::Function::New(env, StopWorker));
    exports.Set("isWorkerRunning", Napi::Function::New(env, IsWorkerRunning));
    
    return exports;
}


NODE_API_MODULE(addon, Init)


