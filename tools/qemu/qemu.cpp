/**
 * @file qemu.cpp
 * @brief QEMU VM management tool for IEC61850 testing
 * @details Provides lightweight VM orchestration using initramfs-based boot
 *          for fast startup and minimal overhead. Supports server/client modes.
 */

#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace
{
    constexpr const char* QEMU_DIR = "tools/qemu";
    constexpr const char* INITRAMFS_NAME = "initramfs.cpio.gz";
    constexpr const char* KERNEL_NAME = "vmlinuz";
    constexpr int VM_MEMORY_MB = 512;

    /**
     * @brief Executes a shell command and returns the output
     * @param cmd Command to execute
     * @return Output string or empty on failure
     */
    std::string execCommand(const std::string& cmd)
    {
        std::array<char, 256> buffer;
        std::string result;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe)
        {
            return "";
        }
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
        {
            result += buffer.data();
        }
        pclose(pipe);
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
        {
            result.pop_back();
        }
        return result;
    }

    /**
     * @brief Checks if running as root
     * @return True if root, false otherwise
     */
    bool isRoot()
    {
        return geteuid() == 0;
    }

    /**
     * @brief Gets the project root directory
     * @return Path to project root
     */
    fs::path getProjectRoot()
    {
        fs::path current = fs::current_path();
        while (!current.empty())
        {
            if (fs::exists(current / "CMakeLists.txt") && fs::exists(current / "src"))
            {
                return current;
            }
            if (current.parent_path() == current)
            {
                break;
            }
            current = current.parent_path();
        }
        return fs::current_path();
    }

    /**
     * @brief Prints usage information
     * @param prog Program name
     */
    void printUsage(const char* prog)
    {
        std::cout << "Usage: " << prog << " <command> [options]\n"
                  << "\nCommands:\n"
                  << "  setup       Create initramfs and prepare QEMU environment\n"
                  << "  server      Start server VM\n"
                  << "  client      Start client VM\n"
                  << "  network     Setup TAP network interfaces\n"
                  << "  cleanup     Remove TAP interfaces and cleanup\n"
                  << "  status      Show VM and network status\n"
                  << "\nOptions:\n"
                  << "  --interface <name>  Network interface (default: tap0/tap1)\n"
                  << "  --memory <MB>       VM memory in MB (default: " << VM_MEMORY_MB << ")\n"
                  << "  --kernel <path>     Custom kernel path\n"
                  << std::endl;
    }

    /**
     * @brief Creates the init script for initramfs
     * @param buildPath Path to built binaries
     * @return Init script content
     */
    std::string createInitScript(const fs::path& buildPath)
    {
        std::ostringstream ss;
        ss << "#!/bin/sh\n"
           << "mount -t proc none /proc\n"
           << "mount -t sysfs none /sys\n"
           << "mount -t devtmpfs none /dev\n"
           << "mkdir -p /dev/pts\n"
           << "mount -t devpts none /dev/pts\n"
           << "mkdir -p /run\n"
           << "mkdir -p /mnt/host\n"
           << "ip link set lo up\n"
           << "ip link set eth0 up 2>/dev/null\n"
           << "ip addr add 192.168.100.2/24 dev eth0 2>/dev/null\n"
           << "echo 'Mounting host filesystem...'\n"
           << "mount -t 9p -o trans=virtio,version=9p2000.L,msize=104857600 host0 /mnt/host\n"
           << "if [ $? -eq 0 ]; then\n"
           << "  echo 'Host mounted at /mnt/host'\n"
           << "  export PATH=/mnt/host/build:$PATH\n"
           << "else\n"
           << "  echo 'WARNING: 9p mount failed. Host filesystem not available.'\n"
           << "fi\n"
           << "echo ''\n"
           << "echo 'IEC61850 VM ready.'\n"
           << "echo 'Run: /mnt/host/build/iec61850_demo (server)'\n"
           << "echo '     /mnt/host/build/iec61850_client (client)'\n"
           << "echo ''\n"
           << "exec /bin/sh\n";
        return ss.str();
    }

    /**
     * @brief Sets up network TAP interfaces
     * @return 0 on success, non-zero on failure
     */
    int setupNetwork()
    {
        if (!isRoot())
        {
            std::cerr << "Error: Network setup requires root privileges\n";
            return 1;
        }

        std::vector<std::string> cmds = {
            "ip tuntap add dev tap0 mode tap",
            "ip tuntap add dev tap1 mode tap",
            "ip link add br-iec type bridge",
            "ip link set tap0 master br-iec",
            "ip link set tap1 master br-iec",
            "ip link set tap0 up",
            "ip link set tap1 up",
            "ip link set br-iec up",
            "ip addr add 192.168.100.1/24 dev br-iec"
        };

        for (const auto& cmd : cmds)
        {
            if (system(cmd.c_str()) != 0)
            {
                std::cerr << "Warning: Command failed: " << cmd << "\n";
            }
        }

        std::cout << "Network setup complete:\n"
                  << "  Bridge: br-iec (192.168.100.1/24)\n"
                  << "  TAP0:   tap0 (server)\n"
                  << "  TAP1:   tap1 (client)\n";
        return 0;
    }

    /**
     * @brief Cleans up network interfaces
     * @return 0 on success, non-zero on failure
     */
    int cleanupNetwork()
    {
        if (!isRoot())
        {
            std::cerr << "Error: Cleanup requires root privileges\n";
            return 1;
        }

        std::vector<std::string> cmds = {
            "ip link set br-iec down 2>/dev/null",
            "ip link del br-iec 2>/dev/null",
            "ip link del tap0 2>/dev/null",
            "ip link del tap1 2>/dev/null"
        };

        for (const auto& cmd : cmds)
        {
            system(cmd.c_str());
        }

        std::cout << "Network cleanup complete\n";
        return 0;
    }

    /**
     * @brief Creates minimal initramfs with busybox
     * @param outputPath Output path for initramfs
     * @return 0 on success, non-zero on failure
     */
    int createInitramfs(const fs::path& outputPath)
    {
        fs::path tempDir = fs::temp_directory_path() / "iec61850_initramfs";
        fs::remove_all(tempDir);
        fs::create_directories(tempDir);

        std::vector<std::string> dirs = {
            "bin", "sbin", "etc", "proc", "sys", "dev",
            "usr/bin", "usr/sbin", "lib", "lib64", "mnt/host", "run"
        };

        for (const auto& dir : dirs)
        {
            fs::create_directories(tempDir / dir);
        }

        std::string busyboxPath = execCommand("which busybox");
        if (busyboxPath.empty())
        {
            std::cerr << "Error: busybox not found. Install with: apt install busybox-static\n";
            return 1;
        }

        fs::copy_file(busyboxPath, tempDir / "bin/busybox", fs::copy_options::overwrite_existing);
        fs::permissions(tempDir / "bin/busybox", fs::perms::owner_all | fs::perms::group_exec | fs::perms::others_exec);

        std::vector<std::string> busyboxLinks = {
            "sh", "ls", "cat", "cp", "mv", "rm", "mkdir", "mount", "umount",
            "ip", "ifconfig", "ping", "ps", "kill", "echo", "sleep", "clear"
        };

        for (const auto& link : busyboxLinks)
        {
            fs::path linkPath = tempDir / "bin" / link;
            if (!fs::exists(linkPath))
            {
                fs::create_symlink("/bin/busybox", linkPath);
            }
        }

        fs::path initPath = tempDir / "init";
        std::ofstream initFile(initPath);
        initFile << createInitScript(getProjectRoot() / "build");
        initFile.close();
        fs::permissions(initPath, fs::perms::owner_all | fs::perms::group_exec | fs::perms::others_exec);

        std::string cpioCmd = "cd " + tempDir.string() + " && find . | cpio -o -H newc 2>/dev/null | gzip > " + outputPath.string();
        if (system(cpioCmd.c_str()) != 0)
        {
            std::cerr << "Error: Failed to create initramfs\n";
            return 1;
        }

        fs::remove_all(tempDir);
        std::cout << "Created initramfs: " << outputPath << "\n";
        return 0;
    }

    /**
     * @brief Finds a suitable kernel
     * @return Path to kernel or empty string
     */
    std::string findKernel()
    {
        std::vector<std::string> paths = {
            "/boot/vmlinuz-" + execCommand("uname -r"),
            "/boot/vmlinuz",
            "/vmlinuz"
        };

        for (const auto& path : paths)
        {
            if (fs::exists(path))
            {
                return path;
            }
        }
        return "";
    }

    /**
     * @brief Runs setup to prepare QEMU environment
     * @param root Project root
     * @return 0 on success, non-zero on failure
     */
    int runSetup(const fs::path& root)
    {
        fs::path qemuDir = root / QEMU_DIR;
        fs::create_directories(qemuDir);

        fs::path initramfsPath = qemuDir / INITRAMFS_NAME;
        int result = createInitramfs(initramfsPath);
        if (result != 0)
        {
            return result;
        }

        std::string kernel = findKernel();
        if (kernel.empty())
        {
            std::cerr << "Warning: No kernel found. You may need to specify --kernel\n";
        }
        else
        {
            std::cout << "Kernel found: " << kernel << "\n";
            std::string check9p = execCommand("grep -q 9P /boot/config-$(uname -r) 2>/dev/null && echo yes || echo no");
            if (check9p != "yes")
            {
                std::cout << "Note: Kernel 9p support status unknown. If mount fails, ensure kernel has 9P_FS enabled.\n";
            }
        }

        std::cout << "\nSetup complete. Next steps:\n"
                  << "  1. Build project: ./scripts/build.sh\n"
                  << "  2. Setup network (as root): sudo ./build/qemu_tool network\n"
                  << "  3. Start VMs: ./build/qemu_tool server\n"
                  << "                ./build/qemu_tool client\n";
        return 0;
    }

    /**
     * @brief Starts a QEMU VM
     * @param root Project root
     * @param role VM role (server/client)
     * @param tapInterface TAP interface name
     * @param memoryMb Memory in MB
     * @param kernelPath Custom kernel path
     * @return 0 on success, non-zero on failure
     */
    int startVM(const fs::path& root, const std::string& role,
                const std::string& tapInterface, int memoryMb,
                const std::string& kernelPath)
    {
        fs::path initramfsPath = root / QEMU_DIR / INITRAMFS_NAME;
        if (!fs::exists(initramfsPath))
        {
            std::cerr << "Error: Initramfs not found. Run 'setup' first.\n";
            return 1;
        }

        std::string kernel = kernelPath.empty() ? findKernel() : kernelPath;
        if (kernel.empty())
        {
            std::cerr << "Error: No kernel found. Specify with --kernel\n";
            return 1;
        }

        std::string tap = tapInterface.empty() ? (role == "server" ? "tap0" : "tap1") : tapInterface;
        std::string ip = role == "server" ? "192.168.100.10" : "192.168.100.11";

        std::ostringstream cmd;
        cmd << "qemu-system-x86_64"
            << " -enable-kvm"
            << " -m " << memoryMb
            << " -kernel " << kernel
            << " -initrd " << initramfsPath.string()
            << " -append \"console=ttyS0 quiet ip=" << ip << "::192.168.100.1:255.255.255.0::eth0:off\""
            << " -netdev tap,id=net0,ifname=" << tap << ",script=no,downscript=no"
            << " -device virtio-net-pci,netdev=net0"
            << " -fsdev local,id=fsdev0,path=" << root.string() << ",security_model=none"
            << " -device virtio-9p-pci,fsdev=fsdev0,mount_tag=host0"
            << " -nographic"
            << " -serial mon:stdio";

        std::cout << "Starting " << role << " VM on " << tap << "...\n";
        return system(cmd.str().c_str());
    }

    /**
     * @brief Shows current status
     * @return 0 on success
     */
    int showStatus()
    {
        std::cout << "Network Interfaces:\n";
        system("ip -br link show tap0 tap1 br-iec 2>/dev/null || echo '  No TAP interfaces found'");

        std::cout << "\nQEMU Processes:\n";
        system("pgrep -a qemu 2>/dev/null || echo '  No QEMU processes running'");

        return 0;
    }
}

