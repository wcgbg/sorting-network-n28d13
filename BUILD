load(
    "@hedron_compile_commands//:refresh_compile_commands.bzl",
    "refresh_compile_commands",
)
load("@protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")
load("@rules_proto//proto:defs.bzl", "proto_library")

package(default_visibility = ["//visibility:public"])

proto_library(
    name = "network_proto",
    srcs = ["network.proto"],
)

cc_proto_library(
    name = "network_cc_proto",
    deps = [":network_proto"],
)

cc_library(
    name = "comparator",
    hdrs = ["comparator.h"],
    deps = [
        "@glog",
    ],
)

cc_library(
    name = "math_utils",
    srcs = ["math_utils.cc"],
    hdrs = ["math_utils.h"],
    deps = [
        "@glog",
    ],
)

cc_test(
    name = "math_test",
    srcs = ["math_test.cc"],
    deps = [
        ":math_utils",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "output_type",
    srcs = ["output_type.cc"],
    hdrs = ["output_type.h"],
    deps = [
        "@glog",
    ],
)

cc_test(
    name = "output_type_test",
    srcs = ["output_type_test.cc"],
    deps = [
        ":output_type",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "mask_library",
    srcs = ["mask_library.cc"],
    hdrs = ["mask_library.h"],
    deps = [
        ":output_type",
        "@boost.dynamic_bitset",
        "@glog",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "mask_library_test",
    srcs = ["mask_library_test.cc"],
    deps = [
        ":mask_library",
        ":output_type",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "isomorphism_test",
    srcs = ["isomorphism_test.cc"],
    deps = [
        ":isomorphism",
        ":output_type",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "network",
    srcs = ["network.cc"],
    hdrs = ["network.h"],
    deps = [
        ":comparator",
        ":network_cc_proto",
        ":output_type",
        "@glog",
    ],
)

cc_test(
    name = "network_test",
    srcs = ["network_test.cc"],
    deps = [
        ":network",
        ":network_cc_proto",
        ":network_utils",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "network_utils_test",
    srcs = ["network_utils_test.cc"],
    deps = [
        ":network_utils",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "isomorphism",
    srcs = ["isomorphism.cc"],
    hdrs = ["isomorphism.h"],
    deps = [
        ":math_utils",
        ":output_type",
        "@glog",
    ],
)

cc_library(
    name = "output_bitset",
    srcs = ["output_bitset.cc"],
    hdrs = ["output_bitset.h"],
    deps = [
        ":mask_library",
        ":output_type",
        "@boost.dynamic_bitset",
        "@glog",
    ],
)

cc_test(
    name = "output_bitset_test",
    srcs = ["output_bitset_test.cc"],
    deps = [
        ":network_utils",
        ":output_bitset",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "network_utils",
    srcs = ["network_utils.cc"],
    hdrs = ["network_utils.h"],
    deps = [
        ":isomorphism",
        ":network",
        ":output_bitset",
        "@boost.algorithm",
        "@glog",
    ],
)

cc_binary(
    name = "permute_main",
    srcs = ["permute_main.cc"],
    deps = [
        ":network",
        ":network_utils",
        "@gflags",
        "@glog",
    ],
)

cc_library(
    name = "cnf_builder",
    srcs = ["cnf_builder.cc"],
    hdrs = ["cnf_builder.h"],
    deps = [
        "@boost.iostreams",
        "@glog",
    ],
)

cc_test(
    name = "cnf_builder_test",
    srcs = ["cnf_builder_test.cc"],
    deps = [
        ":cnf_builder",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "clean_up",
    srcs = ["clean_up.cc"],
    hdrs = ["clean_up.h"],
    deps = [
        ":network",
        ":network_utils",
        "@glog",
    ],
)

cc_library(
    name = "extend_network",
    srcs = ["extend_network.cc"],
    hdrs = ["extend_network.h"],
    deps = [
        ":clean_up",
        ":comparator",
        ":network",
        ":output_type",
        "@glog",
    ],
)

cc_binary(
    name = "add_layers_main",
    srcs = ["add_layers_main.cc"],
    deps = [
        ":extend_network",
        ":network",
        ":network_utils",
        "@boost.algorithm",
        "@gflags",
        "@glog",
    ],
)

cc_test(
    name = "extend_network_test",
    srcs = ["extend_network_test.cc"],
    deps = [
        ":extend_network",
        ":network_utils",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "optimize_window_size",
    srcs = ["optimize_window_size.cc"],
    hdrs = ["optimize_window_size.h"],
    deps = [
        ":isomorphism",
        ":math_utils",
        ":output_type",
        "@glog",
    ],
)

cc_test(
    name = "optimize_window_size_test",
    srcs = ["optimize_window_size_test.cc"],
    deps = [
        ":optimize_window_size",
        ":output_type",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "optimize_window_size_main",
    srcs = ["optimize_window_size_main.cc"],
    deps = [
        ":network",
        ":network_utils",
        ":optimize_window_size",
        ":output_type",
        "@gflags",
        "@glog",
    ],
)

cc_binary(
    name = "sat_generate_cnf_main",
    srcs = ["sat_generate_cnf_main.cc"],
    deps = [
        ":cnf_builder",
        ":network",
        ":network_utils",
        ":output_type",
        "@gflags",
        "@glog",
        "@protobuf",
    ],
)

cc_binary(
    name = "decode_solution_main",
    srcs = ["decode_solution_main.cc"],
    deps = [
        ":math_utils",
        ":network",
        ":network_utils",
        ":output_type",
        ":simplify",
        "@boost.iostreams",
        "@gflags",
        "@glog",
    ],
)

cc_binary(
    name = "add_comparators_main",
    srcs = ["add_comparators_main.cc"],
    deps = [
        ":extend_network",
        ":network",
        ":network_utils",
        "@gflags",
        "@glog",
    ],
)

cc_library(
    name = "stack",
    srcs = ["stack.cc"],
    hdrs = ["stack.h"],
    deps = [
        ":network",
        ":output_type",
        "@glog",
    ],
)

cc_test(
    name = "stack_test",
    srcs = ["stack_test.cc"],
    deps = [
        ":network",
        ":network_utils",
        ":output_type",
        ":stack",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "stack_main",
    srcs = ["stack_main.cc"],
    deps = [
        ":network",
        ":network_utils",
        ":output_type",
        ":stack",
        "@gflags",
        "@glog",
    ],
)

cc_library(
    name = "simplify",
    srcs = ["simplify.cc"],
    hdrs = ["simplify.h"],
    deps = [
        ":network",
        ":network_utils",
        "@glog",
    ],
)

cc_test(
    name = "simplify_test",
    srcs = ["simplify_test.cc"],
    deps = [
        ":simplify",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "network_info_main",
    srcs = ["network_info_main.cc"],
    deps = [
        ":network",
        ":network_utils",
        "@gflags",
        "@glog",
    ],
)

cc_binary(
    name = "convert_main",
    srcs = ["convert_main.cc"],
    deps = [
        ":network",
        ":network_utils",
        "@gflags",
        "@glog",
    ],
)

refresh_compile_commands(
    name = "refresh_compile_commands",
    exclude_external_sources = True,
    targets = [],
)
