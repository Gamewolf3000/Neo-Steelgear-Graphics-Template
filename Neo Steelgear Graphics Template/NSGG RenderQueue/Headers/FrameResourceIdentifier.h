#pragma once

#include "ResourceIdentifiers.h"
#include "CategoryIdentifiers.h"

enum class FrameResourceOrigin
{
	TRANSIENT,
	CATEGORY
};

struct FrameResourceIdentifier
{
	FrameResourceOrigin origin;

	union IdentifierData
	{
		TransientResourceIndex transient;
		CategoryIdentifier category;

		IdentifierData(const TransientResourceIndex& transientIndex) : 
			transient(transientIndex)
		{
			// EMPTY
		}

		IdentifierData(const CategoryIdentifier& categoryIdentifier) :
			category(categoryIdentifier)
		{
			// EMPTY
		}

	} identifier;

	FrameResourceIdentifier(const TransientResourceIndex& transientIndex) :
		origin(FrameResourceOrigin::TRANSIENT), identifier(transientIndex)
	{
		// EMPTY
	}

	FrameResourceIdentifier(const CategoryIdentifier& categoryIdentifier) :
		origin(FrameResourceOrigin::CATEGORY), identifier(categoryIdentifier)
	{
		// EMPTY
	}
};