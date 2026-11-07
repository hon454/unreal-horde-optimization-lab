#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "LabCameraRig.generated.h"

/**
 * 고정 카메라 프리셋. P1 오브 시점(대표, 오브 뒤에서 회전), P2 탑뷰(원거리 밴드 대조군). P3(오브 지상)는 P1과 역할이 겹쳐 뺐다(ADR-0003).
 * 프리셋마다 밴드 분포가 달라서 역할을 정해 동결한다(ADR 이전 결정 "카메라").
 */
UCLASS()
class ALabCameraRig : public ACameraActor
{
	GENERATED_BODY()
public:
	ALabCameraRig();

	void SetPreset(int32 InPreset, const FVector& InTarget);
	virtual void Tick(float DeltaSeconds) override;

protected:
	void Apply();

	int32 Preset = 1;
	FVector Target = FVector::ZeroVector;
	float Elapsed = 0.f;
};
