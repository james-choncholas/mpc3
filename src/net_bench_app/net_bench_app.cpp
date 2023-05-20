#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <optional>
#include <thread>
#include <vector>

#include <CLI/CLI.hpp>
#include <internal_use_only/config.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <mpc3/net_bench_lib.hpp>

int main(int argc, char **argv)
{
  try {
    CLI::App app{ fmt::format("{} version {}", mpc3::cmake::project_name, mpc3::cmake::project_version) };

    bool show_version = false;
    app.add_flag("--version", show_version, "Show version information");

    bool isSender = false;
    auto *sender_flag = app.add_flag("-s,--sender", isSender);

    constexpr const char *_kAddr = "127.0.0.1";
    std::string ipAddr(_kAddr);
    auto *addr_option = app.add_option("-a,--address", ipAddr, "Address to send to");
    addr_option->expected(1);
    addr_option->default_str(_kAddr);

    bool isReceiver = false;
    auto *receiver_flag = app.add_flag("-r,--receiver", isReceiver);

    sender_flag->excludes(receiver_flag);
    receiver_flag->excludes(sender_flag);

    CLI11_PARSE(app, argc, argv);

    if (show_version) {
      fmt::print("{}\n", mpc3::cmake::project_version);
      return EXIT_SUCCESS;
    }

    if (isSender == isReceiver) {
      spdlog::error("must set either --sender or --receiver");
      return EXIT_FAILURE;
    }

    if (isSender) { spdlog::info("sending to ip {}", ipAddr); }

    try {
      auto logfile = fmt::format("results-{}-{}.result", isSender ? "sender" : "receiver", mpc3::cmake::git_sha);
      auto logger = spdlog::basic_logger_mt("results", logfile);
      logger->set_pattern("%v");
    } catch (const spdlog::spdlog_ex &ex) {
      spdlog::error("Log init failed: {}", ex.what());
    }

    {
      using namespace mpc3;
      constexpr const size_t _kTransferMin = 1e6;
      constexpr const size_t _kTransferSteps = 4;

      for (size_t i = 1; i <= _kTransferSteps; ++i) {
        size_t const transferSz = _kTransferMin * i;
        testTransfer<double>(multiSocketTransfer<double>, ipAddr, isSender, "empms", transferSz);
        testTransfer<double>(singleSocketTransfer<double>, ipAddr, isSender, "empss", transferSz);
        testTransfer<double>(transputationTransfer<double>, ipAddr, isSender, "TCP", transferSz);
        testTransfer<double>(transputationTransfer<double>, ipAddr, isSender, "UDT", transferSz);
      }
    }

  } catch (const std::exception &e) {
    spdlog::error("Unhandled exception in main: {}", e.what());
  }
  return EXIT_SUCCESS;
}