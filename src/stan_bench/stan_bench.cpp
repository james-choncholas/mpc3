#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <optional>
#include <thread>
#include <vector>

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include <emp-tool/emp-tool.h>
#include <internal_use_only/config.hpp>
#include <transputation/transputation.h>


namespace mpc3 {

constexpr const char *_kAddr = "127.0.0.1";
constexpr const int _kStartPort = 8181;
constexpr const size_t _kBDP = 125000;

template<typename T>
void connectionThread(T *arr, size_t num_elements, const bool is_sender, std::string const &ip_addr, int port)
{
  if (is_sender) {
    auto io = std::make_unique<emp::NetIO>(nullptr, port);
    io->send_data(arr, num_elements * sizeof(T));
    io->flush();
  } else {
    auto io = std::make_unique<emp::NetIO>(ip_addr.c_str(), port);
    io->recv_data(arr, num_elements * sizeof(T));
  }
}

size_t computeNumberOfThreads(size_t material_size, size_t bandwidth_delay_product)
{
  return static_cast<size_t>(ceil(static_cast<double>(material_size) / static_cast<double>(bandwidth_delay_product)));
}

template<typename T> void multiSocketTransfer(std::vector<T> &arr, const bool is_sender, std::string const &ip_addr)
{
  size_t material_size = arr.size() * sizeof(T);
  // Now we will form as many connections to maximize the throughput
  // ceiling of (material_size / bandwidth_delay_product)
  size_t num_connections = computeNumberOfThreads(material_size, _kBDP);
  std::cout << "Number of Connections: " << num_connections << std::endl;

  // TODO In emp's net_io_channel (or system wide), ensure both the sender and receiver buffers are 2 *
  // bandwidth_delay_product (as linux assumes half the buffers is used for internal kernel structures)

  std::vector<std::thread> threads;
  size_t start_idx = 0;
  size_t offset = num_connections == 1 ? arr.size() : _kBDP / sizeof(T);

  for (size_t idx = 0; idx < num_connections; idx++) {
    int curr_port = _kStartPort + static_cast<int>(idx) + 1;
    threads.push_back(std::thread(connectionThread<T>, &arr[start_idx], offset, is_sender, ip_addr, curr_port));
    start_idx += offset;
    if ((start_idx + offset) > arr.size() && idx != (num_connections - 1)) {
      offset = arr.size() - start_idx;
      spdlog::error("likely off by one bug. index={}, num connec={}", idx, num_connections);
      throw std::logic_error("off by one bug");
    }
  }

  for_each(threads.begin(), threads.end(), mem_fn(&std::thread::join));
}

template<typename T> void singleSocketTransfer(std::vector<T> &arr, const bool is_sender, std::string const &ip_addr)
{
  connectionThread<T>(&arr[0], arr.size(), is_sender, ip_addr, _kStartPort);
}

template<typename T>
void transputationTcpTransfer(std::vector<T> &arr, const bool is_sender, std::string const &ip_addr)
{
  using namespace transputation;

  auto material_size = static_cast<uint32_t>(arr.size() * sizeof(T));
  auto *rawp = static_cast<uint8_t *>(static_cast<void *>(arr.data()));
  auto t = std::unique_ptr<Transport>(Transport::GetTransport("TCP"));

  if (is_sender) {
    t->SetupClient(ip_addr.c_str(), _kStartPort);
    const int shitsleep = 100;// Connect calls exit(1) if server not ready :(
    std::this_thread::sleep_for(std::chrono::milliseconds(shitsleep));
    t->Connect();
    t->SendRaw(material_size, rawp);
    t->Close();
  } else {
    t->SetupServer("0.0.0.0", _kStartPort);
    t->Accept();
    t->RecvRaw(material_size, rawp);
  }
  t->Close();
}

template<typename T>
void transputationUdtTransfer(std::vector<T> &arr, const bool is_sender, std::string const &ip_addr)
{
  using namespace transputation;

  auto material_size = static_cast<uint32_t>(arr.size() * sizeof(T));
  auto *rawp = static_cast<uint8_t *>(static_cast<void *>(arr.data()));
  auto t = std::unique_ptr<Transport>(Transport::GetTransport("UDT"));

  if (is_sender) {
    t->SetupClient(ip_addr.c_str(), _kStartPort);
    const int shitsleep = 100;// Connect calls exit(1) if server not ready :(
    std::this_thread::sleep_for(std::chrono::milliseconds(shitsleep));
    t->Connect();
    t->SendRaw(material_size, rawp);
    t->Close();
  } else {
    t->SetupServer("0.0.0.0", _kStartPort);
    t->Accept();
    t->RecvRaw(material_size, rawp);
  }
  t->Close();
}

template<typename T> void testTransfer(bool is_sender, std::string const &ip_addr, auto testFunc)
{
  const size_t length = 1000000;
  const T initValue = 1;
  std::vector<T> arr;
  if (is_sender) {
    // Initialize the vector
    arr = std::vector<T>(length, initValue);
  } else {
    // set to zero and check if recv correctly afterwords
    arr = std::vector<T>(length, 0);
  }

  spdlog::info("test started");
  testFunc(arr, is_sender, ip_addr);
  spdlog::info("test finished");

  if (std::any_of(arr.begin(), arr.end(), [&](T x) { return x != initValue; })) {
    spdlog::error("arr[0] = {}", arr[0]);
    throw std::ios_base::failure("corrupted data after transmission");
  }
}

}// namespace mpc3

int main(int argc, char **argv)
{
  try {
    CLI::App app{ fmt::format("{} version {}", mpc3::cmake::project_name, mpc3::cmake::project_version) };

    bool show_version = false;
    app.add_flag("--version", show_version, "Show version information");

    bool is_sender = false;
    auto *sender_flag = app.add_flag("-s,--sender", is_sender);

    std::string ip_addr(mpc3::_kAddr);
    auto *addr_option = app.add_option("-a,--address", ip_addr, "Address to send to");
    addr_option->expected(1);
    addr_option->default_str(mpc3::_kAddr);

    bool is_receiver = false;
    auto *receiver_flag = app.add_flag("-r,--receiver", is_receiver);

    sender_flag->excludes(receiver_flag);
    receiver_flag->excludes(sender_flag);

    CLI11_PARSE(app, argc, argv);

    if (show_version) {
      fmt::print("{}\n", mpc3::cmake::project_version);
      return EXIT_SUCCESS;
    }

    if (is_sender == is_receiver) {
      spdlog::error("must set either --sender or --receiver");
      return EXIT_FAILURE;
    }

    if (is_sender) { spdlog::info("sending to {}", ip_addr); }

    mpc3::testTransfer<double>(is_sender, ip_addr, mpc3::multiSocketTransfer<double>);
    mpc3::testTransfer<double>(is_sender, ip_addr, mpc3::singleSocketTransfer<double>);
    mpc3::testTransfer<double>(is_sender, ip_addr, mpc3::transputationTcpTransfer<double>);
    mpc3::testTransfer<double>(is_sender, ip_addr, mpc3::transputationUdtTransfer<double>);

  } catch (const std::exception &e) {
    spdlog::error("Unhandled exception in main: {}", e.what());
  }
  return EXIT_SUCCESS;
}