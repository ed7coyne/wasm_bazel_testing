load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@wasm_toolchain//bazel:transitions.bzl", "wasm_binary", "wasm_run")
load("@aspect_rules_js//js:defs.bzl", "js_test")

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
)

wasm_binary(
    name = "hello_web_wasm",
    binary = ":hello_web_bin",
    visibility = ["//visibility:public"],  # Make it visible to the webserver
)

# Create a copy of the WASM binary with a predictable name
genrule(
    name = "copy_wasm_binary",
    srcs = [":hello_web_wasm"],
    outs = ["hello_web.wasm"],
    cmd = "cp $(location :hello_web_wasm) $@",
)

wasm_run(
    name = "hello_web",
    binary = ":hello_web_bin",
    runner = "@wasmer//:wasmer",
)

# Export test files
exports_files([
    "hello_web_test.html",
    "wasm_puppeteer_test.js",
])

# Create a shell script to run the Puppeteer test
genrule(
    name = "create_puppeteer_runner",
    srcs = [
        ":hello_web.wasm",
        ":hello_web_test.html",
        "wasm_puppeteer_test.js",
        "//tests/hello-web/firefox_downloader:firefox_downloader.js",
        "//tests/hello-web/firefox_downloader:firefox_executable_path.js",
    ],
    outs = ["run_puppeteer_test.sh"],
    cmd = """
cat > $@ << 'EOF'
#!/bin/bash
set -e

# In Bazel, TEST_SRCDIR contains the root of the runfiles tree
# and TEST_WORKSPACE is the name of the workspace
RUNFILES_DIR="$$TEST_SRCDIR"
WORKSPACE_NAME="$$TEST_WORKSPACE"

# Paths to test files
TEST_JS="$$RUNFILES_DIR/$$WORKSPACE_NAME/tests/hello-web/wasm_puppeteer_test.js"
TEST_HTML="$$RUNFILES_DIR/$$WORKSPACE_NAME/tests/hello-web/hello_web_test.html"
WASM_BIN="$$RUNFILES_DIR/$$WORKSPACE_NAME/tests/hello-web/hello_web.wasm"
FIREFOX_DOWNLOADER="$$RUNFILES_DIR/$$WORKSPACE_NAME/tests/hello-web/firefox_downloader/firefox_downloader.js"
FIREFOX_EXECUTABLE_PATH="$$RUNFILES_DIR/$$WORKSPACE_NAME/tests/hello-web/firefox_downloader/firefox_executable_path.js"

# Make sure the WASM binary is accessible
if [ ! -f "$$WASM_BIN" ]; then
  echo "Error: WASM binary not found at $$WASM_BIN"
  ls -la "$$RUNFILES_DIR/$$WORKSPACE_NAME/tests/hello-web/"
  exit 1
fi

# Create a temporary directory for Firefox
FIREFOX_DIR=$$(mktemp -d)
echo "Created temporary directory for Firefox: $$FIREFOX_DIR"

# Download Firefox using the firefox_downloader
echo "Downloading Firefox using firefox_downloader..."
node "$$FIREFOX_DOWNLOADER" "$$FIREFOX_DIR"
if [ $$? -ne 0 ]; then
  echo "Failed to download Firefox"
  exit 1
fi

# Get the Firefox executable path
FIREFOX_PATH="$$FIREFOX_DIR/$$(node -e "console.log(require('$$FIREFOX_EXECUTABLE_PATH').FIREFOX_EXECUTABLE_PATH)")"
echo "Firefox executable path: $$FIREFOX_PATH"

# Make sure Firefox is executable
chmod +x "$$FIREFOX_PATH"

# Run the test
echo "Running Puppeteer test with:"
echo "- Test script: $$TEST_JS"
echo "- Test HTML: $$TEST_HTML"
echo "- WASM binary: $$WASM_BIN"
echo "- Firefox path: $$FIREFOX_PATH"

# Set environment variables for the test
export TEST_SERVER_PORT="8099"
export TEST_HTML_PATH="$$TEST_HTML"
export WASM_BIN_PATH="$$WASM_BIN"
export FIREFOX_PATH="$$FIREFOX_PATH"

# Run the test
node "$$TEST_JS"
RESULT=$$?

# Clean up
echo "Cleaning up temporary directory: $$FIREFOX_DIR"
rm -rf "$$FIREFOX_DIR"

# Exit with the same code as the test
exit $$RESULT
EOF

chmod +x $@
""",
)

# Puppeteer test for hello_web
sh_test(
    name = "hello_web_puppeteer_test",
    srcs = [":create_puppeteer_runner"],
    data = [
        ":hello_web.wasm",
        ":hello_web_test.html",
        "wasm_puppeteer_test.js",
        "//tests/hello-web/firefox_downloader",
        "//tests/hello-web/firefox_downloader:firefox_downloader.js",
        "//tests/hello-web/firefox_downloader:firefox_executable_path.js",
    ],
    # Explicitly set the target platform to host to avoid wasm32/wasi constraints
    target_compatible_with = ["@platforms//os:linux"],
)