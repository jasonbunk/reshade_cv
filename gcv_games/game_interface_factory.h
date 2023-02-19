#pragma once
/*
 * Modified from original source: https://github.com/Derydoca/factory-auto-registration
 * Copyright (C) 2019 Derrick Canfield
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <string>
#include <unordered_map>

class GameInterface;

typedef GameInterface *(*gameinterfcInstanceGenerator)();

class GameInterfaceFactory
{
public:
	static GameInterfaceFactory &get();

	const char **listGameInterfaces(size_t &count);
	GameInterface *getGameInterface(const std::string &typeName);
	bool registerGenerator(const char *typeName, const gameinterfcInstanceGenerator &funcCreate);

private:
	GameInterfaceFactory();
	GameInterfaceFactory(const GameInterfaceFactory &);
	~GameInterfaceFactory();

	std::unordered_map<std::string, gameinterfcInstanceGenerator> m_generators;
};
