/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	SteamVrTrackComponent
//
//	MIT License
//	
//	Copyright (c) 2017 Brian Marshall, Cooperative Innovations LTd.
//	
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//	
//	The above copyright notice and this permission notice shall be included in all
//	copies or substantial portions of the Software.
//	
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//	SOFTWARE.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "SteamVrTrackComponent.h"
#include "Engine.h"
#include "PrimitiveSceneInfo.h"
#include "SteamVRPrivate.h"
#include "SteamVRHMD.h"

namespace
{
	FCriticalSection	LateUpdateCriticalSection;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	USteamVrTrackComponent - Constructor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

USteamVrTrackComponent::USteamVrTrackComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.bTickEvenWhenPaused = true;

	bDisableLateUpdates = false;
	DeviceId = 0;
	bTracked = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	USteamVrTrackComponent - Destructor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

USteamVrTrackComponent::~USteamVrTrackComponent()
{
	if(ViewExtension.IsValid())
	{
		{
			FScopeLock		ScopeLock(&LateUpdateCriticalSection);
			ViewExtension->SteamVrTrackComponent = nullptr;
		}

		if(GEngine)
		{
			GEngine->ViewExtensions.Remove(ViewExtension);
		}
	}
	ViewExtension.Reset();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	TickComponent
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void USteamVrTrackComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FVector		Position;
	FRotator	Orientation;

	//	View extension is only for late updates on local client.
	if(
		!ViewExtension.IsValid() &&
		GEngine
	)
	{
		TSharedPtr< FViewExtension, ESPMode::ThreadSafe > NewViewExtension(new FViewExtension(this));
		ViewExtension = NewViewExtension;
		GEngine->ViewExtensions.Add(ViewExtension);
	}

	bTracked = ReadTrackerState(Position, Orientation);
	if(bTracked)
	{

		//	Tracked, so update where the motion controller is.
		SetRelativeLocationAndRotation(Position, Orientation);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	ReadTrackerState
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool USteamVrTrackComponent::ReadTrackerState(FVector& Position, FRotator& Orientation)
{
	bool		Okay = false;

	//	Blueprint only supports int32 not uint32 but SteamVR id's are uint32. Don't try with negatives, just for safety.
	if(DeviceId < 0)
	{
		return	false;
	}
	Position = FVector::ZeroVector;
	Orientation = FRotator::ZeroRotator;

	Okay = USteamVRFunctionLibrary::GetTrackedDevicePositionAndOrientation((uint32)DeviceId, Position, Orientation);

	return	Okay;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	BeginRenderViewFamily
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void USteamVrTrackComponent::FViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	if(!SteamVrTrackComponent)
	{
		return;
	}

	if(SteamVrTrackComponent->bDisableLateUpdates)
	{
		return;
	}	

	FScopeLock	ScopeLock(&LateUpdateCriticalSection);

	LateUpdatePrimitives.Reset();

	GatherLateUpdatePrimitives(SteamVrTrackComponent, LateUpdatePrimitives);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	PreRenderViewFamily_RenderThread
//
//	Late update on the render thread.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void USteamVrTrackComponent::FViewExtension::PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily)
{
	if(SteamVrTrackComponent == nullptr)
	{
		return;
	}

	if(SteamVrTrackComponent->bDisableLateUpdates)
	{
		return;
	}

	FScopeLock	ScopeLock(&LateUpdateCriticalSection);

	FVector		Position;
	FRotator	Orientation;

	if(!SteamVrTrackComponent->ReadTrackerState(Position, Orientation))
	{
		return;
	}

	if(LateUpdatePrimitives.Num())
	{

		//	Calculate the late update transform that will rebase all children proxies within the frame of reference
		FTransform	OldLocalToWorldTransform = SteamVrTrackComponent->CalcNewComponentToWorld(SteamVrTrackComponent->GetRelativeTransform());
		FTransform	NewLocalToWorldTransform = SteamVrTrackComponent->CalcNewComponentToWorld(FTransform(Orientation, Position));
		FMatrix		LateUpdateTransform = (OldLocalToWorldTransform.Inverse() * NewLocalToWorldTransform).ToMatrixWithScale();
		
		FPrimitiveSceneInfo* RetrievedSceneInfo;
		FPrimitiveSceneInfo* CachedSceneInfo;
		
		//	Apply delta to the affected scene proxies
		int32		i;
		const int32	NumLateUpdatePrimitives = LateUpdatePrimitives.Num();

		for(i=0;i<NumLateUpdatePrimitives;i++)
		{
			LateUpdatePrimitiveInfo		PrimitiveInfo = LateUpdatePrimitives[i];

			RetrievedSceneInfo = InViewFamily.Scene->GetPrimitiveSceneInfo(*PrimitiveInfo.IndexAddress);
			CachedSceneInfo = PrimitiveInfo.SceneInfo;

			//	If the retrieved scene info is different than our cached scene info then the primitive was removed from the scene
			if(
				(CachedSceneInfo == RetrievedSceneInfo)	&&
				CachedSceneInfo->Proxy
			)
			{
				CachedSceneInfo->Proxy->ApplyLateUpdateTransform(LateUpdateTransform);
			}
		}
		LateUpdatePrimitives.Reset();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	GatherLateUpdatePrimitives
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void USteamVrTrackComponent::FViewExtension::GatherLateUpdatePrimitives(USceneComponent* Component, LateUpdateArrayType& LateUpdateList)
{
	int32	ChildIndex;

	//	If a scene proxy is present, cache it
	UPrimitiveComponent*	PrimitiveComponent = dynamic_cast<UPrimitiveComponent*>(Component);
	if(
		PrimitiveComponent				&&
		PrimitiveComponent->SceneProxy
	)
	{
		FPrimitiveSceneInfo*	PrimitiveSceneInfo = PrimitiveComponent->SceneProxy->GetPrimitiveSceneInfo();
		if(PrimitiveSceneInfo)
		{
			LateUpdatePrimitiveInfo		PrimitiveInfo;

			PrimitiveInfo.IndexAddress = PrimitiveSceneInfo->GetIndexAddress();
			PrimitiveInfo.SceneInfo = PrimitiveSceneInfo;
			LateUpdateList.Add(PrimitiveInfo);
		}
	}

	const int32		ChildCount = Component->GetNumChildrenComponents();
	for(ChildIndex=0;ChildIndex<ChildCount;ChildIndex++)
	{
		USceneComponent*	ChildComponent = Component->GetChildComponent(ChildIndex);
		if(!ChildComponent)
		{
			continue;
		}

		GatherLateUpdatePrimitives(ChildComponent, LateUpdateList);
	}
}

