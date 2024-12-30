#pragma once

#include "common.hpp"

template <typename T>
struct Queue {
    virtual ~Queue() {}
    virtual void enqueue(T&& item) = 0;
    virtual std::optional<T> dequeue() = 0;
    virtual std::optional<T>& peek() = 0;
};

class FailableMessage {
public:
    int backoff = 0;
    sdbusplus::message_t msg;
    FailableMessage(sdbusplus::message_t&& msg) : msg(msg) {}

    bool call(sdbusplus::bus::bus& bus) {
        try {
            bus.call(this->msg);
            this->backoff = 0;
            return true;
        } catch (std::exception& e) {
            this->backoff = this->backoff == 0 ? 1 : (this->backoff * 2);
            std::cerr << "method call failed, waiting " << this->backoff << " : " << e.what() << std::endl;
            return false;
        }
    }
};

class SingleItemMQ : public Queue<FailableMessage> {
    std::optional<FailableMessage> msg;

public:
    std::mutex mtx;

    SingleItemMQ(): mtx() {}

    virtual void enqueue(FailableMessage&& msg) override {
        if (this->msg.has_value()) {
            this->msg.value().msg = msg.msg;
        }
        this->msg = msg;
    }

    virtual std::optional<FailableMessage> dequeue() override {
        auto tmp = std::move(this->msg);
        this->msg = {};
        return tmp;
    }

    virtual std::optional<FailableMessage>& peek() override {
        return this->msg;
    }
};

class RpcMessaging {

    sdbusplus::bus::bus bus;
    SingleItemMQ main;

    std::atomic_int waker;
    std::thread processor;

    void processingLoop() {
        while (true) {
            this->waker.wait(0);
            this->process();
        }
    }

    template<typename T>
    inline void processQueue(bool& flag, int& delay, T& queue) {
        std::scoped_lock lock(queue.mtx);
        while (true) {
            auto& msg = queue.peek();
            if (msg.has_value()) {
                if (msg->call(this->bus)) {
                    queue.dequeue();
                    this->waker.fetch_sub(1);
                } else {
                    delay = std::min(delay, msg->backoff);
                    flag = true;
                    return;
                }
            } else {
                return;
            }
        }
    }

    void process() {
        bool flag = true;
        while (flag) {
            int delay = 60;
            flag = false;
            {
                this->processQueue(flag, delay, main);
            }

            if (flag && delay > 0) {
                sleep(delay);
            }
        }
    }

public:
    RpcMessaging() :
            bus(sdbusplus::bus::new_default()),
            processor(std::thread([this](){ this->processingLoop(); })) {
        processor.detach();
    }

    static RpcMessaging& getInstance() {
        static RpcMessaging instance;
        return instance;
    }

    void mainUpdate(uint32_t pid, std::string windowInfo) {
        auto msg = this->bus.new_method_call("lolepopie.wlinspector", "/main", "lolepopie.wlinspector", "WindowInfo");
        msg.append(pid, windowInfo);
        {
            // std::scoped_lock lock(main.mtx); // why does this crash
            main.mtx.lock();
            main.enqueue(std::move(msg));
            main.mtx.unlock();
        }
        this->waker.store(1);
        this->waker.notify_one();
    }
};
