#pragma once
#include "Component.hpp"

class ItemsComponent : Component<ItemsComponent>
{
public:
	ItemsComponent() {}
	~ItemsComponent() {}

	static constexpr std::string_view componentName = "Items";

private:
	int generatedPIIDs = 0;

private:
	FProductInstanceID GeneratePIID(int64_t Product = -1);
	FProductInstanceID IntToProductInstanceID(int64_t Value);
	unsigned long long GetTimestampLong();

public:
	UOnlineProduct_TA* SpawnProduct(int item,
	    TArray<FOnlineProductAttribute> attributes   = {},
	    int                             seriesid     = 0,
	    int                             tradehold    = 0,
	    bool                            log          = false,
	    const std::string&              spawnMessage = "",
	    bool                            animation    = true);

	bool SpawnProductData(const FOnlineProductData& productData, const std::string& spawnMessage, bool animation = true);
};

extern class ItemsComponent Items;