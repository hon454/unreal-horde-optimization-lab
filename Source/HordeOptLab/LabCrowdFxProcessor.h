#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "LabUnitAssets.h" // LabUnitTypeMax
#include "LabCrowdFxProcessor.generated.h"

/**
 * Q2b 엔티티 구동 배경 무리(Docs/Worklog/Q2.md "Q2b"). 그려지지 않는(표현 None) 걷는 미니언 엔티티의 위치를 종류별로 모아 Niagara 배열 사용자 변수로 넘긴다.
 * 배경 무리 입자가 실제 엔티티 자리에 그려져, 엔티티가 표현 LOD에 들어올 때(카메라 188m) 같은 자리에서 입자가 메시로 바뀐다.
 *
 * - 등록: 배경 무리를 켠 실행(사다리 단계 7부터, -LabCrowdFx=0이 끈다)에서만 게임 모듈이 켠다(HordeOptLab.cpp). 그 밖의 단계에는 프로세서가 없어 비용이 0이다.
 * - 자리: Representation 그룹, VisualizationProcessing 뒤(이번 프레임의 CurrentRepresentation을 읽는다). 엔진 ISKM 프로세서와 같은 자리다(MassUpdateSkinnedMeshProcessor.cpp:21-23).
 * - 게임 스레드: Niagara 컴포넌트에 배열을 넣으므로 게임 스레드에서 돈다(UNiagaraDataInterfaceArrayFunctionLibrary).
 * - 조건: FLabMinionTag 있음, FLabPooledTag 없음, CurrentRepresentation == None, 표현 상태 Walk. FMassOffLODTag로 거르지 않는다.
 *   카메라 188~200m는 그리지 않지만 Simulation LOD는 Medium이라 Off 태그가 없다(표현 LOD Off → Low 진입이 Lerp(80m, 200m, 0.9), engine-notes 17절. Simulation Off는 200m, LabMassTraits.cpp:55-68).
 * - 원소 FVector4f(X, Y, T0, 위상): 월드 X·Y(cm). T0는 그 위치에 있던 월드 시각이다. Off 태그 개체는 ULabOffMoveProcessor가 마지막으로 옮긴 시각(FLabOffMoveFragment::LastMoveTime,
 *   옮긴 적이 없으면 지금), 아니면 엔진 이동이 매 프레임 옮기므로 지금. 위상 = −StateStartTime(걷기는 LabWalkPhase(Serial), LabCrowdSubsystem.cpp).
 * - 피격·사망 개체는 넣지 않는다. 멈춰 있고(LabMassProcessors.cpp StopMoving, Off 이동 쿼리 밖) 입자는 걷기 클립과 진행 방향 정렬뿐이라 제자리 걷기로 보인다.
 *   그려지지 않는 거리의 피격·사망은 바깥 저격 구간(오브 80~200m) 끝에서만 생긴다. 개발 로그 lab_crowdrecs의 hitdead가 그 수다.
 */
UCLASS()
class ULabCrowdFxGatherProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	ULabCrowdFxGatherProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery EntityQuery;
	TArray<FVector4f> Recs[LabUnitTypeMax]; // 종류별 원소. 프레임마다 비우고 다시 채운다(할당은 유지)
#if !UE_BUILD_TEST
	// 개발 확인 로그(lab_crowdrecs, 5초마다): 프레임 평균 원소 수, 그려지지 않은 피격·사망 수, 그린 수, Off 태그 원소 수, 모으기 시간, 맵 가장자리 고리의 원소 수
	double DiagSince = 0.;
	int32 DiagFrames = 0;
	int64 DiagRecs[LabUnitTypeMax] = {};
	int64 DiagHitDead = 0;
	int64 DiagDrawn = 0;
	int64 DiagOff = 0;
	int64 DiagRing[2] = {}; // 오브 수평 거리 [480m, 490m), [490m, 500m]
	double DiagMs = 0.;
	double DiagMaxMs = 0.;
	double DiagFarMs = 0.; // 바깥 기록을 붙인 시간(M6 수정 A, Docs/Worklog/V2.md "M6 코드 비용 줄이기")
	double DiagSetMs = 0.; // Niagara 배열에 넣은 시간
	float DiagMaxLag = 0.f; // 원소의 지금 − T0 최대(초). Off 밴드 틱 간격(1초 ± 10%)을 넘지 않아야 한다
	float DiagMinCamM = TNumericLimits<float>::Max(); // 원소(그려지지 않은 걷는 엔티티) 중 카메라에 가장 가까운 3D 거리(m). 메시 렌더러 MinCameraDistance를 정하는 근거
	/**
	 * v2 W8 개발 진단(-LabOrbDiag, lab_orbdiag 5초마다, Docs/Worklog/V2.md "W8").
	 * 입자 오차: 그려지지 않은 걷는 엔티티(카메라 175~260m)의 입자 위치(에셋의 식, 넣은 User.LabOrbPos)와 엔티티가 지금 있어야 할 위치(같은 식, 오브의 지금 위치)의 거리와 방향 차이.
	 * 전환: 지난 프레임 입자였던 엔티티가 이번 프레임 그려질 때 메시 위치와 지난 입자 위치의 거리, 메시 방향과 입자 방향의 차이.
	 * 덮임: 화면 안 카메라 150~215m의 걷는 엔티티를 그린 것(액터·ISKM), 입자로 그린 것(175m 이상), 아무것도 그리지 않은 것(입자 MinCameraDistance 175m 안)으로 나눈다.
	 */
	struct FLabFxPrev { FVector2D Pos; FVector2D Dir; };
	TMap<FMassEntityHandle, FLabFxPrev> DiagPrev;
	double DiagPosErrMax = 0., DiagPosErrSum = 0., DiagDirErrMax = 0., DiagDirErrSum = 0.;
	int64 DiagErrCount = 0;
	double DiagJumpMax = 0., DiagJumpSum = 0., DiagTurnMax = 0., DiagTurnSum = 0.;
	int64 DiagJumpCount = 0;
	int64 DiagCoverDrawn = 0, DiagCoverFx = 0, DiagCoverHidden = 0;
	int32 DiagCoverHiddenMax = 0;
	int64 DiagHitDeadView = 0; // 화면 안의 그려지지 않는 피격·사망 개체(프레임 합)
#endif
};
