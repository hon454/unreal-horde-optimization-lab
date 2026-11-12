# 글 작업 기록

글 하나에 절 하나다. 단계(조사, 초안, 그림, 측정 기록, 확인, 커밋)가 끝날 때마다 한 줄을 더한다.

## 01 액터 군중 기준선(2026-11-12)

- 조사 끝(성공). `Measurements/Full1/summary.csv`·`runs.csv`의 `L1-full`·`S1-budget`·`S1-uro` 행, `full-n-at-60fps.csv`, Q6 트레이스 표를 다시 읽었다. 트레이스 내보내기 CSV(`%TEMP%\horde-trace\F1-traceser-L1-full-GT.csv`)에서 446.43 / 323.49ms를 다시 확인했다. 새로 확인한 엔진 사실: 1만에서는 월드 델타가 0.4초로 잘린다(`BaseGame.ini:199` `MaxUndilatedFrameTime=0.4`, `WorldSettings.cpp:334-343`). 그래서 이동 컴포넌트가 50ms씩 여덟 번, 곧 상한 8회까지 나눈다.
- 초안 끝(성공). `Posts/01-actor-baseline/README.md`. 틀은 요약, 시나리오 카드, "쓰인 것" 다섯 행, 원리(Mermaid 도식, 하위 스텝, 예상), 문제(트레이스 그림, Insights 자리, 개체당 Game 표), 결과(N@16.7ms 797, URO·Budget 표), 한 문장으로. URO가 Budget보다 덜 준 이유는 "확인하지 않았다"로 두었다(post-review의 미확인 설명을 쓰지 않음).
- 그림 끝(성공). `images/actor-gamethread-breakdown.svg`(446ms 구성, 16.7ms 막대를 같은 축척으로), `images/actor-frame-vs-n.svg`(프레임 p50·p95 로그 축 + 개체당 Game µs). 생성 스크립트는 저장소 밖 스크래치에 두었다(값을 손으로 넣은 정적 SVG). 코디네이터가 넣은 `insights-gamethread.jpg`를 보고 옆 표를 그 화면의 값(한 프레임 447ms, CharacterMovement 321.66ms, 10,048회)과 구간 평균으로 고쳤다.
- 측정 기록 끝(성공). `Posts/01-actor-baseline/measurements.md`: 빌드와 묶음, 본문 수치 표(세 회차 값), 계산식, 그림, 엔진 소스(0.4초 델타 상한 포함), 경위 근거(M0.md), 확인하지 않은 것.
- 확인 끝(성공). 상대 링크와 이미지 경로 13개가 모두 실제 파일을 가리킨다(`insights-gamethread.jpg`도 코디네이터가 넣어 있다). 본문 산문은 표·코드·이미지·제목을 빼고 2,446자(공백 포함). 본문 수치를 `summary.csv`·`runs.csv`·트레이스 내보내기 CSV와 다시 대조했다. 본문에 `Full1`, 일정·면접 표현 없음. 코드 스팬 밖 `~`는 `\~`.
- 커밋 끝(성공). 70b3075 `docs(posts): write post 01 on the actor crowd baseline`(이 줄은 다음 커밋에 들어간다).
- 출처 구분 끝(성공). 코디네이터 지시대로 스크린샷(병렬 `Full1trace-L1-full-N10000-P1-r1`, 렌더링 프레임 109: 447.88ms 중 321.66ms, 71.8%)은 캡션에, 옆 표는 직렬 트레이스 25\~75초 구간 평균으로 두고 제목에 밝혔다. 측정 기록에 "두 트레이스" 절을 더했다. 다시 찍을 필요는 없다고 판단했다(병렬 구간 평균 323.06 대 직렬 323.49ms, 1단계는 Mass가 거의 돌지 않는다).

### 확인하지 못한 것

- 하위 스텝 횟수(1만 8회)는 엔진 소스와 0.4초 델타 상한으로 계산했고, 실행에서 세지 않았다. 끄고 다시 잰 실험도 없다.
- Budget·URO 실행의 트레이스가 없어 Budget이 줄인 41.4ms의 내역과 URO가 덜 줄인 이유를 모른다.
- `S1-budget` r3의 GPU p50 112.456ms(다른 회차 약 50ms). 프레임은 같아 본문에 영향 없음.
- 0.4초 상한으로 1만 실행의 게임 시간이 느리게 흐르는데 사망 수가 2단계와 맞는 이유.
- Mermaid 도식과 SVG를 GitHub 렌더러에서 직접 보지는 않았다(SVG는 로컬 브라우저로 확인).

