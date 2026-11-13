#pragma once

#include "CoreMinimal.h"

/** 모듈 공통 로그 카테고리 */
DECLARE_LOG_CATEGORY_EXTERN(LogLab, Log, All);

/**
 * 설정·에셋 오류로 실행을 끝낼 때 부른다. 게임 모드의 다음 틱이 강제 종료 RequestExitWithStatus(true, 코드)를 낸다(ALabGameMode::Tick).
 * Windows의 비강제 종료는 PostQuitMessage(코드)뿐이고(WindowsPlatformMisc.cpp:1520) GuardedMain은 초기화 때의 ErrorLevel을 돌려준다(Launch.cpp:188-204).
 * 그래서 비강제 종료의 코드는 프로세스 종료 코드에 닿지 않는다(늘 0). 강제 종료는 로그를 비운 뒤 TerminateProcess(코드)다(WindowsPlatformMisc.cpp:1493-1515).
 * 처음 부른 코드가 남는다.
 */
void LabRequestExit(uint8 ExitCode, const TCHAR* CallSite);
/** 남은 종료 요청이 있으면 코드와 호출 위치를 돌려주고 지운다 */
bool LabConsumePendingExit(uint8& OutExitCode, const TCHAR*& OutCallSite);
