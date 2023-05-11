#ifndef NET_BENCH_LIB_HPP
#define NET_BENCH_LIB_HPP

#include <mpc3/net_bench_lib_export.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <optional>
#include <thread>
#include <vector>

#include <emp-tool/emp-tool.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <transputation/transputation.h>


[[nodiscard]] constexpr int factorial_constexpr(int input) noexcept
{
  if (input == 0) { return 1; }

  return input * factorial_constexpr(input - 1);
}

namespace mpc3 {

[[nodiscard]] NET_BENCH_LIB_EXPORT int factorial(int) noexcept;

constexpr const int _kStartPort = 8181;
constexpr const size_t _kBDP = 125000;

NET_BENCH_LIB_EXPORT
template<typename T>
long connectionThread(T *arr, size_t num_elements, const bool isSender, std::string const &ipAddr, int port)
{
  time_point<high_resolution_clock> tic;
  if (isSender) {
    auto io = std::make_unique<emp::NetIO>(nullptr, port);
    tic = high_resolution_clock::now();
    io->send_data(arr, num_elements * sizeof(T));
    io->flush();
  } else {
    auto io = std::make_unique<emp::NetIO>(ipAddr.c_str(), port);
    tic = high_resolution_clock::now();
    io->recv_data(arr, num_elements * sizeof(T));
  }

  auto toc = high_resolution_clock::now();
  long us = 0;
  if (tic > toc) {
    spdlog::error("emp timer overflowed in {}", __func__);
  } else {
    us = std::chrono::duration_cast<std::chrono::microseconds>(toc - tic).count();
  }
  return us;
}

NET_BENCH_LIB_EXPORT
template<typename T>
void multiSocketTransfer(std::string const &ipAddr,
  const bool isSender,
  std::string const &funcName,
  std::vector<T> &arr)
{
  size_t material_size = arr.size() * sizeof(T);
  // Now we will form as many connections to maximize the throughput
  // ceiling of (material_size / bandwidth_delay_product)
  size_t num_connections = static_cast<size_t>(ceil(static_cast<double>(material_size) / static_cast<double>(_kBDP)));
  std::cout << "Number of Connections: " << num_connections << std::endl;

  // TODO In emp's net_io_channel (or system wide), ensure both the sender and receiver buffers are 2 *
  // bandwidth_delay_product (as linux assumes half the buffers is used for internal kernel structures)

  std::vector<std::future<long>> threads;
  size_t start_idx = 0;
  size_t offset = num_connections == 1 ? arr.size() : _kBDP / sizeof(T);

  for (size_t idx = 0; idx < num_connections; idx++) {
    int curr_port = _kStartPort + static_cast<int>(idx) + 1;
    threads.push_back(std::async(connectionThread<T>, &arr[start_idx], offset, isSender, ipAddr, curr_port));
    start_idx += offset;
    if ((start_idx + offset) > arr.size() && idx != (num_connections - 1)) {
      offset = arr.size() - start_idx;
      spdlog::error("likely off by one bug. index={}, num connec={}", idx, num_connections);
      throw std::logic_error("off by one bug");
    }
  }

  long us =
    std::accumulate(threads.begin(), threads.end(), 0, [](long sum, auto &thread) { return sum + thread.get(); });
  spdlog::info("emp multi-socket transfer took {}", us);
  spdlog::get("results")->info("emp,{},{},us,{},B,", funcName, us, arr.size() * sizeof(T));
}

NET_BENCH_LIB_EXPORT
template<typename T>
void singleSocketTransfer(std::string const &ipAddr,
  const bool isSender,
  std::string const &funcName,
  std::vector<T> &arr)
{
  auto us = connectionThread<T>(&arr[0], arr.size(), isSender, ipAddr, _kStartPort);
  spdlog::info("emp single socket transfer took {}", us);
  spdlog::get("results")->info("emp,{},{},us,{},B,", funcName, us, arr.size() * sizeof(T));
}

NET_BENCH_LIB_EXPORT
template<typename T>
void transputationTransfer(std::string const &ipAddr,
  const bool isSender,
  std::string const &transport,
  std::vector<T> &arr)
{
  using namespace transputation;

  auto material_size = static_cast<uint32_t>(arr.size() * sizeof(T));
  auto *rawp = static_cast<uint8_t *>(static_cast<void *>(arr.data()));
  auto t = std::unique_ptr<Transport>(Transport::GetTransport(transport.c_str()));

  time_point<high_resolution_clock> tic;
  if (isSender) {
    t->SetupClient(ipAddr.c_str(), _kStartPort);
    const int shitsleep = 100;// Connect calls exit(1) if server not ready :(
    std::this_thread::sleep_for(std::chrono::milliseconds(shitsleep));
    t->Connect();
    tic = high_resolution_clock::now();
    t->SendRaw(material_size, rawp);
  } else {
    t->SetupServer("0.0.0.0", _kStartPort);
    t->Accept();
    tic = high_resolution_clock::now();
    t->RecvRaw(material_size, rawp);
  }
  t->Close();

  auto toc = high_resolution_clock::now();
  long us = 0;
  if (tic > toc) {
    spdlog::error("emp timer overflowed in {}", __func__);
  } else {
    us = std::chrono::duration_cast<std::chrono::microseconds>(toc - tic).count();
  }
  spdlog::info("transputation {} transfer took {}", transport, us);
  spdlog::get("results")->info("transputation,{},{},us,{},B,", transport, us, arr.size() * sizeof(T));
}

NET_BENCH_LIB_EXPORT
template<typename T>
void testTransfer(auto testFunc,
  std::string const &ipAddr,
  bool isSender,
  std::string const &funcName,
  size_t transferSz)
{
  const T initValue = 1;
  std::vector<T> arr;
  if (isSender) {
    // Initialize the vector
    arr = std::vector<T>(transferSz, initValue);
  } else {
    // set to zero and check if recv correctly afterwords
    arr = std::vector<T>(transferSz, 0);
  }

  testFunc(ipAddr, isSender, funcName, arr);

  if (std::any_of(arr.begin(), arr.end(), [&](T x) { return x != initValue; })) {
    size_t i = 0;
    for (i = 0; i < arr.size(); ++i) {
      if (arr[i] != initValue) { break; }
    }

    spdlog::error("first index with mismatch: arr[{}] = {}", i, arr[i]);
    throw std::ios_base::failure("corrupted data after transmission");
  }
}

}// namespace mpc3

#endif
