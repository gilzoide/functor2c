/** @file functor2c.hpp
 * Single header templates for wrapping C++ functors as opaque userdata plus function pointers for C interop.
 */
#ifndef __FUNCTOR2C_HPP__
#define __FUNCTOR2C_HPP__

#include <functional>
#include <utility>

namespace functor2c {

namespace detail {

/**
 * Wrapper for `std::function` with helper methods to get invoker/deleter function pointers.
 * @private
 */
template<bool destroy_on_invoke, typename RetType, typename... Args>
struct destroyable_function {
	/**
	 * Deleter used to wrap destroyable function in `std::unique_ptr`.
	 * @private
	 */
	struct deleter {
		void operator()(void *userdata) const {
			destroy(userdata);
		}
	};

	template<typename Fn>
	destroyable_function(Fn&& fn) : function(std::move(fn)) {}

	RetType operator()(Args... args) {
		std::unique_ptr<destroyable_function> destroyer;
		if (destroy_on_invoke) {
			destroyer = std::unique_ptr<destroyable_function>(this);
		}
		return function(std::forward<Args>(args)...);
	}

	std::tuple<void*, RetType (*)(void*, Args...)> prefix_invoker() {
		return std::make_tuple(static_cast<void*>(this), invoke_prefix);
	}
	std::tuple<void*, RetType (*)(void*, Args...), void (*)(void*)> prefix_invoker_deleter() {
		return std::make_tuple(static_cast<void*>(this), invoke_prefix, destroy);
	}
	std::tuple<std::unique_ptr<void, deleter>, RetType (*)(void*, Args...)> prefix_invoker_unique() {
		return std::make_tuple(std::unique_ptr<void, deleter>(static_cast<void*>(this)), invoke_prefix);
	}
	std::tuple<std::shared_ptr<void>, RetType (*)(void*, Args...)> prefix_invoker_shared() {
		return std::make_tuple(std::shared_ptr<void>(static_cast<void*>(this), destroy), invoke_prefix);
	}

	std::tuple<RetType (*)(Args..., void*), void*> suffix_invoker() {
		return std::make_tuple(invoke_suffix, static_cast<void*>(this));
	}
	std::tuple<RetType (*)(Args..., void*), void*, void (*)(void*)> suffix_invoker_deleter() {
		return std::make_tuple(invoke_suffix, static_cast<void*>(this), destroy);
	}
	std::tuple<RetType (*)(Args..., void*), std::unique_ptr<void, deleter>> suffix_invoker_unique() {
		return std::make_tuple(invoke_suffix, std::unique_ptr<void, deleter>(static_cast<void*>(this)));
	}
	std::tuple<RetType (*)(Args..., void*), std::shared_ptr<void>> suffix_invoker_shared() {
		return std::make_tuple(invoke_suffix, std::shared_ptr<void>(static_cast<void*>(this), destroy));
	}

	static RetType invoke_prefix(void *userdata, Args... args) {
		auto self = static_cast<destroyable_function*>(userdata);
		return (*self)(std::forward<Args>(args)...);
	}

	static RetType invoke_suffix(Args... args, void *userdata) {
		auto self = static_cast<destroyable_function*>(userdata);
		return (*self)(std::forward<Args>(args)...);
	}

