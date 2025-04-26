#include <catch2/catch_test_macros.hpp>
#include "../functor2c.hpp"

TEST_CASE("Test") {
	auto [userdata, invoker, deleter] = functor2c::prefix_invoker_deleter([](int a) {
		REQUIRE(true);
	});

	invoker(userdata, 2);
	deleter(userdata);
}
TEST_CASE("Test <void>") {
	auto [userdata, invoker, deleter] = functor2c::prefix_invoker_deleter<void>([]() {
		REQUIRE(true);
	});

	invoker(userdata);
	deleter(userdata);
}

TEST_CASE("Test2") {
	auto [userdata, invoker] = functor2c::prefix_invoker_oneshot([]() {
		REQUIRE(true);
	});

	invoker(userdata);
}
TEST_CASE("Test2 <void>") {
	auto [userdata, invoker] = functor2c::prefix_invoker_oneshot<void>([]() {
		REQUIRE(true);
	});

	invoker(userdata);
}

TEST_CASE("Test Unique") {
	auto [userdata, invoker] = functor2c::prefix_invoker_unique([]() {
		REQUIRE(true);
	});

	invoker(userdata.get());
}
TEST_CASE("Test Unique <void>") {
	auto [userdata, invoker] = functor2c::prefix_invoker_unique<void>([]() {
		REQUIRE(true);
	});

	invoker(userdata.get());
}

TEST_CASE("Test Shared") {
	auto [userdata, invoker] = functor2c::prefix_invoker_shared([]() {
		REQUIRE(true);
	});

	invoker(userdata.get());
}
TEST_CASE("Test Shared <void>") {
	auto [userdata, invoker] = functor2c::prefix_invoker_shared<void>([]() {
		REQUIRE(true);
	});

	invoker(userdata.get());
}
