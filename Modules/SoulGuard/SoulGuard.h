/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include <variant>
#include <unordered_map>

#include "glog.h"
#include "main.h"
#include "soul.h"
#include "settings.h"

#include "Watchdogs.h"
#include "CodeStopwatch.h"
#include "TypeListBuilder.h"


template<class... Watchdogs>
struct SoulGuard
{
private:
	static_assert(
		std::is_same_v<
			typename utl::variant_factory<utl::typelist_t<Watchdogs...>>::VARIANT,
			typename utl::variant_factory<utl::removed_duplicates_t<Watchdogs...>>::VARIANT
		>,
		"Soul guard repeated units"
	);
	static_assert(!utl::empty(typename utl::typelist_t<Watchdogs...>::RESULT{}), "Watchdogs list must not be empty");


	static constexpr char TAG[] = "SGRD";

	using watchdog_v = std::variant<Watchdogs...>;

	void check(watchdog_v watchdog)
	{
		auto lambda = [](auto& wtchdg) {
			wtchdg.check();
		};

		std::visit(lambda, watchdog);
	}

	using watchdogs_pack = utl::simple_list_t<Watchdogs...>;

	std::unordered_map<unsigned, watchdog_v> watchdogs;
	unsigned index;

	template<class... WList>
	void set_watchdogs(utl::simple_list_t<WList...>)
	{
		(set_watchdog(utl::getType<WList>{}), ...);
	}

	template<class Watchdog>
	void set_watchdog(utl::getType<Watchdog>)
	{
		watchdogs.insert({index++, Watchdog{}});
	}

public:
	SoulGuard()
	{
		index = 0;
		set_watchdogs(watchdogs_pack{});
	}

	void defend()
	{
		auto it = watchdogs.find(index++);
		if (it == watchdogs.end()) {
			index = 0;
			return;
		}

		auto lambda = [] (auto& watchdog) {
			watchdog.check();
		};

		std::visit(lambda, it->second);
	}
};
