#ifndef UTILS_WASMTIME_RUNNER_H_
#define UTILS_WASMTIME_RUNNER_H_

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <memory>
#include <utility>
#include <optional>

#include "wasmtime.h"

namespace utils {

struct WasmResult {
    std::string stdout_output;
    std::string stderr_output;
    int exit_code = 0;
};

// Simple Expected shim since we can't rely on std::expected (C++23)
struct Unexpected {
    std::string error;
};

template <typename T>
class Expected {
public:
    Expected(T value) : value_(std::move(value)), has_value_(true) {}
    Expected(Unexpected u) : error_(std::move(u.error)), has_value_(false) {}
    // Allow implicit conversion from char* for convenience if strictly error? No, might confuse.
    // Keep it explicit.

    bool has_value() const { return has_value_; }
    const T& value() const { return *value_; }
    T& value() { return *value_; } // Non-const accessor
    const std::string& error() const { return error_; }

private:
    std::optional<T> value_;
    std::string error_;
    bool has_value_;
};

// Specialization or requirement: T must be default constructible for this simple shim
// if we don't use union/variant. WasmRunner is movable, pointers are easy.

class WasmRunner {
public:
    // Move-only
    WasmRunner(WasmRunner&& other) noexcept 
        : engine_(other.engine_), store_(other.store_), context_(other.context_) {
        other.engine_ = nullptr;
        other.store_ = nullptr;
        other.context_ = nullptr;
    }

    WasmRunner& operator=(WasmRunner&& other) noexcept {
        if (this != &other) {
            Cleanup();
            engine_ = other.engine_;
            store_ = other.store_;
            context_ = other.context_;
            other.engine_ = nullptr;
            other.store_ = nullptr;
            other.context_ = nullptr;
        }
        return *this;
    }

    WasmRunner(const WasmRunner&) = delete;
    WasmRunner& operator=(const WasmRunner&) = delete;

    ~WasmRunner() {
        Cleanup();
    }

    static Expected<WasmRunner> Create() {
        wasm_engine_t* engine = wasm_engine_new();
        if (!engine) return Expected<WasmRunner>(Unexpected{"Failed to create engine"});

        wasmtime_store_t* store = wasmtime_store_new(engine, nullptr, nullptr);
        if (!store) {
            wasm_engine_delete(engine);
            return Expected<WasmRunner>(Unexpected{"Failed to create store"});
        }

        return Expected<WasmRunner>(WasmRunner(engine, store));
    }

    Expected<WasmResult> Run(const std::string& wasm_path, const std::vector<std::string>& args, const std::optional<std::string>& stdin_content = std::nullopt) {
        // Reset WASI config for this run
        wasi_config_t* wasi = wasi_config_new();
        
        std::vector<const char*> argv_ptrs;
        for (const auto& arg : args) {
            argv_ptrs.push_back(arg.c_str());
        }
        wasi_config_set_argv(wasi, argv_ptrs.size(), argv_ptrs.data());
        wasi_config_inherit_env(wasi);

        if (stdin_content.has_value()) {
            wasm_byte_vec_t stdin_vec;
            wasm_byte_vec_new(&stdin_vec, stdin_content->size(), stdin_content->data());
            wasi_config_set_stdin_bytes(wasi, &stdin_vec);
        }

        auto stdout_file_or = TempUtilsFile::Create();
        if (!stdout_file_or.has_value()) return Expected<WasmResult>(Unexpected{"Stdout temp file creation failed: " + stdout_file_or.error()});
        TempUtilsFile stdout_file = std::move(stdout_file_or.value());

        auto stderr_file_or = TempUtilsFile::Create();
        if (!stderr_file_or.has_value()) return Expected<WasmResult>(Unexpected{"Stderr temp file creation failed: " + stderr_file_or.error()});
        TempUtilsFile stderr_file = std::move(stderr_file_or.value());
        
        if (!wasi_config_set_stdout_file(wasi, stdout_file.path().c_str())) {
             return Expected<WasmResult>(Unexpected{"Failed to set stdout file"});
        }
        if (!wasi_config_set_stderr_file(wasi, stderr_file.path().c_str())) {
             return Expected<WasmResult>(Unexpected{"Failed to set stderr file"});
        }

        // Set WASI - this overwrites previous config
        wasmtime_error_t* error = wasmtime_context_set_wasi(context_, wasi);
        if (error) return HandleError(error, nullptr, &stdout_file, &stderr_file);

        // Load Module
        auto wasm_data_or = ReadFile(wasm_path);
        if (!wasm_data_or.has_value()) return Expected<WasmResult>(Unexpected{wasm_data_or.error()});
        std::string wasm_data = std::move(wasm_data_or.value());

        wasmtime_module_t* module = nullptr;
        error = wasmtime_module_new(engine_, (const uint8_t*)wasm_data.data(), wasm_data.size(), &module);
        if (error) return HandleError(error, nullptr, &stdout_file, &stderr_file);

        // Linker
        wasmtime_linker_t* linker = wasmtime_linker_new(engine_);
        error = wasmtime_linker_define_wasi(linker);
        if (error) {
            wasmtime_module_delete(module);
            return HandleError(error, nullptr, &stdout_file, &stderr_file);
        }

        // Instantiate
        wasmtime_instance_t instance;
        wasm_trap_t* trap = nullptr;
        error = wasmtime_linker_instantiate(linker, context_, module, &instance, &trap);
        
        // Cleanup linker/module
        wasmtime_linker_delete(linker);
        wasmtime_module_delete(module);

        if (error || trap) return HandleError(error, trap, &stdout_file, &stderr_file);

        // Lookup _start
        wasmtime_extern_t start_func;
        bool found = wasmtime_instance_export_get(context_, &instance, "_start", 6, &start_func);
        if (!found || start_func.kind != WASMTIME_EXTERN_FUNC) {
            return Expected<WasmResult>(Unexpected{"_start function not found"});
        }

        // Call _start
        error = wasmtime_func_call(context_, &start_func.of.func, nullptr, 0, nullptr, 0, &trap);
        if (error || trap) {
             return HandleError(error, trap, &stdout_file, &stderr_file);
        }

        // Read outputs
        WasmResult result;
        auto stdout_res = stdout_file.ReadContent();
        if (!stdout_res.has_value()) return Expected<WasmResult>(Unexpected{"Failed to read stdout: " + stdout_res.error()});
        result.stdout_output = std::move(stdout_res.value());

        auto stderr_res = stderr_file.ReadContent();
        if (!stderr_res.has_value()) return Expected<WasmResult>(Unexpected{"Failed to read stderr: " + stderr_res.error()});
        result.stderr_output = std::move(stderr_res.value());

        return Expected<WasmResult>(result);
    }

private:
    WasmRunner() : engine_(nullptr), store_(nullptr), context_(nullptr) {}
    WasmRunner(wasm_engine_t* engine, wasmtime_store_t* store) 
        : engine_(engine), store_(store), context_(wasmtime_store_context(store)) {}

