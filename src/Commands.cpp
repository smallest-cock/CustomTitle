﻿#include "pch.h"
#include "Cvars.hpp"
#include "CustomTitle.h"
#include "Components/Components/Titles.hpp"
#include "Components/Components/Items.hpp"


void CustomTitle::initCommands()
{
	registerCommand(Commands::spawnCustomTitle, [this](std::vector<std::string> args)
	{
		GAME_THREAD_EXECUTE(
			Titles.spawnSelectedPreset();
		);
	});
	
	registerCommand(Commands::spawnItem, [this](std::vector<std::string> args)
	{
		if (args.size() < 2)
			return;

		int productID = std::stoi(args[1]);	// 7012 is cybertruck
		Items.SpawnProduct(productID);
	});

	// testing
	registerCommand(Commands::test, [this](std::vector<std::string> args)
	{
		Titles.test1();

		LOG("did test 1");
	});
	
	registerCommand(Commands::test2, [this](std::vector<std::string> args)
	{
		LOG("did test 2");
	});
	
	registerCommand(Commands::test3, [this](std::vector<std::string> args)
	{
		LOG("did test 3");
	});
}
