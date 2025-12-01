#pragma once

#include <functional>
#include <mutex>
#include <list>
#include <thread>
#include <memory>



template <typename... Args>
class AsyncConnection {
public:
    AsyncConnection(std::function<void(Args...)> callable, bool runAsync = true)
        : callableT(std::move(callable)),
          _runAsync(runAsync) {}

    AsyncConnection(const AsyncConnection&) = default;
    AsyncConnection(AsyncConnection&&) noexcept = default;

    AsyncConnection& operator=(const AsyncConnection&) = default;
    AsyncConnection& operator=(AsyncConnection&&) noexcept = default;

    std::function<void(Args...)> callableT;
    bool _runAsync;
};

template <typename... Args> class Signal {
public:
    using HandlerId = std::size_t;
    static constexpr HandlerId MaxId = std::numeric_limits<HandlerId>::max();

    Signal()
    : syncMutex(std::make_shared<std::recursive_mutex>()),
      currentId(0),
      handlersDetails()
{
}


Signal(const Signal& s)
    : syncMutex(s.syncMutex),
      currentId(s.currentId)
{
    std::lock_guard<std::recursive_mutex> lock(*syncMutex);
    handlersDetails = s.handlersDetails;


    for (auto& h : handlersDetails) {
        h.second.thread.reset();
    }
}    
    
    
Signal(Signal&& s) noexcept
    : currentId(s.currentId),
      handlersDetails(std::move(s.handlersDetails)),
      syncMutex(std::move(s.syncMutex)) 
{

    s.currentId = 0;
    s.handlersDetails.clear();
    if (!s.syncMutex) {
        s.syncMutex = std::make_shared<std::recursive_mutex>();
    }
}

    size_t ConnectionCount() const {
        std::lock_guard<std::recursive_mutex> lock(*syncMutex);
        return handlersDetails.size();
    }
template <typename T>
[[nodiscard]] HandlerId AddHandler(T&& sub) {
    std::lock_guard<std::recursive_mutex> lock(*syncMutex);

    if (ConnectionCount() > MaxId - 1)
        return std::numeric_limits<HandlerId>::max();

    auto id = currentId++;


    handlersDetails.emplace_back(
        id,
        activate{
            sub._runAsync,
            id,
            [handler = std::move(sub.callableT)](Args... args) mutable {
                handler(args...);
            }
        }
    );

    return id;
}

    [[nodiscard]] bool RemoveHandler(HandlerId id) {
        std::lock_guard<std::recursive_mutex> lock(*syncMutex);
        auto it = std::find_if(handlersDetails.begin(), handlersDetails.end(), [&](auto& i) {
            return i.first == id;
        });
        if(it != handlersDetails.end()) {
            handlersDetails.erase(it);
            return true;
        }
        return false;
    }
    template <typename... Args2>
    void operator()(Args2&&... args) {
        Emit(std::forward<Args2>(args) ...);
    }
    template<typename T>
    [[nodiscard]] HandlerId operator+=(T&& t) {
        return AddHandler(std::forward<T>(t));
    }
    void operator -= (HandlerId id) {
        RemoveHandler(id);
    }

void TryJoinOnAll() {
    std::lock_guard<std::recursive_mutex> lock(*syncMutex);
    for(auto& detail: handlersDetails) {
        auto& act = detail.second;
        if(act.runAsync && act.thread && act.thread->joinable()) {
            act.thread->join();
            act.thread.reset(); 
        }
    }
}


template <typename... Args2>
void Emit(Args2&&... args) {
    std::lock_guard<std::recursive_mutex> lock(*syncMutex);

    for (auto& detail : handlersDetails) {
        auto& act = detail.second;

        if (act.runAsync) {

            auto handlerCopy = act.handlerFunc;
            act.thread = std::make_unique<std::thread>(
                [handlerCopy, &args...]() mutable {
                    std::apply(handlerCopy, std::forward_as_tuple(args...));
                }
            );
        } else {

            act.handlerFunc(std::forward<Args2>(args)...);
        }
    }
}

    void ClearHandlers() {
        std::lock_guard<std::recursive_mutex> lock(*syncMutex);
        handlersDetails.clear();
    }
    void DisconnectFromAllSignals() {
        ClearHandlers();
    }
private:
    struct activate {
        activate(bool async, std::size_t id, std::function<void(Args...)> h)
            : runAsync{async}, handlerFunc{h}, thread{}, _id{id} {
        }
        activate(const activate& a) {
            DoAssignments(a);
        }
        activate(activate&& a) {
            DoAssignments(a);
        }


        ~activate() {
            if(thread && thread->joinable())
              thread->join();
        }
        activate& operator=(const activate& a) {
            DoAssignments(a);
            return *this;
        }
        activate& operator=(activate&& a) {
            DoAssignments(a);
            return *this;
        }
        
    
        void DoAssignments(activate&& a) {
            handlerFunc = std::move(a.handlerFunc);
            _id = a._id;
            runAsync = a.runAsync;
        }
        void DoAssignments(const activate& a) {
            handlerFunc = a.handlerFunc;
            _id = a._id;
            runAsync = a.runAsync;
        }        bool runAsync;
        
        std::function<void(Args ...)> handlerFunc;
        std::unique_ptr<std::thread> thread;
        std::size_t _id;
    };
    
    using details = std::pair<size_t, activate>;
    std::shared_ptr<std::recursive_mutex> syncMutex;
    size_t currentId;
    std::list<details> handlersDetails;
};




