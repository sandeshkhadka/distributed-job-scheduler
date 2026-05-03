#include "worker_client.hpp"
#include <fstream>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <thread>
#include <unistd.h>

namespace SystemInfo {
// Get total CPU cores
int get_cpu_cores() { return std::thread::hardware_concurrency(); }

// Get total RAM in Gigabytes
float get_mem_size_gb() {
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return (pages * page_size) / (1024.0 * 1024.0 * 1024.0); // byte to GB
}

// Get disk size of the root partition ("/") in Gigabytes
float get_disk_size_gb() {
    struct statvfs stat;
    if (statvfs("/", &stat) != 0)
        return 0.0;
    return (stat.f_blocks * stat.f_frsize) / (1024.0 * 1024.0 * 1024.0);
}

// Read CPU frequency from sysfs (in GHz)
float get_cpu_freq_ghz() {
    std::ifstream file("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
    long freq_khz = 0;
    if (file >> freq_khz) {
        return freq_khz / 1000000.0;
    }
    return 0.0; // Fallback if frequency scaling is not available
}

// Get the Operating System name
std::string get_os_name() {
    std::ifstream file("/etc/os-release");
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("PRETTY_NAME=") == 0) {
            std::string pretty_name = line.substr(12); // length of "PRETTY_NAME="
            if (pretty_name.size() >= 2 && pretty_name.front() == '"' &&
                pretty_name.back() == '"') {
                pretty_name = pretty_name.substr(1, pretty_name.size() - 2);
            }
            return pretty_name;
        }
    }

    // Fallback
    struct utsname buffer;
    if (uname(&buffer) == 0) {
        return std::string(buffer.sysname) + " " + buffer.release;
    }
    return "Linux";
}

// Get the kernel version
std::string get_kernel_version() {
    struct utsname buffer;
    if (uname(&buffer) == 0) {
        return buffer.release;
    }
    return "Unknown";
}

// Get the hostname of the machine
std::string get_hostname() {
    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, HOST_NAME_MAX) == 0) {
        return std::string(hostname);
    }
    return "UnknownWorker";
}
} // namespace SystemInfo

WorkerClient::WorkerClient(std::shared_ptr<grpc::Channel> channel)
    : stub_(djs::SchedulerService::NewStub(channel)) {}

int WorkerClient::Register() {
    // worker is already registered
    if (this->worker_id != -1) {
        return this->worker_id;
    }

    if (int registered_worker_id = db.get_registered_worker_id(); registered_worker_id != -1) {
        Logger::Info("Registration: Worker already registered.");
        this->worker_id = registered_worker_id;
        return registered_worker_id;
    }

    Logger::Info("Registration: Worker not registered. Registering worker...");
    djs::RegisterWorkerRequest request;

    request.set_cpu_cores(SystemInfo::get_cpu_cores());
    request.set_mem_size(SystemInfo::get_mem_size_gb());
    request.set_disk_size(SystemInfo::get_disk_size_gb());
    request.set_name(SystemInfo::get_hostname());
    request.set_cpu_freq(SystemInfo::get_cpu_freq_ghz());
    request.set_os(SystemInfo::get_os_name());
    request.set_kernel_version(SystemInfo::get_kernel_version());
    djs::RegisterWorkerReply reply;
    grpc::ClientContext context;
    // Perform the RPC call
    Logger::Info("GetJob request received from worker: " + std::to_string(worker_id));
    grpc::Status status = stub_->RegisterWorker(&context, request, &reply);
    // int worker_id{0};
    if (status.ok()) {
        if (reply.ok()) {
            this->worker_id = reply.worker_id();
            db.insert_worker(Worker{this->worker_id,
                                    SystemInfo::get_cpu_cores(),
                                    SystemInfo::get_mem_size_gb(),
                                    SystemInfo::get_disk_size_gb(),
                                    SystemInfo::get_cpu_freq_ghz(),
                                    SystemInfo::get_os_name(),
                                    SystemInfo::get_kernel_version(),
                                    SystemInfo::get_hostname(),
                                    "not running"});

            Logger::Info("Registration: successful: Id: " + std::to_string(worker_id));
        } else {
            Logger::Info("Registration: rejected: " + reply.message());
        }
    } else {
        Logger::Info("RPC failed: " + status.error_message());
    }
    return worker_id;
}

void WorkerClient::GetJob() {
    djs::GetJobRequest request;
    djs::GetJobResponse reply;
    grpc::ClientContext context;

    if (this->worker_id == -1) {
        Logger::Error("This worker is not registered. Register before running jobs!");
        exit(0);
    }

    request.set_worker_id(this->worker_id);
    // Perform the RPC call
    grpc::Status status = stub_->GetJob(&context, request, &reply);
    if (status.ok()) {
        std::cout << "Job ID: " << reply.job_id() << std::endl;
        std::cout << "Payload: " << reply.payload() << std::endl;
        std::system(reply.payload().c_str());
    } else {
        Logger::Info("RPC failed: " + status.error_message());
    }
}
