#include "game_interface_factory.h"
/*
 * Modified from original source: https://github.com/Derydoca/factory-auto-registration
 * Copyright (C) 2019 Derrick Canfield
 * SPDX-License-Identifier: BSD-3-Clause
 */

GameInterfaceFactory::GameInterfaceFactory()
{
}

GameInterfaceFactory::~GameInterfaceFactory()
{
}

GameInterface *GameInterfaceFactory::getGameInterface(const std::string &typeName)
{
	auto it = m_generators.find(typeName);
	if (it != m_generators.end())
	{
		return it->second();
	}

	return nullptr;
}

bool GameInterfaceFactory::registerGenerator(const char *typeName, const gameinterfcInstanceGenerator &funcCreate)
{
	return m_generators.insert(std::make_pair(typeName, funcCreate)).second;
}

const char **GameInterfaceFactory::listGameInterfaces(int &count)
{
	count = m_generators.size();
	const char **arrayHead = new const char *[count];

	int i = 0;
	for (auto g : m_generators)
	{
		size_t bufferSize = g.first.length() + 1;
		char *generatorIdBuffer = new char[bufferSize];
		strncpy_s(generatorIdBuffer, bufferSize, g.first.c_str(), g.first.length());
		arrayHead[i++] = generatorIdBuffer;
	}

	return arrayHead;
}

GameInterfaceFactory &GameInterfaceFactory::get()
{
	static GameInterfaceFactory instance;
	return instance;
}
