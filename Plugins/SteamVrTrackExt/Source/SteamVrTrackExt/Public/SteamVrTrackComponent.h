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

#pragma once

#include "EngineMinimal.h"
#include "SceneViewExtension.h"

#include "SteamVrTrackComponent.generated.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	USteamVrTrackComponent
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = SteamVR)
class STEAMVRTRACKEXT_API USteamVrTrackComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()
public:
										~USteamVrTrackComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamVR")
	uint8								bDisableLateUpdates : 1;

	UPROPERTY(EditAnywhere, Transient, BlueprintReadOnly, Category = "SteamVR")
	uint8								bTracked : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamVR")
	int32								DeviceId;

	//	TickComponent - used to update the tracking (main thread)
	void								TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//	If true, the Position and Orientation args will contain the most recent tracking state
	bool								ReadTrackerState(FVector& Position, FRotator& Orientation);

	//	View extension object that can persist on the render thread without the SteamVrTrackComponent
	//	This handles late updates
	class FViewExtension : public ISceneViewExtension, public TSharedFromThis<FViewExtension, ESPMode::ThreadSafe>
	{
	public:
										FViewExtension(USteamVrTrackComponent* InMotionControllerComponent)
										{
											SteamVrTrackComponent = InMotionControllerComponent;
										}
		virtual 						~FViewExtension()
										{										
										}

		//	ISceneViewExtension interface
		virtual void					SetupViewFamily(FSceneViewFamily& InViewFamily) override
										{

										}
		virtual void					SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override
										{

										}
		virtual void					BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
		virtual void					PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override
										{

										}
		virtual void					PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;
		virtual int32					GetPriority() const override
										{
											return	-10;
										}
		//	End ISceneViewExtension interface

	private:
		friend class USteamVrTrackComponent;

		//	SteamVrTrackComponent component associated with this view extension
		USteamVrTrackComponent*			SteamVrTrackComponent;

		struct LateUpdatePrimitiveInfo
		{
			const int32*			IndexAddress;
			FPrimitiveSceneInfo*	SceneInfo;
		};

		//	Note: 8 was just picked as covering likely uses.
		typedef TArray<LateUpdatePrimitiveInfo, TInlineAllocator<8>>	LateUpdateArrayType;

		void							GatherLateUpdatePrimitives(USceneComponent* Component, LateUpdateArrayType& Primitives);
		LateUpdateArrayType				LateUpdatePrimitives;
	};

	TSharedPtr< FViewExtension, ESPMode::ThreadSafe >
											ViewExtension;
};