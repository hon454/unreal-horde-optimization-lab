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
