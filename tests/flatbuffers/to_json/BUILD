load("@flatbuffers//:build_defs.bzl", "flatbuffer_cc_library")
load("//bazel:transitions.bzl", "wasm_binary")
load("//bazel:embed_data.bzl", "cc_embed_data")

flatbuffer_cc_library(
    name = "robot_fbs",
    srcs = ["robot.fbs"],
)

cc_embed_data(
    name = "robot_fbs_h",
    src = "robot.fbs",
    variable_name = "kRobotFbs",
)

cc_binary(
    name = "to_json_bin",
    srcs = [
        "to_json.cc",
        ":robot_fbs_h",
    ],
    deps = ["@flatbuffers//:flatbuffers"],
)

wasm_binary(
    name = "to_json_wasm",
    binary = ":to_json_bin",
)

cc_test(
    name = "to_json_test",
    srcs = ["to_json_test.cc"],
    data = [":to_json_wasm"],
    deps = [
        "//utils:wasmtime_runner",
        ":robot_fbs",
        "@nlohmann_json//:json",
        "@bazel_tools//tools/cpp/runfiles",
        "@com_google_googletest//:gtest_main",
    ],
)