### 코디네이터용

- engine-notes.md에 더할 사실: `MaxUndilatedFrameTime=0.4`(`Engine/Config/BaseGame.ini:199`), `AWorldSettings::FixupDeltaSeconds`가 월드 델타를 자른다(`WorldSettings.cpp:334-343`). 11절 "CharacterMovement 하위 스텝" 행에 "프레임이 0.4초를 넘으면 델타가 0.4로 잘려 50ms × 8회, 곧 상한"을 덧붙이면 맞다. 이 작업은 engine-notes.md를 고치지 않았다.
- posting.md "차트는 Mermaid `xychart-beta`": 액터 단계는 9ms와 441ms가 한 차트에 있어 선형 축으로는 작은 값이 안 보인다. 로그 축 SVG로 그렸다. "로그 축이 필요하면 SVG"를 허용하면 좋겠다.
- "한 문장에 숫자 둘" 규칙은 비교 문장("2천에서 1만으로 개체 수는 5배인데 Game은 12.3배")에서 지키기 빡빡하다. 비교 한 쌍은 예외로 두거나 표로 넘기라고 적으면 좋겠다.
- "쓰인 것" 다섯 행에 Game p50을 따로 넣을 자리가 없어 첫 행에 묶었다. 측정 지표(프레임 p50·p95, Game, N@16.7ms)는 모든 글에 반복되므로, 공통 지표 카드를 00번 글에 두고 각 글은 기법만 카드에 넣는 쪽이 짧다.
- 기준선 글 틀에 "결과"의 "예상과 실제의 대조 표"는 맞지 않았다(바꾼 것이 없다). 예상 한 줄(166ms)은 원리 끝에 두고 실제 값은 문제 절의 표로 받았다.
- 분량: 산문 2,446자로 범위 안. 표가 여섯 개라 화면 길이는 길다. 요약 표와 결과 표가 URO·Budget 값을 두 번 싣는다. 시범 글을 보고 결과 표를 변화 열만 남길지 정하면 된다.
- Insights 옆 표는 스크린샷의 한 프레임 값과 구간 평균을 나란히 두었다. 스크린샷이 한 프레임만 보이므로 "구간 평균은 내보내기 CSV"라는 구분을 posting.md에 적어 두면 다음 글이 헷갈리지 않는다.
- `Docs/post-review.md`는 "글을 다 쓰면 지운다"라서 측정 기록에서 링크하지 않고 경로만 적었다. 다른 글도 같게 하면 지운 뒤 깨진 링크가 남지 않는다.

## 01 수정(사용자 검토 반영)(2026-11-12)