/**
 * @brief Main entry point
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code
 */
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printUsage(argv[0]);
        return 1;
    }

    std::string command = argv[1];
    fs::path root = getProjectRoot();

    std::string tapInterface;
    std::string kernelPath;
    int memoryMb = VM_MEMORY_MB;

    for (int i = 2; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--interface" && i + 1 < argc)
        {
            tapInterface = argv[++i];
        }
        else if (arg == "--memory" && i + 1 < argc)
        {
            memoryMb = std::stoi(argv[++i]);
        }
        else if (arg == "--kernel" && i + 1 < argc)
        {
            kernelPath = argv[++i];
        }
    }

    if (command == "setup")
    {
        return runSetup(root);
    }
    else if (command == "server")
    {
        return startVM(root, "server", tapInterface, memoryMb, kernelPath);
    }
    else if (command == "client")
    {
        return startVM(root, "client", tapInterface, memoryMb, kernelPath);
    }
    else if (command == "network")
    {
        return setupNetwork();
    }
    else if (command == "cleanup")
    {
        return cleanupNetwork();
    }
    else if (command == "status")
    {
        return showStatus();
    }
    else if (command == "--help" || command == "-h")
    {
        printUsage(argv[0]);
        return 0;
    }
    else
    {
        std::cerr << "Unknown command: " << command << "\n";
        printUsage(argv[0]);
        return 1;
    }
}
