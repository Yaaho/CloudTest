// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/VolumetricCloudComponent.h"
#include "VolumetricCloudHeight.generated.h"

USTRUCT(BlueprintType)
struct FCloudHeightInfo
{
	GENERATED_USTRUCT_BODY()

	FCloudHeightInfo() : RGB(0), Strip(1) {}

	// 0 - 2
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 RGB;

	// 1 - 32
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 Strip;
};
/**
 * 
 */
UCLASS(Blueprintable)
class CLOUDTEST_API AVolumetricCloudHeight : public AVolumetricCloud
{
	GENERATED_BODY()

protected:
	// 매 프레임마다 호출된다.
	virtual void Tick(float DeltaSeconds) override;
	
public:
	AVolumetricCloudHeight();

	AVolumetricCloudHeight(const class FObjectInitializer& ObjectInitializer);

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void InitCloud();
	
	
	void ApplyCloudMap();
	
	void DrawCloudMap();

	
	// CloudHeightInfos 의 내용을 렌더 타겟에 적용한다.
	void ApplyHeightMap(int index);

	// CloudHeightInfos 을 수정하고 ApplyHeightMap 를 실행한다.
	void SetHeightMap(int index, uint8 RGBVal, uint8 StripVal);

	

	
	UTextureRenderTarget2D* GetCurrentCloudMapRT();
	
	UTextureRenderTarget2D* GetPrevCloudMapRT();

	void ChangeCloudMapRT();

	UTextureRenderTarget2D* GetCurrentHeightMapRT();
	
	UTextureRenderTarget2D* GetPrevHeightMapRT();

	void ChangeHeightMapRT();
	
	// 플레이어 카메라 매니저
	// InitCloud 에서 할당됨
	UPROPERTY()
	APlayerCameraManager* PlayerCameraManager;

	// 생성자에서 에셋이 지정된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* CloudMaterial;

	// 위 에셋을 바탕으로 만들어지는 MID
	// 블루프린트에서 생성되고 InitCloud 에서 CloudComponent 의 머티리얼로 지정된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInstanceDynamic* CloudMID;
	
	// 하이트맵 에셋의 한 스트립을 런타임 하이트맵에 복사하는 머티리얼
	// 설정해야할 파라미터 목록:
	// 하이트맵 에셋의 strip 개수
	// 런타임 하이트맵의 strip 개수
	// 복사할 하이트맵 에셋의 strip 정보. 몇번째 스트립인지와 채널값
	// 런타임 하이트맵에서 strip 이 그려질 위치
	UPROPERTY()
	UMaterialInterface* MAT_ApplyHeightMap;

	// CloudMap 에 구름 형상을 그리는 머티리얼
	// 단순하게 만들었지만 더 복잡해져야 한다.
	UPROPERTY()
	UMaterialInterface* MAT_DrawCloudMap;
	
	
	// 클라우드 맵 렌더 타겟
	UPROPERTY()
	UTextureRenderTarget2D* RT_CloudMap[2];

	// 클라우드 런타임 하이트맵 렌더 타겟
	UPROPERTY()
	UTextureRenderTarget2D* RT_RuntimeHeightMap[2];

	int CurrentCloudMap;

	int CurrentHeightMap;

	// 에디터에도 수정할 수 있도록 Tarray 로 선언
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCloudHeightInfo> CloudHeightInfos;

	FVector CameraLocationStored;

	FVector MoveOffsetStored;
};