	static void destroy(void *userdata) {
		auto self = static_cast<destroyable_function*>(userdata);
		delete self;
	}

private:
	std::function<RetType(Args...)> function;
};

}

/**
 * Transform `fn` into a [userdata, invoker, deleter] tuple.
 *
 * The invoker accepts the same parameters as `fn`, with the addition of the `userdata` prefix argument.
 *
 * @note You are responsible for calling the deleter with the userdata as parameter to reclaim allocated memory.
 *
 * @code
 * auto [userdata, invoker, deleter] = prefix_invoker_deleter([](int value) {});
 * // Invoke wrapped function as many times as you need.
 * invoker(userdata, 1);
 * invoker(userdata, 2);
 * invoker(userdata, 3);
 * // Delete wrapped function afterwards to avoid memory leak.
 * deleter(userdata);
 * @endcode
 *
 * @return Tuple containing an opaque userdata, plus its invoker and deleter functions.
 */
template<typename RetType, typename... Args, typename Fn>
std::tuple<void*, RetType (*)(void*, Args...), void (*)(void*)> prefix_invoker_deleter(Fn&& fn) {
	return (new detail::destroyable_function<false, RetType, Args...>(std::move(fn)))->prefix_invoker_deleter();
}

/**
 * Transform `fn` into a [userdata, oneshot_invoker] tuple.
 *
 * The invoker accepts the same parameters as `fn`, with the addition of the `userdata` prefix argument.
 * The allocated memory will be freed in the first invocation, so you must invoke it exactly once.
 *
 * @warning If you don't invoke the function, memory will leak.
 * @warning If you try to invoke the function twice, your app will try to access freed memory and likely SEGFAULT.
 *
 * @code
 * auto [userdata, oneshot_invoker] = prefix_invoker_oneshot([](int value) {});
 * // Invoke wrapped function exactly once
 * oneshot_invoker(userdata, 42);
 * @endcode
 *
 * @return Tuple containing an opaque userdata, plus its oneshot invoker function.
 */
template<typename RetType, typename... Args, typename Fn>
std::tuple<void*, RetType (*)(void*, Args...)> prefix_invoker_oneshot(Fn&& fn) {
	return (new detail::destroyable_function<true, RetType, Args...>(std::move(fn)))->prefix_invoker();
}

/**
 * Transform `fn` into a [userdata, invoker] tuple.
 *
 * The invoker accepts the same parameters as `fn`, with the addition of the `userdata` prefix argument.
 *
 * @note The userdata is owned by a `std::unique_ptr`, so there's no need to manually delete it.
 *
 * @code
 * auto [userdata, invoker] = prefix_invoker_unique([](int value) {});
 * // Invoke wrapped function as many times as you need.
 * invoker(userdata.get(), 1);
 * invoker(userdata.get(), 2);
 * invoker(userdata.get(), 3);
 * // Userdata is a unique_ptr, memory will be freed automatically.
 * @endcode
 *
 * @return Tuple containing an opaque userdata, plus its invoker and deleter functions.
 */
template<typename RetType, typename... Args, typename Fn>
std::tuple<std::unique_ptr<void, typename detail::destroyable_function<false, RetType, Args...>::deleter>, RetType (*)(void*, Args...)> prefix_invoker_unique(Fn&& fn) {
	return (new detail::destroyable_function<false, RetType, Args...>(std::move(fn)))->prefix_invoker_unique();
}

/**
 * Transform `fn` into a [userdata, invoker] tuple.
 *
 * The invoker accepts the same parameters as `fn`, with the addition of the `userdata` prefix argument.
 *
 * @note The userdata is retained by a `std::shared_ptr`, so there's no need to manually delete it.
 *
 * @code
 * auto [userdata, invoker] = prefix_invoker_shared([](int value) {});
 * // Invoke wrapped function as many times as you need.
 * invoker(userdata.get(), 1);
 * invoker(userdata.get(), 2);
 * invoker(userdata.get(), 3);
 * // Userdata is a shared_ptr, memory will be freed automatically.
 * @endcode
 *
 * @return Tuple containing an opaque userdata, plus its invoker and deleter functions.
 */
template<typename RetType, typename... Args, typename Fn>
std::tuple<std::shared_ptr<void>, RetType (*)(void*, Args...)> prefix_invoker_shared(Fn&& fn) {
	return (new detail::destroyable_function<false, RetType, Args...>(std::move(fn)))->prefix_invoker_shared();
}


/**
 * Same as `prefix_invoker_deleter` where the invoker accepts userdata parameter suffix instead of prefix.
 *
 * @code
 * auto [userdata, invoker, deleter] = suffix_invoker_deleter([](int value) {});
 * // Invoke wrapped function as many times as you need.
 * invoker(1, userdata);
 * invoker(2, userdata);
 * invoker(3, userdata);
 * // Delete wrapped function afterwards to avoid memory leak.
 * deleter(userdata);
 * @endcode
 */
template<typename RetType, typename... Args, typename Fn>
std::tuple<void*, RetType (*)(Args..., void*), void (*)(void*)> suffix_invoker_deleter(Fn&& fn) {
	return (new detail::destroyable_function<false, RetType, Args...>(std::move(fn)))->suffix_invoker_deleter();
}

/**
 * Same as `prefix_invoker_oneshot` where the invoker accepts userdata parameter suffix instead of prefix.
 *
 * @code
 * auto [userdata, oneshot_invoker] = suffix_invoker_oneshot([](int value) {});
 * // Invoke wrapped function exactly once
 * oneshot_invoker(42, userdata);
 * @endcode
 */
template<typename RetType, typename... Args, typename Fn>
std::tuple<void*, RetType (*)(Args..., void*)> suffix_invoker_oneshot(Fn&& fn) {
	return (new detail::destroyable_function<true, RetType, Args...>(std::move(fn)))->suffix_invoker();
}

/**
 * Same as `prefix_invoker_unique` where the invoker accepts userdata parameter suffix instead of prefix.
 *
 * @code
 * auto [userdata, invoker] = suffix_invoker_unique([](int value) {});
 * // Invoke wrapped function as many times as you need.
 * invoker(1, userdata.get());
 * invoker(2, userdata.get());
 * invoker(3, userdata.get());
 * // Userdata is a unique_ptr, memory will be freed automatically.
 * @endcode
 */
template<typename RetType, typename... Args, typename Fn>
std::tuple<std::unique_ptr<void, typename detail::destroyable_function<false, RetType, Args...>::deleter>, RetType (*)(Args..., void*)> suffix_invoker_unique(Fn&& fn) {
	return (new detail::destroyable_function<false, RetType, Args...>(std::move(fn)))->suffix_invoker_unique();
}

/**
 * Same as `prefix_invoker_shared` where the invoker accepts userdata parameter suffix instead of prefix.
 *
 * @code
 * auto [userdata, invoker] = suffix_invoker_shared([](int value) {});
 * // Invoke wrapped function as many times as you need.
 * invoker(1, userdata.get());
 * invoker(2, userdata.get());
 * invoker(3, userdata.get());
 * // Userdata is a shared_ptr, memory will be freed automatically.
 * @endcode
 */
template<typename RetType, typename... Args, typename Fn>
std::tuple<std::shared_ptr<void>, RetType (*)(Args..., void*)> suffix_invoker_shared(Fn&& fn) {
	return (new detail::destroyable_function<false, RetType, Args...>(std::move(fn)))->suffix_invoker_shared();
}


#if __cplusplus >= 201703L

/// Overload used for automatic type deduction in C++17
template<typename RetType, typename... Args>
auto prefix_invoker_deleter(std::function<RetType(Args...)>&& fn) {
	return (new detail::destroyable_function<false, RetType, Args...>(std::move(fn)))->prefix_invoker_deleter();
}
/// Overload used for automatic type deduction in C++17
template<typename Fn>
auto prefix_invoker_deleter(Fn&& fn) {
	return prefix_invoker_deleter(std::move(std::function(fn)));
}

/// Overload used for automatic type deduction in C++17
template<typename RetType, typename... Args>
auto prefix_invoker_oneshot(std::function<RetType(Args...)>&& fn) {
	return (new detail::destroyable_function<true, RetType, Args...>(std::move(fn)))->prefix_invoker();
}
/// Overload used for automatic type deduction in C++17
template<typename Fn>
auto prefix_invoker_oneshot(Fn&& fn) {
	return prefix_invoker_oneshot(std::move(std::function(fn)));
}

/// Overload used for automatic type deduction in C++17
template<typename RetType, typename... Args>
auto prefix_invoker_unique(std::function<RetType(Args...)>&& fn) {
	return (new detail::destroyable_function<false, RetType, Args...>(std::move(fn)))->prefix_invoker_unique();
}
/// Overload used for automatic type deduction in C++17
template<typename Fn>
auto prefix_invoker_unique(Fn&& fn) {
	return prefix_invoker_unique(std::move(std::function(fn)));
}

/// Overload used for automatic type deduction in C++17
template<typename RetType, typename... Args>
auto prefix_invoker_shared(std::function<RetType(Args...)>&& fn) {
	return (new detail::destroyable_function<false, RetType, Args...>(std::move(fn)))->prefix_invoker_shared();
}
/// Overload used for automatic type deduction in C++17
template<typename Fn>
auto prefix_invoker_shared(Fn&& fn) {
	return prefix_invoker_shared(std::move(std::function(fn)));
}

/// Overload used for automatic type deduction in C++17
template<typename RetType, typename... Args>
auto suffix_invoker_deleter(std::function<RetType(Args...)>&& fn) {
	return (new detail::destroyable_function<false, RetType, Args...>(std::move(fn)))->suffix_invoker_deleter();
}
/// Overload used for automatic type deduction in C++17
template<typename Fn>
auto suffix_invoker_deleter(Fn&& fn) {
	return suffix_invoker_deleter(std::move(std::function(fn)));
}

/// Overload used for automatic type deduction in C++17
template<typename RetType, typename... Args>
auto suffix_invoker_oneshot(std::function<RetType(Args...)>&& fn) {
	return (new detail::destroyable_function<true, RetType, Args...>(std::move(fn)))->suffix_invoker();
}
/// Overload used for automatic type deduction in C++17
template<typename Fn>
auto suffix_invoker_oneshot(Fn&& fn) {
	return suffix_invoker_oneshot(std::move(std::function(fn)));
}

/// Overload used for automatic type deduction in C++17
template<typename RetType, typename... Args>
auto suffix_invoker_unique(std::function<RetType(Args...)>&& fn) {
	return (new detail::destroyable_function<false, RetType, Args...>(std::move(fn)))->suffix_invoker_unique();
}
/// Overload used for automatic type deduction in C++17
template<typename Fn>
auto suffix_invoker_unique(Fn&& fn) {
	return suffix_invoker_unique(std::move(std::function(fn)));
}

/// Overload used for automatic type deduction in C++17
template<typename RetType, typename... Args>
auto suffix_invoker_shared(std::function<RetType(Args...)>&& fn) {
	return (new detail::destroyable_function<false, RetType, Args...>(std::move(fn)))->suffix_invoker_shared();
}
/// Overload used for automatic type deduction in C++17
template<typename Fn>
auto suffix_invoker_shared(Fn&& fn) {
	return prefix_invoker_shared(std::move(std::function(fn)));
}

#endif

}

#endif  // __FUNCTOR2C_HPP__
