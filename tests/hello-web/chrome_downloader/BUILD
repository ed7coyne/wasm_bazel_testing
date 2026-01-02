load("@aspect_rules_js//js:defs.bzl", "js_binary")

# Export the JS files for direct use
exports_files([
    "chrome_downloader.js",
    "chrome_executable_path.js",
], visibility = ["//tests/hello-web:__pkg__"])

js_binary(
    name = "chrome_downloader",
    entry_point = "chrome_downloader.js",
    data = [
        "chrome_executable_path.js",
    ],
    visibility = ["//tests/hello-web:__pkg__"],
)