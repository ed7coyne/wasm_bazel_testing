"""
Transitions and rules for building and running WebAssembly (WASM) targets.

This module defines:
1. `wasm_platform_transition`: A transition that forces the build configuration
   to use the `//:wasm32_wasi` platform.
2. `wasm_binary`: A rule that applies the transition to build a binary for WASM.
3. `wasm_run`: A rule that builds a WASM binary (transitioned) and wraps it
   in a script to execute it with a specified runner (e.g., wasmer).
"""

def _wasm_platform_transition_impl(settings, attr):
    return {
        "//command_line_option:platforms": "//:wasm32_wasi",
    }

wasm_platform_transition = transition(
    implementation = _wasm_platform_transition_impl,
    inputs = [],
    outputs = ["//command_line_option:platforms"],
)

def _wasm_binary_impl(ctx):
    # We expect the transitioned binary to produce a single executable or artifact
    # Since we are grabbing the files from the dependency, we just forward them.
    # We collect all files from the 'binary' attribute.
    
    files = []
    for target in ctx.attr.binary:
        files.extend(target.files.to_list())
        
    return [
        DefaultInfo(
            files = depset(files),
            runfiles = ctx.runfiles(files = files),
        )
    ]

wasm_binary = rule(
    implementation = _wasm_binary_impl,
    attrs = {
        "binary": attr.label(cfg = wasm_platform_transition, allow_files = True),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    },
)

def _wasm_run_impl(ctx):
    # Get the wasm file (transitioned)
    wasm_target = ctx.attr.binary[0]
    wasm_file = wasm_target.files.to_list()[0]
    
    # Get the runner file
    runner_file = ctx.attr.runner.files.to_list()[0]
    
    # Create wrapper script
    output_script = ctx.actions.declare_file(ctx.label.name)
    
    script_content = """#!/bin/bash
# Find runfiles root
if [ -z "$RUNFILES_DIR" ]; then
  if [ -d "$0.runfiles" ]; then
    RUNFILES_DIR="$0.runfiles"
  else
    echo "Error: Could not find runfiles."
    exit 1
  fi
fi

# Paths in runfiles
# workspace_name is usually the directory name in runfiles for the main repo
# external repos are under their own name.

# We need to correctly locate the files.
# For the main repo file (wasm_file), it is usually under ctx.workspace_name
# But wait, ctx.workspace_name might be 'main' or the actual name?
# Let's try to find it dynamically or use the short_path hint.

# runner_file.short_path usually looks like "../wasmer/bin/wasmer" or "external/wasmer/bin/wasmer"
# wasm_file.short_path usually looks like "hello/hello" (if in same repo)

RUNNER="$RUNFILES_DIR/{runner_path}"
WASM="$RUNFILES_DIR/{wasm_path}"

# Normalize paths (remove ../ if present at start, handled by runfiles structure)
# Actually, runfiles structure flattens external repos.
# external/wasmer/bin/wasmer -> wasmer/bin/wasmer
# hello/hello -> wasm-bazel/hello/hello (if wasm-bazel is workspace name)

# Let's verify short_path values.
# If short_path starts with ../, it means it is external.
# e.g. ../wasmer/bin/wasmer -> runfiles/wasmer/bin/wasmer

exec "$RUNNER" "$WASM" "$@"
"""
    
    # Logic to convert short_path to runfiles path
    def to_runfiles_path(f):
        path = f.short_path
        if path.startswith("../"):
            return path[3:]
        else:
            return ctx.workspace_name + "/" + path

    runner_path = to_runfiles_path(runner_file)
    wasm_path = to_runfiles_path(wasm_file)

    ctx.actions.write(
        output = output_script,
        content = script_content.format(
            runner_path = runner_path,
            wasm_path = wasm_path
        ),
        is_executable = True,
    )
    
    runfiles = ctx.runfiles(files = [wasm_file, runner_file])
    
    return [
        DefaultInfo(
            executable = output_script,
            runfiles = runfiles,
        )
    ]

wasm_run = rule(
    implementation = _wasm_run_impl,
    executable = True,
    attrs = {
        "binary": attr.label(cfg = wasm_platform_transition, allow_files = True),
        "runner": attr.label(allow_files = True),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    },
)
