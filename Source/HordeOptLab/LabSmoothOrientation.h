#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "LabSmoothOrientation.generated.h"

/**
 * 방향 보간의 병렬 사본(-LabSmoothOrient=1, 기본 0, Docs/Worklog/V2.md "방향 보간 병렬 사본").
 * 엔진 UMassSmoothOrientationProcessor(MassSmoothOrientationProcessors.cpp:20-135)와 같은 쿼리, 그룹(Movement, 앞뒤 선언 없음), 넷 모드, 식이다.
 * 다른 점은 두 쿼리를 ParallelForEachEntityChunk로 돈다는 것 하나다. 개체마다 자기 프래그먼트(회전, DesiredFacing)만 쓰고 공유 상태가 없어 결과는 같다.
 * 엔진 프로세서는 한 스레드(ForEachEntityChunk)라 Test 1만 단계 8에서 작업자 하나가 약 0.58ms를 쓰고, 그동안 위치를 읽는 표현 LOD(게임 스레드)가 기다렸다
 * (Docs/Worklog/V2.md "분리 프로세서 기다림", engine-notes.md 48절).
 * 게임 모듈이 FLabConfig::bSmoothOrient일 때만 엔진 프로세서를 끄고 이것을 켠다(HordeOptLab.cpp). 끈 실행은 프로세서 구성이 그대로다.
 * 개발 빌드 진단: -LabSmoothOrientCheck는 엔진 프로세서를 그대로 두고 이것을 엔진 프로세서 앞에서 "예측"으로 돌린다(프래그먼트를 쓰지 않는다).
 * ULabSmoothOrientationCheckProcessor가 엔진 프로세서 뒤에서 엔진이 쓴 값과 비트 단위로 견준다. -LabSmoothOrientSerial은 사본을 한 스레드로 돌린다(시간 견주기).
 */
UCLASS()
class ULabSmoothOrientationProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	ULabSmoothOrientationProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery HighResEntityQuery;
	FMassEntityQuery LowResEntityQuery_Conditional;
	bool bPredict = false; // 개발 검사 모드(-LabSmoothOrientCheck). 생성자에서 정한다
#if !UE_BUILD_TEST
	double LogSince = 0.;
	int32 LogFrames = 0;
	uint64 LogExecCycles = 0, LogWaitCycles = 0;
	double LogExecMaxMs = 0.;
	int64 LogHiChunks = 0, LogHiEnts = 0, LogLoEnts = 0;
#endif
};

/** 개발 검사(-LabSmoothOrientCheck): 엔진 UMassSmoothOrientationProcessor 뒤에서 그 결과를 사본의 예측과 견준다. Test 빌드에서는 등록하지 않는다. */
UCLASS()
class ULabSmoothOrientationCheckProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	ULabSmoothOrientationCheckProcessor();
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	FMassEntityQuery HighResEntityQuery;
	FMassEntityQuery LowResEntityQuery;
#if !UE_BUILD_TEST
	double LogSince = 0.;
	int32 LogFrames = 0;
	int64 TotFrames = 0;
	int64 TotHiCompared = 0, TotHiMismatch = 0, TotLoCompared = 0, TotLoMismatch = 0, TotChanged = 0, TotMissing = 0, TotChangedVel = 0, TotChangedTarget = 0;
	int32 Reported = 0;
	int32 ReportedChanged = 0;
#endif
};
