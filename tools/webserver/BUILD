load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "webserver",
    srcs = ["main.cc"],
    deps = [
        "@uwebsockets//:uwebsockets_lib",
        "@bazel_tools//tools/cpp/runfiles",
    ],
    data = glob(["static/**/*"]) + [
        "//tests/hello-web:hello_web_wasm",
    ],
)