#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "tools/cpp/runfiles/runfiles.h"
#include "utils/wasmtime_runner.h"

using bazel::tools::cpp::runfiles::Runfiles;

TEST(HelloTest, ReturnsHelloWorld) {
    std::string error;
    std::unique_ptr<Runfiles> runfiles(Runfiles::CreateForTest(&error));
    ASSERT_NE(runfiles, nullptr) << "Failed to initialize runfiles: " << error;

    std::string hello_path = runfiles->Rlocation("wasm-bazel/tests/hello/hello_bin");
    ASSERT_FALSE(hello_path.empty()) << "Could not find hello_bin";

    auto runner_or_error = utils::WasmRunner::Create();
    ASSERT_TRUE(runner_or_error.has_value()) << "Failed to create runner: " << runner_or_error.error();
    
    auto& runner = runner_or_error.value();
    auto result_or_error = runner.Run(hello_path, {"hello_bin"});
    
    ASSERT_TRUE(result_or_error.has_value()) << "Execution failed: " << result_or_error.error();
    EXPECT_EQ(result_or_error.value().stdout_output, "Hello World!\n");
}

TEST(HelloTest, ReturnsHelloName) {
    std::string error;
    std::unique_ptr<Runfiles> runfiles(Runfiles::CreateForTest(&error));
    ASSERT_NE(runfiles, nullptr) << "Failed to initialize runfiles: " << error;

    std::string hello_path = runfiles->Rlocation("wasm-bazel/tests/hello/hello_bin");
    ASSERT_FALSE(hello_path.empty()) << "Could not find hello_bin";

    auto runner_or_error = utils::WasmRunner::Create();
    ASSERT_TRUE(runner_or_error.has_value()) << "Failed to create runner: " << runner_or_error.error();
    
    auto& runner = runner_or_error.value();
    auto result_or_error = runner.Run(hello_path, {"hello_bin", "Test"});
    
    ASSERT_TRUE(result_or_error.has_value()) << "Execution failed: " << result_or_error.error();
    EXPECT_EQ(result_or_error.value().stdout_output, "Hello Test!\n");
}
