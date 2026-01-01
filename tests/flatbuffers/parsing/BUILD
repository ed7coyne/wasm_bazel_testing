load("@flatbuffers//:build_defs.bzl", "flatbuffer_cc_library")
load("@wasm_toolchain//bazel:transitions.bzl", "wasm_binary")

flatbuffer_cc_library(
    name = "message_fbs",
    srcs = ["message.fbs"],
)

cc_binary(
    name = "parser_bin",
    srcs = ["parser.cc"],
    deps = [":message_fbs"],
)

wasm_binary(
    name = "parser_wasm",
    binary = ":parser_bin",
    visibility = ["//visibility:public"],
)

cc_test(
    name = "parser_test",
    srcs = ["parser_test.cc"],
    data = [":parser_wasm"],
    deps = [
        "//utils:wasmtime_runner",
        ":message_fbs",
        "@bazel_tools//tools/cpp/runfiles",
        "@com_google_googletest//:gtest_main",
    ],
)
