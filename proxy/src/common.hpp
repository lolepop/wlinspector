#pragma once

#include <cstdint>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>
#include <typeinfo>
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <cerrno>
#include <cstdarg>
#include <cstddef>
#include <ios>
#include <format>
#include <fstream>
#include <dlfcn.h>
#include <unordered_map>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-util.h>
#include <sdbusplus/bus.hpp>
#include <nlohmann/json.hpp>

#include "argparser.hpp"
#include "dbus.hpp"

// this sucks
class Logger {
private:
    std::ofstream f;
    Logger() {
#ifdef DEBUG
        this->f.open("/tmp/thelog.txt", std::ios_base::app);
#endif
    }
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void log(std::string s) {
#ifdef DEBUG
        this->f << s << "\n";
        this->f.flush();
#endif
    }
};
