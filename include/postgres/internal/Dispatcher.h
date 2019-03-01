#pragma once

#include <functional>
#include <future>
#include <memory>
#include <utility>
#include <vector>
#include <postgres/internal/IChannel.h>

namespace postgres {

class Connection;
class Context;

}  // namespace postgres

namespace postgres::internal {

class Worker;

class Dispatcher {
public:
    explicit Dispatcher(std::shared_ptr<Context const> ctx, std::shared_ptr<IChannel> chan);
    Dispatcher(Dispatcher const& other) = delete;
    Dispatcher& operator=(Dispatcher const& other) = delete;
    Dispatcher(Dispatcher&& other) noexcept = delete;
    Dispatcher& operator=(Dispatcher&& other) noexcept = delete;
    ~Dispatcher() noexcept;

    template <typename T>
    std::future<T> send(std::function<T(Connection&)> job) {
        // todo: cannot compile with just packaged_task or unique_ptr :(
        auto task = std::make_shared<std::packaged_task<T(Connection&)>>(std::move(job));
        auto fut  = task->get_future();
        auto const[is_sent, worker] = chan_->send([task = std::move(task)](Connection& conn) mutable {
            (*task)(conn);
        });
        if (!is_sent) {
            scale(worker);
        }
        return fut;
    }

private:
    void scale(Worker* recycled);
    int size() const;

    std::shared_ptr<Context const>       ctx_;
    std::shared_ptr<IChannel>            chan_;
    std::vector<std::unique_ptr<Worker>> workers_;
};

}  // namespace postgres::internal