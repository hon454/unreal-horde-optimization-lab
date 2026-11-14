#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LabHud.generated.h"

/**
 * 상태 오버레이. 평소에는 숨기고 F1이나 -LabHud로 켠다. Q5부터 -LabBench와 함께 줘도 켜진다(촬영 실행 = 측정과 같은 조건 + HUD, plan.md "HUD와 촬영").
 * 측정 실행은 -LabHud를 주지 않고, 켜졌는지는 CSV 메타데이터 LabHud로 남는다.
 * 왼쪽 위 반투명 판 하나에 GIF 독자용으로 사다리 단계(번호, 이름, 앞 단계에서 바꾼 것. 빼 보기는 "Final minus: ..."), 프레임 시간(1초 평균)과 목표 16.7ms,
 * Game·Render·GPU 시간(1초 평균), 호드 총수와 표현별 개체 수(액터·인스턴스·입자·그리지 않음)와 카메라 거리 구간, 최근 5초 프레임 시간 그래프(16.7ms 선),
 * 맨 아래 작은 줄에 내부 변형 이름·단계·프리셋·빌드를 그린다(Docs/Worklog/Q5-hud.md). 캔버스 타일과 글자만 쓰므로 Test 패키지에서도 그려진다(디버그 드로우와 stat 시스템을 쓰지 않는다,
 * engine-notes.md 20절). 글꼴은 엔진 Roboto를 화면 높이에 맞는 포인트 크기로 래스터화해 QHD와 720p 모두 선명하다.
 */
UCLASS()
class ALabHud : public AHUD
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

	void Toggle();
	bool IsShown() const { return bVisible; }

protected:
	/**
	 * 시험 실행 Spike1(-LabMotionAids=1, Docs/Worklog/V2.md "시험 실행"): 화면 오른쪽 위에 고정한 작은 판(왼쪽 위 상태 오버레이와 같은 여백·배율·바탕). 넓은 평지에서 카메라가 오브를 따라가면
	 * 오브가 움직이는 것이 화면에 보이지 않아서 오브의 월드 위치(m)와 속도(m/s), 월드 기준 진행 방향 화살표(위 +X, 오른쪽 +Y)를 적는다.
	 * 값은 인자가 아니라 오브 액터의 실제 위치와 프레임 사이 이동에서 구한다(속도는 0.25초 창의 이동 거리 ÷ 시간). 상태 오버레이(bVisible)와 무관하게 그린다.
	 */
	void DrawOrbPanel();
	TWeakObjectPtr<class ALabOrb> OrbForPanel;
	FVector OrbPanelLast = FVector::ZeroVector;        // 지난 프레임의 오브 위치
	FVector OrbPanelWindowStart = FVector::ZeroVector; // 속도 창이 시작할 때의 위치
	double OrbPanelWindowTime = -1.;                   // 속도 창이 시작한 월드 시각
	double OrbPanelWindowPath = 0.;                    // 창 안에서 움직인 거리(cm)
	float OrbPanelSpeed = 0.f;                         // 표시하는 속도(cm/s)
	FVector OrbPanelDir = FVector::ZeroVector;         // 표시하는 진행 방향(월드, 단위 벡터)

	/** 이번 프레임 값을 1초 창과 그래프 기록에 더한다(보일 때만) */
	void Sample();

	bool bVisible = false;

	// 1초 창 평균(프레임·Game·Render·GPU ms). 창이 끝날 때마다 Avg*를 바꿔 숫자가 1초에 한 번만 바뀐다
	double WindowStart = 0.0;
	int32 WindowFrames = 0;
	double SumFrameMs = 0.0, SumGameMs = 0.0, SumRenderMs = 0.0, SumGpuMs = 0.0;
	float AvgFrameMs = 0.f, AvgGameMs = 0.f, AvgRenderMs = 0.f, AvgGpuMs = 0.f;
	bool bHasAverage = false;

	// 프레임 시간 기록(그래프, 최근 GraphSeconds초). 고리 버퍼
	struct FSample { double Time = 0.0; float Ms = 0.f; };
	static constexpr int32 HistoryCapacity = 4096;
	TArray<FSample> History;
	int32 HistoryNext = 0;
	int32 HistoryCount = 0;
};
