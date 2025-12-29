# bazel/wasi_sdk.BUILD
package(default_visibility = ["//visibility:public"])

filegroup(
    name = "sysroot",
    srcs = glob(
        include = [
            "share/wasi-sysroot/**/*",
            "lib/clang/**/*",
        ],
        allow_empty = True,
    ),
)


filegroup(
    name = "clang_binaries",
    srcs = [
        "bin/clang",
        "bin/clang++",
        "bin/wasm-ld",
        "bin/llvm-ar",
        "bin/llvm-nm",
        "bin/llvm-objcopy",
        "bin/llvm-objdump",
    "bin/llvm-strip",
])

filegroup(
    name = "everything",
    srcs = [
        ":clang_binaries",
        ":sysroot",
    ],
)


# Export individual files for the toolchain config rule
exports_files([
    "bin/clang",
    "bin/clang++",
    "bin/wasm-ld",
    "bin/llvm-ar",
    "bin/llvm-nm",
    "bin/llvm-objcopy",
    "bin/llvm-objdump",
    "bin/llvm-strip",
    "share/wasi-sysroot/include/wasm32-wasi/stdint.h",
])