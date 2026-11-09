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
#if !UE_BUILD_TEST
	/**
	 * 개발 전용 빠른 회전(-LabCamTurn, E3b (8) 화질 캡처 2026-11-09): P1과 같은 위치·높이·피치에서 5초마다 요 90도를 0.5초에 돌고 4.5초 멈춘다(60초에 12회, 한 방향).
	 * 측정 프리셋 P1/P2는 그대로다. 회전이 끝난 프레임에 lab_turn 한 줄(k, 시작·끝 프레임)을 남긴다. Test 빌드에는 없다.
	 */
	static constexpr float TurnPeriod = 5.f;
	static constexpr float TurnDuration = 0.5f;
	static constexpr float TurnStep = 90.f;
	bool bCamTurn = false;
	int32 TurnLogged = -1;      // lab_turn을 남긴 마지막 회전 번호
	uint64 TurnStartFrame = 0;  // 진행 중인 회전의 첫 프레임
	int32 TurnStarted = -1;     // TurnStartFrame을 적은 회전 번호
#endif
};
