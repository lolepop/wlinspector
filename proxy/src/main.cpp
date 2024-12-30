#include "common.hpp"

struct Window {
    std::optional<std::string> appId;
    std::optional<std::string> title;

    bool isFullyConstructed() {
        return this->appId.has_value() && this->title.has_value();
    }

    void setAppId(std::string&& appId) {
        this->appId = appId;
    }

    void setTitle(std::string&& title) {
        this->title = title;
    }
};

class Registry {
    std::mutex mtx;
    std::unordered_map<const wl_proxy*, const wl_interface*> interfaces;
    std::unordered_map<long, Window> windows;
    Registry() {}

    std::optional<std::string> serialiseWindows() {
        nlohmann::json o = nlohmann::json::object();
        bool flag = false;
        for (auto i = this->windows.begin(); i != this->windows.end(); i++) {
            auto& [k, v] = *i;
            if (v.isFullyConstructed()) {
                nlohmann::json w = nlohmann::json::object();
                w["appId"] = v.appId.value();
                w["title"] = v.title.value();
                o[std::to_string(k)] = w;
                flag = true;
            }
        }
        if (o.is_null() || !flag) {
            return {};
        }
        return o.dump();
    }

    void queueWindowUpdate() {
        auto serialised = this->serialiseWindows();
        if (serialised.has_value()) {
            RpcMessaging::getInstance().mainUpdate(getpid(), std::move(serialised.value()));
        }
    }

    const wl_interface* getRegisteredInterfaceImpl(const wl_proxy* proxy) {
        return this->interfaces.contains(proxy) ? this->interfaces[proxy] : nullptr;
    }

public:
    static Registry& getInstance() {
        static Registry instance;
        return instance;
    }

    void registerInterface(const wl_proxy* proxy, const wl_interface* interface) {
        std::scoped_lock lock(mtx);
        this->interfaces[proxy] = interface;
    }

    const wl_interface* getRegisteredInterface(const wl_proxy* proxy) {
        std::scoped_lock lock(mtx);
        return this->getRegisteredInterfaceImpl(proxy);
    }

    void handleInterfaceCall(wl_proxy* proxy, uint32_t opcode, wl_argument* args, wl_proxy* ret) {
        std::scoped_lock lock(mtx);
        auto registeredInterface = this->getRegisteredInterfaceImpl(proxy);
        if (registeredInterface == nullptr || opcode >= registeredInterface->method_count)
            return;

        auto registeredInterfaceName = std::string(registeredInterface->name);
        auto requestedOp = registeredInterface->methods[opcode];
        auto requestedOpName = std::string(requestedOp.name);
        auto argParser = ArgParser(args, requestedOp.signature);
        Logger::getInstance().log(std::format("handling wl method: {0} ({1})", registeredInterfaceName, requestedOpName));

        if (registeredInterfaceName == "xdg_toplevel") {
            if (requestedOpName == "set_title") {
                auto title = std::string(argParser.next<const char*>());
                Logger::getInstance().log(std::format("set_title: {0}", title));
                this->windows[(long)proxy].setTitle(std::move(title));
                this->queueWindowUpdate();
            } else if (requestedOpName == "set_app_id") {
                auto appId = std::string(argParser.next<const char*>());
                Logger::getInstance().log(std::format("set_app_id: {0}", appId));
                this->windows[(long)proxy].setAppId(std::move(appId));
                this->queueWindowUpdate();
            } else if (requestedOpName == "destroy") {
                this->windows.erase((long)proxy);
                this->queueWindowUpdate();
            }
        } else if (registeredInterfaceName == "xdg_surface") {
            if (requestedOpName == "get_toplevel") {
                this->windows[(long)ret] = {};
                this->queueWindowUpdate();
            }
        }
    }

    void print() {
        for (auto i = this->interfaces.begin(); i != this->interfaces.end(); i++) {
            Logger::getInstance().log(std::format("{0}", i->second->name));
        }
    }
};

void dumpMessages(const wl_message* messages, int count) {
    std::string s;
    for (int i = 0; i < count; i++) {
        // messages[i]
        s += std::format("{0}: {1}\n", messages[i].name, messages[i].signature);
    }
    Logger::getInstance().log(s);
}

// #define CALL_ORIG(fn) \
//     void* args = __builtin_apply_args(); \
//     void* ret = __builtin_apply((void(*)(...))dlsym(RTLD_NEXT, fn), args, 50);

struct wl_proxy * wl_proxy_marshal_array_flags(struct wl_proxy *proxy, uint32_t opcode, const struct wl_interface *interface, uint32_t version, uint32_t flags, wl_argument *args) {
    // CALL_ORIG("wl_proxy_marshal_flags");
    wl_proxy* ret = ((wl_proxy*(*)(...))dlsym(RTLD_NEXT, "wl_proxy_marshal_array_flags"))(proxy, opcode, interface, version, flags, args);

    auto& registry = Registry::getInstance();
    auto& logger = Logger::getInstance();

    if (interface != nullptr) {
        registry.registerInterface(ret, interface);
        logger.log(std::format("registered interface ({0}): {1}", (long)ret, interface->name));
    } else {
        registry.handleInterfaceCall(proxy, opcode, args, ret);
    }
    // __builtin_return(ret);
    return ret;
}
