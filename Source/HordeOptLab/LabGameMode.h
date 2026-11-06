#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LabGameMode.generated.h"

class ALabCameraRig;
class ALabCrowdSpawner;
class ALabOrb;

/**
 * 맵(L_Bench, Scripts/create-bench-map.py)에는 바닥·조명·하늘만 있고, 오브·스포너·카메라는 이 게임 모드가 실행 시 만든다.
 * 플레이어 폰은 없고 카메라 프리셋으로만 본다.
 */
UCLASS()
class ALabGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ALabGameMode();

	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	void ApplyVariantCVars();

	UPROPERTY() TObjectPtr<ALabCrowdSpawner> Spawner;
	UPROPERTY() TObjectPtr<ALabOrb> Orb;
	UPROPERTY() TObjectPtr<ALabCameraRig> Camera;

	float Elapsed = 0.f;
	bool bQuitRequested = false;
	bool bShotTaken = false;
};
