load("@aspect_rules_js//js:defs.bzl", "js_binary")

# Export the JS files for direct use
exports_files([
    "firefox_downloader.js",
    "firefox_executable_path.js",
], visibility = ["//tests/hello-web:__pkg__"])

js_binary(
    name = "firefox_downloader",
    entry_point = "firefox_downloader.js",
    data = [
        "firefox_executable_path.js",
    ],
    visibility = ["//tests/hello-web:__pkg__"],
)