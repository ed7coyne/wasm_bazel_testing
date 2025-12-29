#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <optional>

#include "tools/cpp/runfiles/runfiles.h"
#include "utils/wasmtime_runner.h"
#include "tests/flatbuffers/parsing/message_generated.h"

using bazel::tools::cpp::runfiles::Runfiles;

TEST(FlatbuffersTest, ParsesMessage) {
    std::string error;
    std::unique_ptr<Runfiles> runfiles(Runfiles::CreateForTest(&error));
    ASSERT_NE(runfiles, nullptr) << "Failed to initialize runfiles: " << error;

    std::string parser_path = runfiles->Rlocation("wasm-bazel/tests/flatbuffers/parsing/parser_bin");
    ASSERT_FALSE(parser_path.empty()) << "Could not find parser_bin";

    // 1. Create Flatbuffer message
    flatbuffers::FlatBufferBuilder builder(1024);
    auto payload = builder.CreateString("Hello WASM");
    auto message = tests::parsing::CreateMessage(builder, payload);
    builder.Finish(message);

    std::string input_data(
        reinterpret_cast<const char*>(builder.GetBufferPointer()),
        builder.GetSize()
    );

    // 2. Run WASM
    auto runner_or_error = utils::WasmRunner::Create();
    ASSERT_TRUE(runner_or_error.has_value()) << runner_or_error.error();
    auto& runner = runner_or_error.value();

    auto result_or = runner.Run(parser_path, {"parser_bin"}, input_data);
    
    ASSERT_TRUE(result_or.has_value()) << "Execution failed: " << result_or.error();
    EXPECT_EQ(result_or.value().stdout_output, "Hello WASM\n");
}
