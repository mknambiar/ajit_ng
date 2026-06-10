#pragma once
#include <coroutine>
#include <cstdint>
#include <exception>
#include <optional>
#include <type_traits>
#include <utility>

// Minimal interface that any "owner" (module, testbench, etc.) can implement
// to let the coroutine runtime know which coroutine handle should be resumed next.
struct CoroutineOwner {
  virtual void set_active(std::coroutine_handle<> h) noexcept = 0;
  virtual std::coroutine_handle<> get_active() const noexcept = 0;
  virtual ~CoroutineOwner() = default;
};

struct StepPromiseBase {
  CoroutineOwner* owner = nullptr;            // owning object (for active-handle tracking)
  std::coroutine_handle<> continuation = {};  // awaiting coroutine (parent)

  std::suspend_always initial_suspend() noexcept { return {}; } // start suspended

  struct FinalAwaiter {
    bool await_ready() noexcept { return false; }
    void await_resume() noexcept {}

    template <typename PromiseT>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<PromiseT> h) noexcept {
      auto &p = h.promise();
      auto cont = p.continuation;

      if (p.owner) {
        // After finishing, the continuation becomes the next active coroutine (or empty if none).
        p.owner->set_active(cont ? cont : std::coroutine_handle<>{});
      }

      return cont ? cont : std::noop_coroutine();
    }
  };

  FinalAwaiter final_suspend() noexcept { return {}; }
  void unhandled_exception() { std::terminate(); }
};

// StepTaskT<T>: coroutine type with nested co_await and external step-by-step resumption.
// Use T=void for "no value"; use non-void T for typed return values.
template <typename T>
struct StepTaskT {
  struct promise_type {
    static_assert(!std::is_void_v<T>, "Use StepTaskT<void> specialization for void result");
    CoroutineOwner* owner = nullptr;
    std::coroutine_handle<> continuation = {};
    std::optional<T> result;

    StepTaskT get_return_object() {
      return StepTaskT{std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    std::suspend_always initial_suspend() noexcept { return {}; }
    StepPromiseBase::FinalAwaiter final_suspend() noexcept { return {}; }
    void return_value(T value) noexcept { result = std::move(value); }
    void unhandled_exception() { std::terminate(); }
  };

  std::coroutine_handle<promise_type> h{};

  StepTaskT() = default;
  explicit StepTaskT(std::coroutine_handle<promise_type> handle) : h(handle) {}

  StepTaskT(const StepTaskT&) = delete;
  StepTaskT& operator=(const StepTaskT&) = delete;

  StepTaskT(StepTaskT&& other) noexcept : h(std::exchange(other.h, {})) {}
  StepTaskT& operator=(StepTaskT&& other) noexcept {
    if (this != &other) {
      if (h) h.destroy();
      h = std::exchange(other.h, {});
    }
    return *this;
  }

  ~StepTaskT() { if (h) h.destroy(); }

  explicit operator bool() const noexcept { return (bool)h; }

  void set_owner(CoroutineOwner* o) noexcept {
    if (h) h.promise().owner = o;
  }

  // Awaiter: lets one StepTask await another (parent awaits child).
  struct Awaiter {
    std::coroutine_handle<promise_type> child;

    bool await_ready() const noexcept { return !child || child.done(); }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> parent) noexcept {
      auto &p = child.promise();
      p.continuation = parent;

      if (p.owner) {
        // Child becomes active so the external scheduler can resume the leaf.
        p.owner->set_active(child);
      }
      return child; // transfer execution into child
    }

    T await_resume() const noexcept { return *(child.promise().result); }
  };

  Awaiter operator co_await() const noexcept { return Awaiter{h}; }
};

template <>
struct StepTaskT<void> {
  struct promise_type : public StepPromiseBase {
    StepTaskT get_return_object() {
      return StepTaskT{std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    void return_void() noexcept {}
  };

  std::coroutine_handle<promise_type> h{};

  StepTaskT() = default;
  explicit StepTaskT(std::coroutine_handle<promise_type> handle) : h(handle) {}

  StepTaskT(const StepTaskT&) = delete;
  StepTaskT& operator=(const StepTaskT&) = delete;

  StepTaskT(StepTaskT&& other) noexcept : h(std::exchange(other.h, {})) {}
  StepTaskT& operator=(StepTaskT&& other) noexcept {
    if (this != &other) {
      if (h) h.destroy();
      h = std::exchange(other.h, {});
    }
    return *this;
  }

  ~StepTaskT() { if (h) h.destroy(); }

  explicit operator bool() const noexcept { return (bool)h; }

  void set_owner(CoroutineOwner* o) noexcept {
    if (h) h.promise().owner = o;
  }

  struct Awaiter {
    std::coroutine_handle<promise_type> child;

    bool await_ready() const noexcept { return !child || child.done(); }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> parent) noexcept {
      auto &p = child.promise();
      p.continuation = parent;

      if (p.owner) {
        p.owner->set_active(child);
      }
      return child;
    }

    void await_resume() const noexcept {}
  };

  Awaiter operator co_await() const noexcept { return Awaiter{h}; }
};

using StepTask = StepTaskT<void>;

// Yield awaiter: suspends the CURRENT coroutine back to the external scheduler (e.g., owner->run()).
// It also records the yielding coroutine handle into owner->active so next run() resumes the leaf.
struct YieldAwaiter {
  CoroutineOwner* owner = nullptr;

  bool await_ready() const noexcept { return false; }
  void await_resume() const noexcept {}

  template <typename PromiseT>
  void await_suspend(std::coroutine_handle<PromiseT> h) const noexcept {
    if (owner) owner->set_active(h);
  }
};

inline YieldAwaiter yield_point(CoroutineOwner* o) noexcept { return YieldAwaiter{o}; }

inline StepTask wait_penalty_cycles(CoroutineOwner* owner, uint64_t cycles)
{
  for (uint64_t i = 0; i < cycles; ++i) {
    co_await yield_point(owner);
  }
}

// Helpers to reduce repeated "set_owner(owner); co_await task;" boilerplate.
// Example:
//   StepTaskT<int> t = foo(owner, ...);
//   CO_AWAIT_OWNED(owner, t);
//
//   int rc = 0;
//   CO_AWAIT_OWNED_TO(rc, owner, foo(owner, ...));
#define CO_AWAIT_OWNED(owner, task_var)              \
  do {                                               \
    (task_var).set_owner((owner));                   \
    co_await (task_var);                             \
  } while (0)

#define CO_AWAIT_OWNED_VAL(out_var, owner, task_var) \
  do {                                                \
    (task_var).set_owner((owner));                    \
    (out_var) = co_await (task_var);                  \
  } while (0)

#define CO_AWAIT_OWNED_TO(out_var, owner, task_expr) \
  do {                                                \
    auto __co_task = (task_expr);                     \
    __co_task.set_owner((owner));                     \
    (out_var) = co_await __co_task;                   \
  } while (0)