- 조사 끝(성공). 엔진 소스를 직접 열어 발췌 대상과 설명 근거를 확인했다: `GetSimulationTimeStep`(`CharacterMovementComponent.cpp:8162-8187`), `PhysWalking` 루프(`:5653`, while `:5686`, `MoveAlongFloor` `:5744`, `FindFloor` `:5773`), 기본값 `:698-699`, `FixupDeltaSeconds`(`WorldSettings.cpp:334-343`), URO 화면 크기 단계(`EngineTypes.h:2820-2826` 0.24·0.12, `SkinnedMeshComponent.cpp:284-367`), Budget 분배(`AnimationBudgetAllocator.cpp:327-342, 556-640`). `summary.csv`에서 렌더 스레드·GPU 중앙값을 읽었다. 새 도식이 `actor-gamethread-breakdown.svg`와 같은 네 값을 보이므로 둘을 합치기로 했다(네 칸의 순서와 비용을 한 그림에).
- 그림 끝(성공). `images/actor-frame-flow.svg`를 새로 그렸다(네 칸을 순서대로, 칸마다 하는 일을 18px·타이머 이름을 14px로, 오른쪽 막대 길이가 1만의 프레임당 시간, 맨 아래 16.7ms 막대를 같은 축척으로). 네 번째 칸 `UWorld_SendAllEndOfFrameUpdates`는 측정 기록에 값이 없어 따로 막대를 두지 않고 "나머지 81.0ms(이 칸과 그 밖의 일)"를 점선 막대로 보였다. 이 그림이 `actor-gamethread-breakdown.svg`와 같은 네 값(323.49 / 27.24 / 14.70 / 81.00)을 보이므로 그 파일을 지우고 하나로 합쳤다(주장 하나에 시각 자료 하나). `actor-frame-vs-n.svg`는 폭 960·글자 11\~13px이라 800px에서 14px에 못 미쳐 폭 800·글자 14px 이상으로 다시 그렸고, 범례의 `p50`·`p95`·`N@16.7ms`와 제목의 `P1`·`Game`을 풀어 썼다. 두 SVG 모두 자기 흰 배경(둥근 테두리)을 가진다. Edge 헤드리스로 흰 배경·GitHub 어두운 배경(#0d1117) 위에 렌더링해 눈으로 확인했다. 생성 스크립트는 저장소 밖 스크래치에 두었다.
- 작성 끝(성공). `README.md`: Mermaid를 지우고 `actor-frame-flow.svg`를 넣었다. "쓰인 것" 표를 "쓰인 기술" 절(CharacterMovement, URO, Animation Budget Allocator, 734자)로 바꿨다. 요약 표를 "군중을 만든 방법"과 프레임·게임 스레드·렌더 스레드·GPU 시간(ms)으로 고치고, 프레임 시간을 가장 느린 스레드가 정한다는 문장과 "자세한 설명은 00번 글"을 더했다. 결과 표는 변화 열만 남겼다(요약과 같은 값 되풀이 없음). 개체 수별 표는 `actor-frame-vs-n.svg`와 같은 값이라 지우고 산문과 그림으로 받았다. 스크린샷 옆 표는 스크린샷에서 읽은 값(448ms, 322ms, 10,048회)으로 바꿨다. 엔진 코드 발췌 세 블록(`GetSimulationTimeStep` 12줄, `PhysWalking` 루프 13줄, `FixupDeltaSeconds` 5줄, 합 30줄)을 넣고 산문의 하위 스텝 설명을 줄였다. HUD 문장은 측정 기록 "화면의 HUD와 표의 차이"로 옮겼다. `measurements.md`: 렌더 스레드·GPU 행, `S1-budget` r3 GPU 112.456ms 문단, 스크린샷 행, 발췌의 원본 줄 표, URO·Budget 소스 위치를 더했다. Budget 중요도의 기준점이 오브가 아니라 카메라 트랜스폼(`LabGameMode.cpp:595-601`)임을 확인해 "카메라와의 거리"로 고쳤다.
- 확인 끝(성공). 본문 산문(표·코드·이미지·제목·"쓰인 기술" 제외) 3,184자, "쓰인 기술" 734자. 본문 숫자를 모두 뽑아 측정 기록 표와 대조했고, 새 값(렌더 30.277 / 28.706 / 28.939, GPU 50.516 / 50.483 / 50.246, 1천 게임 스레드 16.624)은 `summary.csv`와 직접 대조했다. 상대 링크·이미지 5개와 측정 기록의 링크가 모두 실제 파일을 가리킨다. 본문에 `Full1`, `p50`, `p95`, `N@`, `P1`, "1만 오브 시점", "녹화" 없음. 코드 밖 `~`는 모두 `\~`. 80자를 넘는 문장 없음(코드 경로 줄 제외). 발췌는 소스를 열어 그대로 옮겼다.

### 코디네이터용(01 수정)

- engine-notes.md에 더할 사실이 늘었다: URO 기본 화면 크기 단계 0.24·0.12와 그려지지 않은 메시 4프레임(`EngineTypes.h:2820-2826`, `SkinnedMeshComponent.cpp:283-370`), Budget 분배 세 묶음(`AnimationBudgetAllocator.cpp:558-640`), 0.4초 델타 상한(앞 절). 이 작업은 engine-notes.md를 고치지 않았다.
- posting.md "기준선 글은 원리를 문제 앞에" + "주장 하나에 시각 자료 하나"를 따르면, 원리 도식에 비용 막대를 넣는 순간 문제 절의 구성 그림과 겹친다. 01은 둘을 합쳤다. 기준선 글에서는 "원리 도식이 비용을 함께 보이면 문제 절은 Insights 스크린샷만 둔다"를 적어 두면 다음 기준선 글(04 등)이 헷갈리지 않는다.
- 요약 표(프레임·게임·렌더·GPU 네 열)와 posting.md의 "요약: 수치 표 3\~4행"은 맞는다. 다만 posting.md "결과: 예상과 실제의 대조 표"는 기준선 글에 맞지 않아 이번에도 예상(166ms)과 실제(427ms)를 산문 한 문장으로 대조했다.
- 측정 기록(`measurements.md`)도 `Posts/` 안의 공개 파일인데 `Full1`·`N@16.7ms`·`P1`을 쓴다. 규칙은 본문(README)만 막는다. 실행 라벨에 `Full1`이 들어가므로 측정 기록은 그대로 두었고, `N@16.7ms`만 코드 스팬에 넣었다. 측정 기록에도 약어 규칙을 적용할지 정해 주면 좋겠다.
- 네 번째 칸 `UWorld_SendAllEndOfFrameUpdates`의 값을 트레이스 내보내기 CSV에서 읽으면 그림의 "나머지" 점선을 실제 막대로 바꿀 수 있다. 지시("측정 기록에 값이 있을 때만")대로 이번에는 읽지 않았다.
- 새 규칙 반영 끝(성공). 첫 커밋(b5bff0a) 사이에 posting.md가 바뀌어(c61a439, ad434c5, cdefef7, ADR-0014: 본문 표 금지, SVG 글자 16px 이상, "배경 지식", 수치 그림은 `summary.csv`에서 스크립트로) 다시 맞췄다. 요약 표 → `images/actor-summary.svg`(세 구성 × 프레임·게임·렌더·GPU 열두 값), 결과 표 → `images/actor-anim-techniques.svg`(줄어든 시간과 애니메이션 쪽 두 타이머의 합 비교), Insights 옆 표 → 스크린샷 아래 세 문장. `actor-frame-vs-n.svg` 아래 칸에 "예상: 1천의 값으로 일정" 점선을 더해 결과 절의 예상과 실제 대조를 그림으로 보였다. "쓰인 기술"을 "배경 지식"으로 바꿨다. 네 SVG를 `Posts/01-actor-baseline/make-figures.py`가 `summary.csv`·`full-n-at-60fps.csv`에서 만든다(트레이스 값 네 개는 상수). 글자는 모두 16px 이상(14px 이하 0개). 다시 렌더링해 밝은·어두운 배경에서 확인했다. 본문 산문 3,251자, "배경 지식" 710자, 본문 Markdown 표 0개, 링크·이미지 7개 모두 실제 파일.

### 코디네이터용(새 규칙 반영)

- cdefef7이 이 세션이 `git rm`으로 스테이징해 둔 `actor-gamethread-breakdown.svg` 삭제를 함께 커밋했다. 의도한 삭제라 결과는 맞지만, 한 체크아웃에서는 다른 세션이 스테이징한 것이 `git commit`에 섞인다. 코디네이터 커밋은 `git commit -- <경로>`로 경로를 한정하면 막을 수 있다.
- "수치 그림은 `summary.csv`에서 스크립트로 만든다"의 스크립트 위치가 정해져 있지 않다. `Scripts/`는 측정 세션 소유라 01은 글 폴더에 `make-figures.py`를 두었다. 글마다 둘지, `Scripts/plot.py`에 모을지 정하면 좋겠다.
- 트레이스 값(Insights 내보내기)은 `summary.csv`에 없어 스크립트에 상수로 넣었다. 트레이스 표를 CSV로 커밋하면(예: `Measurements/Full1/traces.csv`) 그림이 모두 파일에서 나온다.
- 결과 절의 "예상과 실제를 대조한 그림"은 기준선 글에서는 개체당 시간의 예상선(1천의 값으로 일정)으로 대신했다. 기준선 글의 결과 그림을 무엇으로 할지 posting.md에 한 줄 있으면 04가 헷갈리지 않는다.
- 커밋 끝(성공). b5bff0a(첫 수정), 87e1d9a(새 규칙 반영). 이 줄은 다음 커밋에 들어간다.
- 표 지침 요청 확인(성공, 바꿀 것 없음). 갈라진 세션이 c61a439의 표 지침 두 개(크기 비교는 SVG, Markdown 표는 다섯 열·칸 하나 한 줄)를 01에 반영해 달라고 했다. 87e1d9a가 이미 그 뒤의 cdefef7(ADR-0014, 본문 표 금지)까지 반영했다. 요약·결과의 두 표는 `actor-summary.svg`와 `actor-anim-techniques.svg`로 바꿨고 같은 값 되풀이도 없다. 타이머별 표와 호드 총수별 표는 ADR-0014에 따라 본문에서 지웠고, 값은 `actor-frame-flow.svg`·`actor-frame-vs-n.svg`와 측정 기록의 표에 있다. 그림 네 개의 값이 모두 `measurements.md` 표에 있음을 다시 확인했다. 측정 기록의 표는 모두 다섯 열 이하이고 칸에 줄바꿈이 없다.

## README 그림(2026-11-12)

- 조사 끝(성공). 그림의 값은 `Measurements/Full1/summary.csv`(1만, 오브 시점: `L1-full`\~`L8-q4final`, `A8-*` 열 개), `Docs/figures/full1/full-n-at-60fps.csv`(`n_p95`, `note_p95`가 "above range"면 3만 이상), `Measurements/Full1/transition-dev/runs.csv`(조건 넷 × 시드 셋, `mean_excess`, `noticeable/events`)에서 읽는다. 조건과 기법의 대응은 `Scripts/full-conditions.json`의 `note`로 확인했다(`A8-farcull` = 200m 밖을 입자 대신 ISKM, `A8-crowdfx` = 배경 무리 뺌, `A8-farshadow` = 80m 밖 그림자 켬). 그림 모양은 `Posts/01-actor-baseline/make-figures.py`를 따른다.
- 그림 끝(성공). `Scripts/plot-readme.py`가 `Docs/figures/readme/`에 `ladder.svg`, `max-entities.svg`, `ablation.svg`, `transition.svg`를 쓴다(표준 라이브러리만, 값은 모두 CSV에서). 사다리의 축척 차이는 위아래 두 칸으로 풀었다. 1단계는 축척 A(0\~450ms), 2\~8단계는 축척 B(0\~17ms, 약 26배)이고 칸마다 16.7ms 선이 있다. 잘린 막대는 1단계 안의 비율(게임 스레드 427 대 GPU 50.5)을 숨기므로 버렸다. 병목은 `summary.csv`의 `bottleneck` 열로 표시했다. 빼 보기는 measurement.md의 판정(중앙값 차이가 두 구성의 범위 중 큰 쪽보다 작으면 구별 안 됨)으로 흐린 막대를 그리고, 프레임은 구별되지 않고 게임 스레드만 갈리는 행에 "프레임은 그대로, 게임 스레드만"을 붙였다. 이 판정으로 액터 하이드레이션, 애니메이션 예산, Simulation LOD에 더해 고정 바운드(게임 스레드 +0.17ms, 범위 0.115ms)도 걸린다. 전환은 세 시드의 `mean_excess` 중앙값을 점, 최소\~최대를 막대로, 눈에 띈 전환은 시드별 `noticeable/events`의 최소\~최대(반올림)다. 다시 돌려도 같은 SVG가 나온다(md5 같음).
- README 끝(성공). "결과 한눈에 보기"의 사다리·빼 보기·전환 표 세 개와 `full-ladder-P1.svg` 삽입을 빼고 `Docs/figures/readme/`의 그림 네 개를 넣었다(파일 `full-ladder-P1.svg`는 그대로). 단계별 화면은 사다리 그림 아래 한 줄(단계 이름에 링크 여덟 개)로 이었다. 정확한 값은 `summary.csv`, `full-n-at-60fps.csv`, `full-ablation.csv`, `transition-dev/runs.csv` 링크로 이었다. 그림이 말하는 수치 문장(병목 목록, 빼 보기의 ms 나열)과 여유 기준 문장(13.3ms)은 지웠다. HUD 문장은 맨 위 두 화면을 읽는 데 필요해 한 문장으로 남겼다. 2단계 2만의 흔들림은 한 문장(세 번 잰 중앙값이 3.35ms 갈림)으로 남겼다. 맨 위 소개의 "Game Thread, Render Thread"를 "게임 스레드, 렌더 스레드"로, "기법 하나를 뺀" 표를 그림으로 고쳤다. README에 `L1`\~`L8`, `p50`, `p95`, `P1`, "Game" 없음.
- 확인 끝(성공). 그림의 값을 `summary.csv`·`full-n-at-60fps.csv`·`runs.csv`와 대조했다(사다리 32값과 병목 8개, 최대 개체 수 8개, 빼 보기 30값, 전환 중앙값·범위·비율 4행. 지난 README 표의 값과도 같다). SVG 글자는 모두 16px 이상(16·17·22px만). Edge 헤드리스로 흰 배경과 GitHub 어두운 배경(#0d1117) 위에 800px 폭으로 렌더링해 겹침과 잘림이 없음을 눈으로 확인했다(사다리 아래 칸의 16.7ms 글자가 오른쪽 끝을 넘던 것을 선 왼쪽으로 옮겼다). 확인용 PNG는 저장소 밖 `C:/Users/hon45/AppData/Local/Temp/claude/596429fce533a335/93149a3c-81a4-4f72-851d-95dcb714dbf7/scratchpad/png/`의 `{ladder,max-entities,ablation,transition}-{light,dark}.png`다. README의 Markdown 표는 맨 위 화면 표·"글"·"구조"뿐이고 수치 표는 없다. 상대 링크 22개가 모두 실제 파일을 가리킨다. 코드 밖 `~` 없음.
- 커밋 끝(성공). 305f43a(`Scripts/plot-readme.py`, `Docs/figures/readme/`, `README.md`, `Scripts/README.md`, 이 절). 푸시하지 않았다. 이 줄은 다음 커밋에 들어간다. 코디네이터용: 루트 README "구조" 표의 `Measurements/` 행에 묶음 이름 `Full1/`이 남아 있다(지시대로 건드리지 않았다).

## README 시나리오 그림(2026-11-12)

- 조사 끝(성공). 값은 `Source/HordeOptLab/LabConfig.h` `FLabScenario`(맵 반경 500m, 재생성 링 490\~500m, 피해 범위 8m, 초당 피해 100\~300, 체력 60\~240, 3m/s, 저격 2초 5명, P1 뒤 15m·높이 9m·피치 −12도·60초 한 바퀴, P2 높이 100m), `LabSnipe.h` `FLabSnipeBands`(30/80/200m 구간, 1/2/2명), `LabUnitAssets.h` `FLabUnitTypes::IsLarge`(일련번호 10의 배수가 대형), `LabCrowdSpawner.cpp` `SpawnInitial`(반지름을 [8m, R]에서 균등 = 밀도 ∝ 1/r), `LabGameMode.cpp`(`DefaultPawnClass = nullptr`, 뷰 타깃은 카메라 리그), `LabCameraRig.cpp` `Apply`에서 확인했다. measurement.md "확정값"과 같다.
- 그림 끝(성공). `Scripts/plot-readme.py`에 `scenario()`를 더해 `Docs/figures/readme/scenario.svg`를 만든다(기존 네 그림과 같은 글꼴·색·흰 카드, 스크립트·외부 글꼴 없음, 글자 16·17·22px만). 상수는 스크립트 위쪽에 모으고 줄마다 출처 심볼을 적었다. 위 칸은 맵 전체를 축척대로(반경 500m), 아래 칸은 오브 둘레 20m를 축척대로 그렸다. 8m 피해 범위와 15m 뒤 카메라는 500m 축척에서 몇 픽셀이라 확대 칸이 필요했다. 오브 시점 화면 쐐기는 가로 90°다. `ALabCameraRig`는 `ACameraActor`이고 FOV를 바꾸지 않으므로 엔진 기본값 `UCameraComponent::FieldOfView = 90`(`Engine/Source/Runtime/Engine/Private/Camera/CameraComponent.cpp:78`)이다. 저격도 이 FOV로 화면 안을 판정한다(`LabOrb.cpp:155`). 회전 화살표는 위에서 볼 때 시계 방향이다(UE는 왼손 좌표계라 요 증가가 위에서 시계 방향, `LabCameraRig.cpp` `Apply`의 `Yaw = 360 × Elapsed / 60`). 개체 점 700개는 반지름을 [8, 500]m에서 균등으로 뽑아(스포너와 같은 규칙) 밀도 ∝ 1/r을 보이고, 10번째마다 시즈 미니언으로 그렸다. 저격 선 다섯은 화면 쐐기 안에서 구간별 1·2·2개를 시드 순서로 골랐다(해시 최소의 대역). 다시 돌려도 SVG 다섯 개의 md5가 같다.
- 그림에 넣지 않은 것: 표현 밴드(액터 / ISKM / 입자, 단계마다 다름, 지시대로), 카메라 피치 −12도(위에서 본 그림에 드러나지 않음, 높이만 적음), 대형 저격 예약 발 규칙(90 / 10 비율을 지키는 장치라 독자에게는 비율로 충분), 사망 애니와 재활용(표현 쪽 동작), 조명. 체력 60\~240은 그림 범례에, 시드로 정한다는 것과 장치의 목적은 README 글에 남겼다.
- README 끝(성공). "시나리오" 절을 그림(alt 텍스트에 맵·개체·피해·저격·재생성·카메라 요약)과 두 문장(장치의 목적, 에셋과 고정 시드가 정하는 것)으로 바꿨다. 다른 절은 건드리지 않았다(작업 중 다른 세션이 4391b31로 README의 다른 절을 고쳤다).
- 확인 끝(성공). Edge 헤드리스로 흰 배경과 GitHub 어두운 배경(#0d1117) 위에 800px 폭으로 렌더링해 겹침·잘림이 없음을 눈으로 확인했다(저격 범례 줄이 오른쪽 여백을 넘던 것을 두 줄로 나눴고, "맵 전체" 제목을 부제에서 띄웠다). 확인용 PNG는 저장소 밖 `C:/Users/hon45/AppData/Local/Temp/claude/596429fce533a335/e67e3e1e-d1b7-4a5d-97de-65edca98024b/scratchpad/png/final-{light,dark}.png`다.

## README 자원 그림(2026-11-12)

- 조사 끝(성공). 값은 `Measurements/Full1/summary.csv`(1만, 오브 시점, `L1-full`\~`L8-q4final`)의 `mem_total_mb`, `gpu_mem_p50`, `draw_calls_p50`이다. 셋 다 실행마다 프레임 값의 중앙값을 내고(`Scripts/summarize.py`), summary는 세 실행(r1\~r3, r0과 A/A r11\~ 제외)의 중앙값이다. `gpu_mem_p50`은 summary에 최소·최대 열이 없어 범위를 `runs.csv`의 같은 세 실행에서 셌다. 뜻은 엔진 소스로 확인했다. `mem_total_mb` = CSV `PhysicalUsedMB`(`Core/Private/ProfilingDebugging/CsvProfiler.cpp:4189`, `MemoryStats.UsedPhysical`) = 프로세스 작업 집합(`Core/Private/Windows/WindowsPlatformMemory.cpp:342` `WorkingSetSize`), MB. `gpu_mem_p50` = CSV `GPUMem/LocalUsedMB`(`RHI/Private/RHIStats.cpp:91`) = DXGI `QueryVideoMemoryInfo`의 로컬(VRAM) `CurrentUsage`(`RHICore/Private/DXGIUtilities.cpp:111`), 이 프로세스가 쓰는 VRAM, MB. `draw_calls_p50` = CSV `RHI/DrawCalls`(`RHI/Private/GPUProfiler.cpp:2107`), 프레임당 드로우 콜 수(GPU 전체 합). 평가의 값은 CSV와 맞다: 프로세스 메모리 3,083.0 → 1,936.4MB, GPU 메모리 8,642.8 → 3,355.3MB, 드로우 콜 10,269.5 → 1,713(반올림해 평가의 3,083→1,936, 8,643→3,355, 10,270→1,713).

## README 실행 전제(2026-11-12)

- 조사·편집 끝(성공). README "실행" 절 첫머리에 전제 세 줄을 넣고, 순서는 `Docs/Guides/commands.md`에 "처음 준비" 절을 새로 써서 모았다. 스크립트는 읽기만 했고 실행하지 않았다.
- 확인한 사실과 출처:
  - 엔진: `HordeOptLab.uproject` `EngineAssociation`(GUID)을 `Scripts/common.ps1`이 `HKCU\Software\Epic Games\Unreal Engine\Builds`에서 찾고, 없으면 오류로 멈춘다. 버전 UE 5.8.4 소스 빌드는 `Docs/Reference/pc-specs.md` 2.3.
  - 에셋: `.gitignore`가 `Content/ParagonMinions/`, `Content/NiagaraExamples/`, `Content/Lab/Derived/`, `Content/Lab/VAT/Baked/`, `Content/Lab/ISKM/Baked/`를 제외한다. 커밋된 `Content/Lab/`은 `git ls-files` 기준 AnimBP 2, FX 4, 맵 `L_Bench`, `M_LabBandFloor`뿐이다. Paragon: Minions는 `Docs/Reference/references.md`에 Fab, 무료, 재배포 금지로 적혀 있다.
  - 순서: `migrate-assets.py` 헤더(세트 `minion`/`siege`/`orbfx`), `make-static-minion.py`·`bake-vat.py` 헤더("미니언 이전 뒤"), `create-iskm.py` 헤더("bake-vat.py 뒤"), `Scripts/create-crowd-fx.md`("bake-vat.py를 소형·대형 둘 다"). `LabOrb.cpp`가 `/Game/NiagaraExamples/...`를 읽는다.
  - 단계 이름: `LabConfig.cpp`의 `-LabStage=` 파싱이 M0\~M5를 받는다. README 표기가 맞아 고치지 않았다.
- 확인 못 함:
  - 깨끗한 클론에서 처음부터 끝까지 돌려 본 적이 없다(스크립트 실행 금지). 순서는 스크립트 헤더와 문서에서 읽은 것이다.
  - `make-static-minion.py`를 대형으로도 돌려야 하는지는 헤더의 `LAB_UNIT=large` 분기만 보고 적었다. 소형만으로 충분한 단계가 있는지는 모른다.
  - `orbfx`의 원본인 Niagara Examples Pack을 어디서 받는지(Fab 여부)는 저장소에 없다. README와 commands.md는 받는 곳을 적지 않았다.
  - `create-anim-bp.py`와 `create-band-floor.py`, `create-bench-map.py`는 산출물이 이미 커밋돼 있어 전제에 넣지 않았다.
  - 마이그레이션 명령의 경로는 작성자 PC의 것이다. 다른 PC에서는 바꿔야 한다.
- 그림 끝(성공). `Scripts/plot-readme.py`에 `resources()`를 더해 `Docs/figures/readme/resources.svg`를 만든다(`ladder.svg`와 같은 글꼴·색·흰 카드·단계 이름, 글자 16·17·22px만, 스크립트·외부 글꼴 없음). 단위가 달라 세 지표를 칸 셋으로 나누고 칸마다 1단계 값을 막대 최대로 두었다. 값은 막대 끝에 적고, 세 실행 범위는 가는 선으로 그렸다. 아래 주석은 4단계부터 다시 느는 곳을 단계와 그 단계에서 바꾼 것으로만 적었다(GPU 메모리 4·6·7단계, 드로우 콜 4·8단계). 원인은 적지 않았다. 스크립트가 각 증가를 measurement.md 판정(앞뒤 단계 차이 > 두 단계의 세 실행 범위 중 큰 쪽)으로 확인하고, summary 값이 세 실행의 중앙값인지도 확인한다(assert). 프로세스 메모리는 6→7단계 +175MB가 6단계 범위(193MB)보다 작아 주석에 넣지 않고, 같은 단계의 실행 사이 차이가 최대 약 190MB라는 문장만 넣었다. 다른 SVG 다섯 개는 다시 돌려도 바뀌지 않았다.
- README 끝(성공). "결과 한눈에 보기"의 단계별 화면 표 바로 아래(최대 개체 수 그림 앞)에 그림, alt 텍스트, 두 문장(같은 실행에서 잰 값, 4단계부터 다시 늘고 그 단계는 그림에 적음)을 넣었다. 수치는 글로 되풀이하지 않았다. 다른 절은 건드리지 않았다. `Scripts/README.md`의 `plot-readme.py` 설명에 이 그림을 더했다.
- 확인 끝(성공). Edge 헤드리스로 흰 배경과 #0d1117 위에 800px 폭으로 렌더링해 겹침·잘림이 없음을 눈으로 확인했다. 확인용 PNG는 저장소 밖 `C:/Users/hon45/AppData/Local/Temp/claude/596429fce533a335/64ef7ba8-383f-405e-b020-8c8cb70db270/scratchpad/png/resources-{light,dark}.png`다.
- 커밋 끝(성공). cfe50de(`Scripts/plot-readme.py`, `Docs/figures/readme/resources.svg`, `README.md`, `Scripts/README.md`, 이 절). 이 절의 첫 두 줄(제목, 조사 끝)은 다른 세션의 커밋 108e00c에 먼저 들어갔다. 푸시하지 않았다.
