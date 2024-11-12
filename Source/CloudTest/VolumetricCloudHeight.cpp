// Fill out your copyright notice in the Description page of Project Settings.


#include "VolumetricCloudHeight.h"

#include "HistoryManager.h"
#include "Kismet//GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h" // MID
#include "Engine/TextureRenderTarget2D.h" // 렌더 타겟
#include "Kismet/KismetRenderingLibrary.h" // 렌더 타겟을 다루는 함수들


AVolumetricCloudHeight::AVolumetricCloudHeight(const class FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer), CurrentCloudMap(0), CurrentHeightMap(0)
{
	PrimaryActorTick.bCanEverTick = true;
	
	// CloudMaterial 에셋 불러오기
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VolumetricCloudMaterialRef(TEXT("/Game/Cloud/m_VolumetricCloudHeight.m_VolumetricCloudHeight"));
	CloudMaterial = VolumetricCloudMaterialRef.Object;

	// ApplyHeightMap 에셋 불러오기
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ApplyHeightMapRef(TEXT("/Game/Cloud/MAT_ApplyHeightMap.MAT_ApplyHeightMap"));
	MAT_ApplyHeightMap = ApplyHeightMapRef.Object;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DrawCloudMapMaterialRef(TEXT("/Game/Cloud/MAT_DrawCloudMap.MAT_DrawCloudMap"));
	MAT_DrawCloudMap = DrawCloudMapMaterialRef.Object;
	
	CloudHeightInfos.SetNum(32);
}


void AVolumetricCloudHeight::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}


void AVolumetricCloudHeight::BeginPlay()
{
	Super::BeginPlay();

	PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	CameraLocationStored = PlayerCameraManager->GetCameraLocation();

	InitCloud();
}


/*
 * VolumetricCloudComponent 가 private 로 선언되어 c++ 코드에선 MID 를 만들어 VolumetricCloudComponent 에 할당할 수 없고 블루프린트에서만 가능하다.
 * 블루프린드의 construction script 에서 Cloud Material 을 통해 Cloud MID 를 만들고
 * Cloud MID 을 컴포넌트에 대해 SetMaterial 한 후에 InitCloud 가 실행되어야 한다.
*/
void AVolumetricCloudHeight::InitCloud()
{
	for (int i = 0; i < 2; i++)
	{
		// 크기 512 * 512
		// 포맷
		RT_CloudMap[i] = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), 512, 512, ETextureRenderTargetFormat::RTF_RGBA8, FLinearColor::Black, false, false);

		// 크기 32 * 128
		// 포맷 R8
		RT_RuntimeHeightMap[i] = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), 32, 128, ETextureRenderTargetFormat::RTF_R8, FLinearColor::Black, false, false);
	}

	// 클라우드 맵 그리기
	DrawCloudMap();
	
	for (int i = 0; i < 32; i++)
	{
		ApplyHeightMap(i);
	}
}


void AVolumetricCloudHeight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	/*
	카메라의 이동에 따라 클라우드 맵 렌더 타겟의 UV 를 변경하기
	*/

	MoveOffsetStored.X += (PlayerCameraManager->GetCameraLocation().X - CameraLocationStored.X) / 3200000.0f;
	MoveOffsetStored.Y += (PlayerCameraManager->GetCameraLocation().Y - CameraLocationStored.Y) / 3200000.0f;
	
	CloudMID->SetVectorParameterValue(FName("MoveOffset"), MoveOffsetStored);
	
	CameraLocationStored = PlayerCameraManager->GetCameraLocation();

	// offset 이 일정 절대값 이상으로 커져버리면
	// if offset >= 0.5?
	// offset 을 다시 0.0 으로 바꾸고 렌더 타겟을 리프레쉬 해야 한다.

	// UE_LOG(LogTemp, Warning, TEXT("Cloud Offset X :: %f, Y :: %f"), MoveOffsetStored.X, MoveOffsetStored.Y);


	// ApplyCloudMap();
}

