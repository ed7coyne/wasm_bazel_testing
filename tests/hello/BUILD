load("@rules_cc//cc:defs.bzl", "cc_binary")
load("//bazel:transitions.bzl", "wasm_binary", "wasm_run")

cc_binary(
    name = "hello_bin",
    srcs = ["hello.cc"],
    target_compatible_with = [
        "@platforms//os:wasi",
        "@platforms//cpu:wasm32",
    ],
    deps = [
        "@fmt",
    ],
)

wasm_binary(
    name = "hello_wasm",
    binary = ":hello_bin",
)

wasm_run(
    name = "hello",
    binary = ":hello_bin",
    runner = "@wasmer//:wasmer",
)

cc_test(
    name = "hello_test",
    srcs = ["hello_test.cc"],
    data = [
        ":hello_wasm",
    ],
    deps = [
        "@bazel_tools//tools/cpp/runfiles",
        "@com_google_googletest//:gtest_main",
        "//utils:wasmtime_runner",
    ],
)