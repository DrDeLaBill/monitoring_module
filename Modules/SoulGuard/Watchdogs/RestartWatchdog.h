/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


struct RestartWatchdog
{
public:
	// TODO: check IWDG or another reboot
	void check();

	static void reset_i2c_errata();

private:
	static constexpr char TAG[] = "RSTw";
	static bool flagsCleared;

};