void AVolumetricCloudHeight::ApplyCloudMap()
{
	// 클라우드 맵 적용.
	CloudMID->SetTextureParameterValue(FName("CloudMap"), GetCurrentCloudMapRT());

	// UE_LOG(LogTemp, Warning, TEXT("Apply Cloud Map %d"), CurrentCloudMap);

	ChangeCloudMapRT();
}

void AVolumetricCloudHeight::DrawCloudMap()
{
	UMaterialInstanceDynamic* DrawCloudMap_MID = UMaterialInstanceDynamic::Create(MAT_DrawCloudMap, this);

	UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(), GetCurrentCloudMapRT(), DrawCloudMap_MID);
	
	ApplyCloudMap();
}


// CloudHeightInfos 의 내용을 렌더 타겟에 적용한다.
void AVolumetricCloudHeight::ApplyHeightMap(int index)
{
	UMaterialInstanceDynamic* ApplyHeightMap_MID = UMaterialInstanceDynamic::Create(MAT_ApplyHeightMap, this);

	// 복사될 스트립의 RGB 채널이 무엇인지
	ApplyHeightMap_MID->SetScalarParameterValue(FName("FromRGB"), CloudHeightInfos[index].RGB);

	// 몇번째 스트립을 복사할지
	ApplyHeightMap_MID->SetScalarParameterValue(FName("FromStripNum"), CloudHeightInfos[index].Strip);

	// 하이트맵 에셋의 스트립의 개수가 몇개인지. 레데리는 32개이다.
	ApplyHeightMap_MID->SetScalarParameterValue(FName("HeightMapNum"), 4);
		
	// 런타임 하이트맵의 스트립 개수가 몇개인지, 32개이다.
	ApplyHeightMap_MID->SetScalarParameterValue(FName("RuntimeMapNum"), 32);

	// 이전에 렌더타겟으로 쓰인 하이트맵을 파라미터로 설정
	ApplyHeightMap_MID->SetTextureParameterValue(FName("RuntimeTexture"), GetPrevHeightMapRT());

	// 런타임 스트립의 몇번째 스트립에 적용될지.
	ApplyHeightMap_MID->SetScalarParameterValue(FName("ToStripNum"), index + 1);

	UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(), GetCurrentHeightMapRT(), ApplyHeightMap_MID);
	
	CloudMID->SetTextureParameterValue(FName("HeightMap"), GetCurrentHeightMapRT());

	// UE_LOG(LogTemp, Warning, TEXT("Apply Height Map %d"), CurrentHeightMap);
	
	ChangeHeightMapRT();
}

// CloudHeightInfos 을 수정하고 ApplyHeightMap 를 실행한다.
void AVolumetricCloudHeight::SetHeightMap(int index, uint8 RGBVal, uint8 StripVal)
{
	CloudHeightInfos[index].RGB = RGBVal;
	CloudHeightInfos[index].Strip = StripVal;

	ApplyHeightMap(index);
}


UTextureRenderTarget2D* AVolumetricCloudHeight::GetCurrentCloudMapRT()
{
	return RT_CloudMap[CurrentCloudMap];
}
	
UTextureRenderTarget2D* AVolumetricCloudHeight::GetPrevCloudMapRT()
{
	if (CurrentCloudMap == 1)
	{
		return RT_CloudMap[0];
	}
	return RT_CloudMap[1];
}

void AVolumetricCloudHeight::ChangeCloudMapRT()
{
	if (CurrentCloudMap == 1)
	{
		CurrentCloudMap = 0;
	}
	else
	{
		CurrentCloudMap = 1;
	}
}

UTextureRenderTarget2D* AVolumetricCloudHeight::GetCurrentHeightMapRT()
{
	return RT_RuntimeHeightMap[CurrentHeightMap];
}
	
UTextureRenderTarget2D* AVolumetricCloudHeight::GetPrevHeightMapRT()
{
	if (CurrentHeightMap == 1)
	{
		return RT_RuntimeHeightMap[0];
	}
	return RT_RuntimeHeightMap[1];
}

void AVolumetricCloudHeight::ChangeHeightMapRT()
{
	if (CurrentHeightMap == 1)
	{
		CurrentHeightMap = 0;
	}
	else
	{
		CurrentHeightMap = 1;
	}
}
