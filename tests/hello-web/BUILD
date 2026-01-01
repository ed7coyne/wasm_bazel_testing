load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@wasm_toolchain//bazel:transitions.bzl", "wasm_binary", "wasm_run")

cc_binary(
    name = "hello_web_bin",
    srcs = ["hello_web.cc"],
    target_compatible_with = [
        "@platforms//os:wasi",
        "@platforms//cpu:wasm32",
    ],
    deps = [
        "@fmt",
    ],
    # Add optimization flags for better performance
    copts = [
        "-O3",                # Highest optimization level
    ],
    # Add linker flags for optimization
    linkopts = [
        "-O3",                # Highest optimization level
        "-flto",              # Link-time optimization
        "-z stack-size=8192", # Smaller stack size (8KB)
    ],
)

wasm_binary(
    name = "hello_web_wasm",
    binary = ":hello_web_bin",
    visibility = ["//visibility:public"],  # Make it visible to the webserver
)

wasm_run(
    name = "hello_web",
    binary = ":hello_web_bin",
    runner = "@wasmer//:wasmer", 
)