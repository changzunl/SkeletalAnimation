#include "GameCommon.hpp"

#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

RandomNumberGenerator rng;

const char* GetNameFromType(BillboardType type)
{
	static const char* const names[3] = { "none", "facing", "aligned" };
	return names[(unsigned int)type];
}

BillboardType GetTypeByName(const char* name, BillboardType defaultType)
{
	static const BillboardType types[3] = { BillboardType::NONE, BillboardType::FACING, BillboardType::ALIGNED };
	for (BillboardType type : types)
	{
		if (_stricmp(GetNameFromType(type), name) == 0)
			return type;
	}
	return defaultType;
}

