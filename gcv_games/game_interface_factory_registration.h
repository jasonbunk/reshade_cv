#pragma once
/*
 * Modified from original source: https://github.com/Derydoca/factory-auto-registration
 * Copyright (C) 2019 Derrick Canfield
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "game_interface_factory.h"

namespace GameInterfaceFactoryRegistrations {

	template <typename T>
	class GameInterfaceFactoryRegistration
	{
	public:
		GameInterfaceFactoryRegistration(const char *id)
		{
			GameInterfaceFactory::get().registerGenerator(
				id,
				[]() { return static_cast<GameInterface *>(new T()); }
			);
		}
	};

}

#define REGISTER_GAME_INTERFACE(gicls, gictr, exename) \
  namespace GameInterfaceFactoryRegistrations { GameInterfaceFactoryRegistration<gicls> _##gicls##gictr(exename); }
