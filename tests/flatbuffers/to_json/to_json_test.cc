#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

#include "tools/cpp/runfiles/runfiles.h"
#include "utils/wasmtime_runner.h"
#include "tests/flatbuffers/to_json/robot_generated.h"

using bazel::tools::cpp::runfiles::Runfiles;
using json = nlohmann::json;

TEST(ToJsonTest, ConvertsBinaryToJson) {
    std::string error;
    std::unique_ptr<Runfiles> runfiles(Runfiles::CreateForTest(&error));
    ASSERT_NE(runfiles, nullptr) << "Failed to initialize runfiles: " << error;

    std::string wasm_path = runfiles->Rlocation("wasm-bazel/tests/flatbuffers/to_json/to_json_bin");
    ASSERT_FALSE(wasm_path.empty()) << "Could not find to_json_bin";

    // 1. Create Flatbuffer message
    flatbuffers::FlatBufferBuilder builder(1024);
    auto name = builder.CreateString("Bender B. Rodriguez");
    
    tests::to_json::RobotBuilder robot_builder(builder);
    robot_builder.add_model_name(name);
    robot_builder.add_year_manufactured(2996);
    robot_builder.add_battery_voltage(12.5);
    auto robot = robot_builder.Finish();
    
    builder.Finish(robot);

    std::string input_data(
        reinterpret_cast<const char*>(builder.GetBufferPointer()),
        builder.GetSize()
    );

    // 2. Run WASM
    auto runner_or_error = utils::WasmRunner::Create();
    ASSERT_TRUE(runner_or_error.has_value()) << runner_or_error.error();
    auto& runner = runner_or_error.value();

    auto result_or = runner.Run(wasm_path, {"to_json_bin"}, input_data);
    
    ASSERT_TRUE(result_or.has_value()) << "Execution failed: " << result_or.error();
    std::string stdout_str = result_or.value().stdout_output;
    ASSERT_FALSE(stdout_str.empty()) << "Stdout is empty, stderr: " << result_or.value().stderr_output;

    // 3. Verify JSON
    try {
        auto j = json::parse(stdout_str);
        EXPECT_EQ(j["model_name"], "Bender B. Rodriguez");
        EXPECT_EQ(j["year_manufactured"], 2996);
        EXPECT_EQ(j["battery_voltage"], 12.5);
    } catch (const json::parse_error& e) {
        FAIL() << "JSON parse error: " << e.what() << "\nOutput was: " << stdout_str;
    }
}
