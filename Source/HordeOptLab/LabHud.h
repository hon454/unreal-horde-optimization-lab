#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LabHud.generated.h"

/**
 * 상태 오버레이. 평소에는 숨기고 F1이나 -LabHud로 켠다. 벤치마크 실행(-LabBench)에서는 항상 꺼서 수치에 섞이지 않게 한다.
 * 시나리오(단계·변형·N·프리셋·시드), 프레임/Game/Render/GPU ms, 물리 메모리, 군중 상태를 왼쪽 위에 글자로 그린다.
 * 캔버스 글자만 쓰므로 Test 패키지에서도 그려진다(디버그 드로우는 Test에서 컴파일되지 않는다).
 */
UCLASS()
class ALabHud : public AHUD
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

	void Toggle() { bVisible = !bVisible; }

protected:
	bool bVisible = false;
	float SmoothedFrameMs = 0.f;
};
