#include "pch.h"
#include "Items.hpp"
#include "util/Instances.hpp"
#include "ModUtils/util/Utils.hpp"

UOnlineProduct_TA *ItemsComponent::SpawnProduct(int item,
    TArray<FOnlineProductAttribute>                 attributes,
    int                                             seriesid,
    int                                             tradehold,
    bool                                            log,
    const std::string                              &spawnMessage,
    bool                                            animation) {
	FOnlineProductData productData;
	productData.ProductID      = item;
	productData.SeriesID       = seriesid;
	productData.InstanceID     = GeneratePIID(item);
	productData.TradeHold      = tradehold;
	productData.AddedTimestamp = GetTimestampLong();
	productData.Attributes     = attributes;

	UOnlineProduct_TA *product = UProductUtil_TA::CreateOnlineProduct(productData);
	if (!product)
		return nullptr;

	if (SpawnProductData(product->InstanceOnlineProductData(), spawnMessage, animation))
		return product;

	return nullptr;
}

bool ItemsComponent::SpawnProductData(const FOnlineProductData &productData, const std::string &spawnMessage, bool animation) {
	UGFxData_ContainerDrops_TA *containerDrops = Instances.getInstanceOf<UGFxData_ContainerDrops_TA>();
	if (!containerDrops) {
		LOG("UGFxData_ContainerDrops_TA is null!");
		return false;
	}

	UOnlineProduct_TA *onlineProduct = containerDrops->CreateTempOnlineProduct(productData);
	if (!onlineProduct) {
		LOG("UOnlineProduct_TA is null!");
		return false;
	}

	// to prevent cached title spawn text (where the "Title: ..." spawn animation text doesnt match the title being spawned)
	static int hashId = 0;
	if (hashId == INT32_MAX)
		hashId = INT32_MIN;
	onlineProduct->CachedHash.Id = hashId++;

	USaveData_TA *saveData = Instances.getSaveData();
	if (!saveData) {
		LOG("USaveData_TA is null!");
		return false;
	}

	FString messageFstr = (spawnMessage == "") ? L"" : FString::create(spawnMessage);

	saveData->eventGiveOnlineProduct(onlineProduct, messageFstr, 0.0f);
	if (saveData->OnlineProductSet) {
		saveData->OnlineProductSet->Add(onlineProduct);

		// auto ProductData = onlineProduct->InstanceOnlineProductData();
		// Events.spawnedProducts.push_back(ProductData);	// maybe uncommment and implement later to make spawned items "persistent"
	}

	if (animation) {
		saveData->GiveOnlineProductHelper(onlineProduct);
		saveData->OnNewOnlineProduct(onlineProduct, messageFstr);
		saveData->EventNewOnlineProduct(saveData, onlineProduct, messageFstr);
	}

	LOG("Successfully spawned product: {}", Format::EscapeBraces(onlineProduct->ToJson().ToString()));

	return true;
}

FProductInstanceID ItemsComponent::GeneratePIID(int64_t Product) {
	generatedPIIDs++;
	return IntToProductInstanceID(GetTimestampLong() * Product + generatedPIIDs);
}

FProductInstanceID ItemsComponent::IntToProductInstanceID(int64_t Value) {
	FProductInstanceID ID;
	ID.UpperBits = static_cast<uint64_t>(Value >> 32);
	ID.LowerBits = static_cast<uint64_t>(Value & 0xffffffff);
	return ID;
}

unsigned long long ItemsComponent::GetTimestampLong() {
	return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

class ItemsComponent Items{};