    void Cleanup() {
        if (store_) {
            wasmtime_store_delete(store_);
            store_ = nullptr;
        }
        if (engine_) {
            wasm_engine_delete(engine_);
            engine_ = nullptr;
        }
        context_ = nullptr;
    }

    static Expected<std::string> ReadFile(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return Expected<std::string>(Unexpected{"Open failed: " + path});
        std::stringstream buffer;
        buffer << file.rdbuf();
        return Expected<std::string>(buffer.str());
    }

    // TempFile helper internal class
    class TempUtilsFile {
    public:
        // Use static create instead of constructor to avoid exceptions
        static Expected<TempUtilsFile> Create() {
             char template_path[] = "/tmp/wasm_runner_XXXXXX";
             int fd = mkstemp(template_path);
             if (fd == -1) return Expected<TempUtilsFile>(Unexpected{"mkstemp failed"});
             close(fd);
             return Expected<TempUtilsFile>(TempUtilsFile(template_path));
        }

        TempUtilsFile(TempUtilsFile&& other) noexcept : path_(std::move(other.path_)) {
            other.path_.clear(); // Prevent weak state
        }
        
        TempUtilsFile& operator=(TempUtilsFile&& other) noexcept {
            if (this != &other) {
                if (!path_.empty()) std::remove(path_.c_str());
                path_ = std::move(other.path_);
                other.path_.clear();
            }
            return *this;
        }

        ~TempUtilsFile() { if (!path_.empty()) std::remove(path_.c_str()); }
        const std::string& path() const { return path_; }
        
        Expected<std::string> ReadContent() const { return WasmRunner::ReadFile(path_); }
    
    private:
        TempUtilsFile(std::string path) : path_(std::move(path)) {}
        std::string path_;
    };

    std::string FormatError(wasmtime_error_t* error, wasm_trap_t* trap = nullptr) {
        std::string err_msg;
        if (trap) {
             wasm_message_t message;
             wasm_trap_message(trap, &message);
             err_msg = "Trap: " + std::string(message.data, message.size);
             wasm_byte_vec_delete(&message);
             wasm_trap_delete(trap);
        }
        if (error) {
            wasm_message_t message;
            wasmtime_error_message(error, &message);
            if (!err_msg.empty()) err_msg += " | ";
            err_msg += "Error: " + std::string(message.data, message.size);
            wasm_byte_vec_delete(&message);
            wasmtime_error_delete(error);
        }
        return err_msg;
    }

    Expected<WasmResult> HandleError(wasmtime_error_t* error, wasm_trap_t* trap = nullptr, const TempUtilsFile* stdout_file = nullptr, const TempUtilsFile* stderr_file = nullptr) {
        std::string infra_error = FormatError(error, trap);
        std::string output_info;
        
        if (stdout_file) {
             auto stdout_res = stdout_file->ReadContent();
             if (stdout_res.has_value() && !stdout_res.value().empty()) {
                 output_info += "\nSTDOUT:\n" + stdout_res.value();
             }
        }
        if (stderr_file) {
             auto stderr_res = stderr_file->ReadContent();
             if (stderr_res.has_value() && !stderr_res.value().empty()) {
                 output_info += "\nSTDERR:\n" + stderr_res.value();
             }
        }
        return Expected<WasmResult>(Unexpected{infra_error + output_info});
    }

    wasm_engine_t* engine_;
    wasmtime_store_t* store_;
    wasmtime_context_t* context_;
};

} // namespace utils

#endif // UTILS_WASMTIME_RUNNER_H_
