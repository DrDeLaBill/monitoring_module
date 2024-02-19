/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include <variant>

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

	using watchdog_v = std::variant<Watchdogs...>;

	void check(watchdog_v watchdog)
	{
		auto lambda = [](auto& wtchdg) {
			wtchdg.check();
		};

		std::visit(lambda, watchdog);
	}

public:
	void defend()
	{
		(this->check(Watchdogs{}), ...);
	}

	bool hasErrors()
	{
		utl::CodeStopwatch stopwatch("GRD", GENERAL_TIMEOUT_MS);
		for (unsigned i = ERRORS_START + 1; i < ERRORS_END; i++) {
			if (is_error(static_cast<SOUL_STATUS>(i + 1))) {
				return true;
			}
		}
		return false;
	}
};
