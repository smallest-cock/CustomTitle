#include "pch.h"
#include "CustomTitle.h"


void CustomTitle::cmd_spawnCustomTitle(std::vector<std::string> args)
{
	GAME_THREAD_EXECUTE(
		Titles.spawnSelectedPreset();
	);
}

void CustomTitle::cmd_spawnItem(std::vector<std::string> args)
{
	if (args.size() < 2)
		return;

	int productID = std::stoi(args[1]);	// 7012 is cybertruck
	Items.SpawnProduct(productID);
}



// ##############################################################################################################
// #############################################    TESTING    ##################################################
// ##############################################################################################################

void CustomTitle::cmd_test(std::vector<std::string> args)
{
	Titles.test1();

	LOG("did test 1");
}

void CustomTitle::cmd_test2(std::vector<std::string> args)
{
	LOG("did test 2");
}

void CustomTitle::cmd_test3(std::vector<std::string> args)
{
	LOG("did test 3");
}
