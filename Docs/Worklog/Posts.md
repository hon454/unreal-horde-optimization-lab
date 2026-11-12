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

## README 사다리 단계 구분(2026-11-12)

- 그림 끝(성공). `Scripts/plot-readme.py`의 `ladder()`, `max_entities()`, `resources()`에서 1\~6단계와 7·8단계를 나눴다. 묶음 이름은 "1–6단계 · 기법을 더해 프레임 시간을 버는 단계"와 "7·8단계 · 번 시간을 품질에 쓰는 단계"이고(README의 품질 단계 문장과 같은 뜻), 7단계 앞에 진한 구분선을 둔다. 7·8단계 막대는 같은 색 위에 흰 빗금 무늬를 얹어 색만으로 구분하지 않게 했고, 그림마다 아래에 빗금 범례를 넣었다(공용 `SPEND_FROM`, `GROUPS`, `HATCH`, `bar()`, `group_head()`, `hatch_key()`). 사다리의 16.7ms 선은 이미 두 칸(축척 A 0\~450ms, 축척 B 0\~17ms)에 있어 그대로 두었고, 7·8단계 막대가 선 안쪽임이 그림에서 읽힌다. 최대 개체 수 그림은 2단계(20,250)와 8단계(16,826)가 다른 묶음에 들어가는 것으로 그쳤고 원인은 적지 않았다. 목표 1만 선이 첫 묶음 이름을 지나가던 것을 이름 아래에서 시작하게 고쳤다. 빼 보기·전환·시나리오 그림은 바뀌지 않았다.
- README 끝(성공). 최대 개체 수 그림 아래 문장 "7단계와 8단계는 앞 단계에서 번 프레임 시간을 … 쓰는 품질 단계다"를, 그림이 말하게 된 구분을 빼고 "7·8단계는 번 시간을 배경 무리와 액터 수, 전환 거리에 쓴다"로 줄였다. 다른 절은 건드리지 않았다.
- 확인 끝(성공). Edge 헤드리스로 세 그림을 흰 배경과 #0d1117 위에 800px 폭으로 렌더링해 겹침·잘림이 없음을 눈으로 확인했다. 글자는 16·17·22px만. 확인용 PNG는 저장소 밖 `C:/Users/hon45/AppData/Local/Temp/claude/596429fce533a335/64ef7ba8-383f-405e-b020-8c8cb70db270/scratchpad/png/stage-{ladder,max-entities,resources}-{light,dark}.png`다.
- 범위 표시 다시 그림(성공, 사용자 검토 반영). `resources.svg`의 검은 수염(가는 가로선과 끝 세로선)이 렌더링 흠처럼 보여, 막대 뒤에 같은 색 옅은 띠(불투명도 0.28, 둥근 끝, 막대보다 위아래 4px 큼)를 최소\~최대 구간에 깔고 막대는 중앙값까지 그리도록 바꿨다. 범위가 2px보다 좁은 막대는 띠를 그리지 않고(`RANGE_MIN_PX`), 부제를 "막대 둘레의 옅은 띠: 세 번 잰 범위(좁아서 2px가 안 되는 범위는 그리지 않았다)"로 고쳤다. 띠가 그려지는 곳은 프로세스 메모리 2·4·5·6단계뿐이다. 7·8단계 빗금 막대는 범위가 2px보다 좁아 띠가 없다. 사다리·최대 개체 수·빼 보기에는 범위 표시가 없고, 전환 그림은 이미 같은 색 옅은 막대와 점이라 건드리지 않았다(렌더링으로 확인). 확인용 PNG는 저장소 밖 `C:/Users/hon45/AppData/Local/Temp/claude/596429fce533a335/64ef7ba8-383f-405e-b020-8c8cb70db270/scratchpad/png/range-{resources,ablation,transition}-{light,dark}.png`다.

## 평가 반영 가운데 끝난 것(2026-11-12)

받은 평가 여덟 항목을 사용자와 하나씩 정했다. 끝난 항목의 경위를 STATUS.md에서 옮긴다. 남은 항목(입자 수정과 재측정, 참고 묶음, 엔티티 풀, 코드 안내 문서, 릴리스)은 STATUS.md에 있다.

- **AI 협업 흔적(평가 3번).** 그대로 둔다. README에 작업 방식 단락을 넣지 않고 소스 주석도 정리하지 않는다. 대신 AGENTS.md를 주제와 역할별로 다시 짰다(aaef5b8. 규칙 내용은 그대로이고, 지운 결정 날짜는 `Docs/Decisions/README.md` "작업 방식의 결정"에 옮겼다). 이어서 지침 파일 점검에서 나온 여섯 군데를 고쳤다(359befa: 끊어진 절 이름 참조 넷, 첫 문단의 단계 문장, 시작 절차의 코디네이터 범위, 사람에게 하는 문장 삭제).
- **README에 빠진 것(평가 5번).** 단계별 프로세스 메모리, GPU 메모리, 드로우 콜 그림을 더했다(`resources.svg`, cfe50de. 범위 표시는 사용자 검토로 다시 그렸다, 4821c23). "실행" 절에 전제 세 줄을 넣었다(108e00c, `Docs/Guides/commands.md` "처음 준비"). MIT 라이선스를 넣었다(d5d2e93).
- **사다리 그림의 단계 구분(평가 6번).** `ladder.svg`, `max-entities.svg`, `resources.svg`에서 1\~6단계와 7·8단계를 구분선과 묶음 이름, 빗금 막대로 나눴다(b4675be).
- **README "시나리오" 절.** 위에서 내려다본 그림으로 바꿨다(`scenario.svg`, bdd3fbe).

## Full2 값 반영(2026-11-13)

문서 세션이 공개 문서와 측정 규약의 수치를 총괄 묶음 `Full2`(Test 패키지 08e45f2) 값으로 바꾼다. 근거는 [Q6.md](Q6.md) "Full2 재측정"이다. 에디터, 빌드, 패키징, 게임 실행은 하지 않았다.

### 진행

- README 그림 끝(성공). `Scripts/plot-readme.py`가 `Measurements/Full2/`와 `Docs/figures/full2/`를 읽는다. 전환 그림만 `Measurements/Full1/transition-dev/runs.csv`를 그대로 읽는다. 엔티티 풀 그림 `entity-pool.svg`를 더했다(`Measurements/Pool1/timeline.csv`). 빼 보기 그림은 행을 프레임 변화 순으로 놓고, 병목이 바뀐 행과 GPU 차이가 프레임에 드러나지 않는 행에 표시를 단다. SVG 일곱 개의 글자를 `summary.csv`, `full-n-at-60fps.csv`, `timeline.csv`와 대조했다. 다음은 README 문장.
- README 문장 끝(성공). 첫 문단 440ms, 요약 표 머리 7.8ms, 빌드 08e45f2, 4단계부터 게임 스레드 병목, 최종 구성 7.78ms·약 2만 6천, 빼 보기 문단(ISKM은 GPU 시간, 80m 밖 그림자는 GPU만), 엔티티 풀 절과 그림, 정확한 값 링크, 명령의 `--batch Full2`를 고쳤다. 2단계 2만의 "3.35ms 갈려" 문장은 지웠다(`Full2` 세 실행 범위 14.68\~15.14ms, 0.46ms). 다음은 measurement.md.
- measurement.md 끝(성공). `N@16.7ms` 값, `L8` 건너뜀 비율, 80m 밖 그림자 행(GPU +0.39ms, 프레임 구별 안 됨, 결정은 ADR-0010 그대로)을 `Full2` 값으로 고쳤다. 절 "총괄 묶음 `Full1` 규약"을 "총괄 묶음 규약"으로 바꾸고 "묶음" 행을 더했다. "참고 묶음 규약", "엔티티 풀 묶음 규약" 절을 더했다. ADR-0013 "결과"에 `Full2` 소절과 절 이름 변경을 덧붙였다. 다음은 01번 글.
- 01번 글 끝(성공). 본문 수치 열한 곳과 `measurements.md`의 표·빌드(08e45f2)·실행 라벨을 `Full2`의 `L1-full`·`S1-uro`·`S1-budget` 값으로 바꿨다. `make-figures.py`가 `Full2`를 읽게 하고 SVG 넷을 다시 만들었다. 트레이스 값은 `Full1` 트레이스 그대로 두고 이유를 "측정 기록의 변경" 절에 적었다. 다음은 ADR-0010·0011 "결과".
- ADR 결과 끝(성공). ADR-0011 "결과"에 Q6.md의 제안 문장을, ADR-0010 "결과"에 `A8-farshadow`의 `Full2` 값을 날짜와 함께 덧붙였다. 다른 절은 고치지 않았다. 다음은 그 밖의 `Full1` 참조와 링크 확인.
- 그 밖의 `Full1` 참조 끝(성공). AGENTS.md 문서 지도, `Docs/Guides/commands.md`(절 이름 "총괄 묶음 러너·검사·요약·촬영", 명령 예시, `-Batch` 행), `setup.md`의 절 이름 링크, `posting.md`의 CSV 링크 규칙, README "실행"의 `-Batch Full2`를 고쳤다. ADR-0015 "결과"에 링크 변경과 새 그림을 덧붙였다. 바꾼 문서 열세 개의 상대 링크는 모두 실제 파일을 가리킨다. README에는 파일 경로와 명령 밖의 묶음 이름·조건 라벨이 없다. 커밋은 3951de6, d03b40e, 51555d5, 0254691, 7c6f097과 이 절의 커밋이다. 푸시하지 않았다.

### 결정

- **README 빼 보기 그림의 행 순서.** 프레임 시간 변화가 큰 순(느려지는 쪽 위)으로 바꿨다. 대안은 옛 고정 순서(하이드레이션부터 기법 순)였다. 병목이 게임 스레드로 바뀌어 옛 순서의 뜻("GPU에 가려진 게임 스레드 차이")이 사라졌다. 크기 순이 그림만으로 읽힌다. 표시는 데이터로 단다: 병목이 바뀐 행("병목이 GPU로 바뀜", 하이드레이션), GPU 차이가 프레임에 드러나지 않는 행(ISKM, 80m 밖 그림자).
- **엔티티 풀 그림을 README에 넣었다.** 브리프의 기본안이다. 실행이 한 번씩이라 범위가 없다는 것을 그림 부제와 본문에 적었다. ADR-0015 결정 1(비교는 SVG)에 맞다.
- **엔티티 풀 문단에 원인 한 문장.** 엔진이 파괴된 엔티티의 ISKM 트랙을 돌려받지 않는다는 것(`MassVisualizationComponent.cpp`, [M5.md](M5.md) 12행의 소스 확인)을 적었다. 이슈 링크는 찾지 않았다.
- **ISKM을 쓰는 이유의 "굽기 단계가 없다".** ISKM도 `UAnimSequenceTransformProviderData` 에셋을 만든다(`Scripts/create-iskm.py`). 그래서 "애니메이션을 텍스처로 굽는 단계(AnimToTexture)가 없다"로 좁혀 적었다.
- **참고 묶음 값은 README에 넣지 않았다.** 브리프 기본안과 D21 뒤 결정을 따랐다.
- **전환 그림 문단.** 그 그림만 입자 메시를 바꾸기 전의 빌드에서 쟀다는 한 문장을 더했다. 전환 그림은 다시 재지 않았기 때문이다.
- **01번 글의 트레이스 값은 `Full1` 트레이스 그대로 둔다.** 대안은 `Full2` 트레이스를 내보내 바꾸는 것이다. 내보내기는 Unreal Insights를 돌려야 하고 촬영 중이라 할 수 없다. 두 빌드의 `Source/`·`Config/`가 같고(`git diff --stat 730ff18 08e45f2`) 1단계는 배경 무리를 쓰지 않으므로 값의 뜻은 같다. 이유를 글의 측정 기록 "측정 기록의 변경"에 적었다.
- **measurement.md 절 이름.** "총괄 묶음 `Full1` 규약"을 "총괄 묶음 규약"으로 바꿨다. 다음 묶음에서 다시 바꾸지 않아도 된다. 같은 이유로 commands.md 절 "총괄 묶음(Full1) 러너·검사·요약·촬영"도 묶음 이름을 뺐다. ADR-0013 본문의 옛 절 이름은 고치지 않고 "결과"에 덧붙였다.
- **README "실행" 명령에 `-Batch Full2`를 붙였다.** 러너 기본값이 이미 쓴 `Full1`이라, 붙이지 않으면 아래 `--batch Full2` 명령과 맞지 않는다. 러너 기본값은 고치지 않았다(측정 세션의 파일).

### 확인하지 못한 것

- SVG는 브라우저로 열어 보지 않았다(촬영 중이라 브라우저를 띄우지 않았다). 글자와 값만 텍스트로 대조했다. 엔티티 풀 그림의 선과 끝값 겹침, 빼 보기 그림의 표시 줄 길이는 화면에서 확인해야 한다.
- 엔티티 풀 그림의 "풀 켬 트랙이 약 2만에서 멈추는 이유"는 Q6.md대로 확인하지 않았다. README는 사실만 적었다.
- 01번 글: `Full2` 트레이스 16개는 열지 않았다. 직렬 트레이스의 1단계 값이 `Full1`과 같은지 보지 않았다.

### 코디네이터용

- **STATUS.md 40행:** "총괄 묶음 `Full1` 규약" → "총괄 묶음 규약".
- **STATUS.md "측정 결과":** "`A8-iskm`은 … ISKM의 이점은 GPU 시간으로 적는다"는 이미 맞다. "다음 할 일" 1번은 끝났다(이 절의 커밋들).
- **backlog.md 22행:** 09번 글의 묶음 `Full1` → `Full2`. 27행의 "총괄 묶음 `Full1`은 …" 문장도 지금 묶음으로 고칠지 본다.
- **plan.md 276·282·283행:** `Full1`을 이름으로 적은 설명이다. 경위로 둘지 "지금은 `Full2`"를 더할지 정한다.
- **ADR-0012 "결과":** `Full1` 실행 결과만 있다. 빼 보기 스위치의 `Full2` 값을 덧붙일지 정한다(이 세션은 ADR-0010·0011·0013·0015만 덧붙였다).
- **스크립트 기본값:** `run-full.ps1`·`capture-full.ps1`·`run-transition.ps1`의 `-Batch` 기본값과 `full-conditions.json`의 `batch`가 `Full1`이다. 측정 세션 몫이라 고치지 않았다.
- **엔진 문제 인용:** README의 ISKM 트랙 누수 문장에는 이슈 링크가 없다. 글 06(ISKM)에서 이슈 링크와 해결 코드 블록을 보인다.

### 묶음 이름 숨김(2026-11-13, 코디네이터가 전한 사용자 지시)

- README와 01번 글의 독자용 문장에서 묶음 이름을 "최종 측정"으로 바꿨다. 링크 글자에는 경로를 두지 않는다(`[최종 측정의 표](…)`). README "실행"의 명령은 `-Batch <묶음>`으로 썼다. 01번 측정 기록에는 실행 라벨(`Full2-…`, `Full1trace…`)만 남았다. `S1-budget` r3 GPU 튐 문장 둘은 옛 측정과 견주는 문장이라 지웠다.
- 확인: README에서 링크 주소 밖의 `Full`은 0건이다. 상대 링크는 모두 살아 있다.
- 01번 글 트레이스: 코디네이터가 `Full2trace-*`·`Full2traceser-*`로 다시 뽑으라고 했다. `Scripts/trace-export.ps1`은 Unreal Insights를 띄우고 스크린샷 값은 Insights 화면에서 읽은 값이라 촬영 중에는 할 수 없다. 730ff18 트레이스 값 그대로 두고, 어느 트레이스의 값인지와 이유를 측정 기록 "측정 기록의 변경"에 적었다. 촬영이 끝난 뒤 다시 뽑을 일로 남긴다.

## 02 Mass 로직(2026-11-13)

`Posts/02-mass-logic/`를 쓴다. 수치는 최종 측정(Test 패키지 08e45f2)의 `L1-full`·`L2-mass` 행과 `full-n-at-60fps.csv`에서만 만든다.

### 진행

- 조사 끝(성공). 읽은 것: AGENTS.md, STATUS.md, posting.md, ADR-0014, backlog.md, post-review.md "02 Mass 코어", post-material.md "02", measurement.md, engine-notes.md 1·3·11·12·13·27절, 01번 글 전체, `full-conditions.json` `L2-mass`(note "로직을 Mass로, 표현은 애니 없는 ISM"), plan.md M1과 사다리, code-tour.md, `Source/HordeOptLab/`(LabMassTypes.h, LabMassTraits.cpp, LabCrowdSubsystem.cpp, LabMassProcessors.cpp, HordeOptLab.cpp, LabMinion.cpp/.h, LabOrb.cpp), `Config/DefaultMass.ini`. 엔진 소스: `MassMovementProcessors.cpp:41,100`, `MassRepresentationTypes.h:105`, `MassUpdateISMProcessor.cpp`, 기본으로 꺼진 프로세서 셋. 다음은 초안.
- 그림 끝(성공). `make-figures.py`가 SVG 넷(`mass-summary`, `mass-processors`, `mass-draw-calls`, `mass-frame-vs-n`)을 만든다. 입력은 `summary.csv`와 `full-n-at-60fps.csv`뿐이고 트레이스 값은 없다. 다음은 초안.
- 초안 끝(성공). `README.md`를 posting.md 틀(요약, 시나리오 카드, 배경 지식 셋, 문제, 원리, 바꾼 것, 결과, 한 문장으로)로 썼다. 본문은 코드·그림·제목을 빼고 약 4,000자(공백 포함, 시나리오 카드 포함)다. 엔진 코드 발췌는 14줄 하나(`UMassApplyMovementProcessor::Execute`)다. Insights 자리는 주석으로 두었다. 다음은 측정 기록.
- 측정 기록 끝(성공). `measurements.md`: 빌드와 묶음, 2단계 구현 위치(파일:줄), 본문의 수치와 출처(세 회차 값, 계산식, 보간식), HUD 차이, 그림, 코디네이터가 찍을 Insights 화면, 엔진 소스, 발췌 원본 줄(엔진·프로젝트), 경위의 근거, 확인하지 않은 것. 다음은 확인.
- 확인 끝(성공). 상대 링크는 모두 실제 파일을 가리킨다(주석 안의 `images/insights-mass-processors.jpg`만 아직 없다). 00·03번 글이 커밋돼 있어 "00번 글"과 "다음 글"에 링크를 걸었다. 코드 스팬 밖 `~`는 모두 `\~`다. 독자용 문장에 묶음 이름·조건 라벨·마일스톤 이름·다른 프로젝트 이름이 없다(이미지 경로에만 있다). 본문 수치를 측정 기록 표와 하나씩 대조했다. SVG 넷의 숫자 글자를 `summary.csv`·`full-n-at-60fps.csv`와 대조했고 글자 크기는 모두 16px 이상이다. 글자 겹침은 좌표로 어림해 두 곳(결과 그림의 500 값과 774 표시, 원리 도식의 Chunk 제목줄)을 고쳤다. 다음은 커밋.
- 커밋 끝(성공). bed7b5c `docs(posts): write post 02 on moving the horde logic to Mass`(`Posts/02-mass-logic/`, 이 절). 이 줄은 뒤 커밋에 든다. 푸시하지 않았다.

### 결정

- **변수 둘을 본문에 밝힌다.** 사다리 1 → 2단계는 로직의 자리와 표현(애니메이션 있는 스켈레탈 메시 → 애니메이션 없는 ISM)이 함께 바뀌고 최종 측정에 둘을 가르는 조건이 없다. posting.md "바뀐 변수가 하나임을 명시"를 지킬 수 없어 "바꾼 것" 첫 문단과 "한 문장으로" 목록에 "변수 둘"을 적었다. 결과 그림에서 두 단계 계열을 잇지 않았다(measurement.md "차이 판정"). 버린 대안: 옛 묶음의 애니 없는 액터 변형으로 분해(묶음이 달라 규칙 위반).
- **문제 절의 증거를 드로우 콜 그림으로 한다.** 1단계의 게임 스레드 원인은 01번 글이 트레이스로 보였으므로 한 문장과 링크로 받고, 이 글에서 새로 필요한 사실("GPU 50.2ms가 남아 표현도 바꿔야 한다")을 최종 측정의 `draw_calls_p50`로 보였다. 버린 대안: 01번 글의 `actor-frame-flow.svg`를 다시 싣기(트레이스 값이 든 그림이라 "트레이스에서만 나오는 수치는 본문에 쓰지 않는다"에 걸린다).
- **결과의 "예상과 실제"를 개체당 게임 스레드 시간으로 그린다.** 1단계는 하위 스텝이 없는 500\~2천의 17µs가 기준이 되고, 2단계는 5천\~3만에서 0.62µs로 평평하다. 1만끼리의 67.4배 가운데 하위 스텝 몫과 개체당 비용 몫(27.0배)을 나눠 읽을 수 있다. 버린 대안: 프레임 시간 배율만 쓰기(병목이 GPU로 바뀌어 게임 스레드의 변화가 가려진다).
- **엔진 코드 발췌는 `UMassApplyMovementProcessor::Execute` 하나(14줄)다.** Chunk 순회의 모양과 0.1초 상한을 한 블록에서 보인다. 그림자 기본값과 기본으로 꺼진 Processor는 발췌 없이 파일·심볼을 측정 기록에 두었다. 0.1초 상한은 엔진 주석대로 "폭주 방지 장치"로 적고 알려진 문제로 쓰지 않았다(그래서 이슈 링크가 필요 없다).
- **프로젝트 코드 발췌의 줄 끝 주석을 뺐다.** 주석에 작업 단계 이름(M3b 등)이 있어 공개 문장 규칙에 걸린다. 뺀 줄을 측정 기록 발췌 표에 적었다.
- **Lumen은 한 문장으로만 쓰고 수치를 쓰지 않는다.** 옛 시험 실행 값이고 최종 측정에 대응 조건이 없다. 모든 단계 공통 설정이라 2단계에서 바뀐 변수도 아니다. 03번 글이 "Lumen 항목은 02번 글이 맡는다"고 정해 이 글에 두었다.
- **Insights 화면은 직렬 트레이스로 찍게 했다.** 브리프는 병렬 트레이스 `Full2trace-L2-mass-*`를 가리켰지만, 병렬 실행은 Processor가 워커 스레드로 흩어져 한 화면에서 읽기 어렵다(measurement.md "산출물"). 직렬 `Full2traceser-L2-mass-*`를 권하고 병렬을 대안으로 적었다.
- **최대 개체 수는 20,753으로 적었다.** posting.md "개수는 줄이지 않는다"와 01·00번 글을 따랐다.
- **본문 분량.** 시나리오 카드와 목록을 넣고 공백을 세면 약 4,000자다. 코드 블록이 넷이라 읽는 시간은 01번 글보다 길다.

### 확인하지 못한 것

- SVG와 WebP를 화면에서 보지 않았다(브라우저 금지). 겹침은 좌표와 글자 수로만 어림했다. `mass-processors.svg`의 칸 안 글자 길이, `mass-frame-vs-n.svg` 위쪽 판의 값 글자와 선의 겹침은 화면에서 봐야 한다.
- 2단계 화면에서 "걷는 동작 없이 한 자세로 움직이고, 피격·사망 때 멈췄다가 사라진다"는 코드에서 판단했다. WebP를 재생해 보지 않았다.
- 로직과 표현 가운데 어느 쪽이 얼마를 줄였는지, GPU 50.2ms의 패스별 내역, 게임 스레드 6.30ms의 내역, 병렬 처리의 몫.
- 최종 측정의 트레이스(`Full2trace*-L2-mass-*`)는 열지 않았다.

### 코디네이터용

- **Insights 화면:** `Saved/Traces/Full2traceser-L2-mass-N10000-P1-r1.utrace`(직렬)를 열어 25\~75초 안의 보통 프레임 하나를 고른다. 왼쪽 빨간 상자는 그 프레임(GameThread 트랙을 펼쳐 Mass PrePhysics 처리 단계 안의 Processor 타이머와 PostPhysics의 `MassVisualizationComponent EndVisualChanges`가 보이게), 오른쪽 빨간 상자는 Timers 창을 "Processor"로 거르고 Incl 내림차순으로 둔 행이다. `images/insights-mass-processors.jpg`로 넣고, README "결과"의 주석을 풀어 캡션의 OO(프레임 길이, Mass 처리 단계 Incl, `EndVisualChanges` Incl, 가장 긴 Processor 이름과 Incl)를 채운다. 읽은 값은 측정 기록 "본문의 수치와 출처"에 "직렬 트레이스 한 프레임"으로 행을 더한다. 자세한 지시는 측정 기록 "코디네이터가 찍을 Insights 화면".
- **루트 README "글" 표와 `Posts/README.md`:** 02번 행을 더한다.
- **backlog.md 15행:** 02번 글 상태를 바꾼다. "글에 넣을 것"의 Mass ISM 그림자 기본값과 Lumen 항목은 02번 글이 다뤘다(Lumen은 수치 없이 한 문장).
- **00번 글 "여덟 단계의 지도":** 02번 항목에 `../02-mass-logic/README.md` 링크를 건다. 제목은 "02 로직을 Mass로 — 액터마다 돌던 틱이 Processor 몇 개가 된다"다.
- **engine-notes.md 11절 "이동 프로세서의 시간 상한" 행:** 엔진 주석의 의도("force explosion" 방지, `MassMovementProcessors.cpp:40,99`)를 한 줄 더할지 정한다. 이 세션은 그 파일을 고치지 않는다.

## 00 목적·목표 사양·측정 방법(2026-11-13)

`Posts/00-goals-and-method/`를 쓴다. 수치는 최종 측정(Test 패키지 08e45f2)의 `summary.csv`·`runs.csv`·`full-n-at-60fps.csv`에서만 만든다. 에디터, 빌드, 패키징, 게임, 브라우저는 띄우지 않았다.

### 진행(00)

- 조사 끝(성공). 읽은 것: AGENTS.md, STATUS.md, posting.md, ADR-0001·0014·0015, backlog.md, post-review.md "핵심 인사이트"·"재료 선별"·"00" 항목, post-material.md "01 프레임 시간"·"02 Game Thread 시간"·"00", measurement.md, plan.md "품질 계획", README.md, 01번 글 전체, pc-specs.md, `full-conditions.json`, `Source/HordeOptLab/LabHud.cpp`, `Scripts/plot.py` `n_at_budget`. 엔진 소스: `LaunchEngineLoop.cpp:1518-1520`, `UnrealClient.cpp:1842-1845`, `CsvProfiler.cpp:4383-4390`, `RenderingThread.cpp:2435-2450,2505-2510`. 다음은 초안.
- 그림 끝(성공). `make-figures.py`가 SVG 넷을 만든다: `frame-pipeline.svg`(개념도, 값 없음), `judging-difference.svg`, `max-entities-method.svg`, `stage-map.svg`(설정값만). 글자 폭을 어림해 오른쪽 끝을 넘는 글자를 고쳤다. 요약과 시나리오는 README 그림 `ladder.svg`, `scenario.svg`를 상대 경로로 쓴다. 다음은 초안.
- 초안 끝(성공). `README.md`: 요약, 시나리오 카드, 배경 지식 셋(Test 구성, CSV 프로파일러, Unreal Insights), 목적, 목표 사양, 시나리오, 측정 방법 여섯 소절, 여덟 단계의 지도, 한 문장으로. 엔진 발췌 17줄. 첫 초안이 본문 약 5,700자라 줄여 약 4,300자(배경 지식 제외)로 맞췄다. 다음은 측정 기록.
- 측정 기록 끝(성공). `measurements.md`: 빌드와 묶음, 본문의 수치와 출처(세 회차 값), 그림의 값(그림마다 표), 시나리오와 절차의 출처(설정 파일과 줄), 엔진 소스, 발췌 원본 줄, 경위의 근거, 확인하지 않은 것. 다음은 확인.
- 확인 끝(성공). 상대 링크 26개가 모두 실제 파일을 가리킨다. 코드 스팬 밖 `~`는 모두 `\~`다. README 산문에 묶음 이름·조건 라벨·마일스톤 이름·측정 약어가 없다(링크 주소 안의 파일 이름만 있다). 본문 수치를 측정 기록 표와 하나씩 대조했다. SVG 넷의 글자를 `summary.csv`·`full-n-at-60fps.csv`와 대조했다. 다음은 커밋.
- 커밋 끝(성공). 2eb698f. 푸시하지 않았다.

### 결정(00)

- **요약 그림은 README의 `ladder.svg`를 가져다 쓴다.** 대안은 글 전용 요약 그림을 새로 그리는 것이다. 00번 글의 요약은 연재 전체의 결과라 사다리 그림이 맞고, 브리프가 맞는 README 그림의 재사용을 권했다. 그림의 32개 값은 측정 기록 "그림의 값"에 표로 옮겼다.
- **"가장 느린 쪽" 설명은 엔진 코드 둘과 개념도 하나로 한다.** 게임 스레드 시간의 정의(`UnrealClient.cpp`)와 프레임 동기화(`RenderingThread.cpp` `FFrameEndSync`)가 원리를 정하는 코드라 발췌했다(합 17줄, 한도 30줄). 데이터 예는 새 그림 대신 요약 그림의 3·4단계를 문장으로 짚었다. 대안(별도 막대 그림)은 요약 그림과 같은 값을 되풀이한다.
- **차이 판정 그림의 예는 A/A, `A8-fix`, `A8-budget`이다.** "구별되지 않음" 둘과 "차이 있음" 하나를 같은 축에 둔다. 측정 끝 A/A는 재현성의 증거도 겸한다. 대안 `A8-hydrate`는 병목이 바뀌어 판정 규칙보다 병목 설명이 앞선다.
- **최대 개체 수 그림은 최종 구성 한 계열만 그린다.** 단계별 값은 README `max-entities.svg`에 있고, 이 그림은 읽는 법(보간과 p50·p95의 차이)이 요점이다.
- **여덟 단계의 지도는 거리별 표현 구성 그림으로 그린다.** 단계마다 바꾼 것이 대부분 "어느 거리를 무엇으로 그리나"라서 한 장에 모인다. 표현과 무관한 변경(애니메이션 예산, Simulation LOD, 엔티티 풀, 고정 바운드)은 줄마다 한 줄 문장으로 적었다. 링크는 그림 안에 걸 수 없어 아래 목록에 둔다.
- **HUD 읽는 법을 넣었다.** backlog의 00번 항목에 "HUD 읽는 법"이 있다. `LabHud.cpp`에서 배치와 색 규칙을 확인해 한 문단으로 적었다. 화면의 숫자는 인용하지 않는다(8단계 화면은 다시 찍는 중이다).
- **환경이 수치를 흔든 사례는 수치 없이 방향만 적었다.** 옛 묶음의 값이고 최종 측정에 대응 조건이 없다. 대신 지금의 장치(러너 검사, 스파이크 0건, A/A)를 최종 측정의 값으로 적었다. 에디터 사례는 글에서 빼고 러너 검사 문장에만 남겼다(분량).
- **"여유분 20%(13.3ms)"는 쓰지 않았다.** plan.md에서 잠정값이고 확정 기록을 찾지 못했다.
- **본문 분량.** 공통 지표 설명을 모두 이 글에 두라는 규칙 때문에 배경 지식을 빼고 약 4,300자로 4,000자를 조금 넘는다. 시나리오 카드와 목록 글자를 빼면 약 4,100자다.
- **최대 개체 수는 25,958로 적었다.** posting.md "개수는 줄이지 않는다"와 01번 글(774)을 따랐다. measurement.md의 "유효 숫자 둘"과 README의 "약 2만 6천"과는 표기가 다르다.

### 확인하지 못한 것(00)

- SVG를 화면에서 보지 않았다(브라우저 금지). 글자 폭은 어림 스크립트로만 확인했다. `max-entities-method.svg`의 값 글자와 곡선의 겹침, `frame-pipeline.svg`의 빗금 모양, `stage-map.svg`의 빗금(그림자 끔) 대비는 화면에서 봐야 한다.
- `Waits`에 들어가지 않는 기다림이 게임 스레드 시간에 얼마나 섞이는지는 보지 않았다(measurement.md "함정"의 사례). 본문은 "잠든 시간이 빠진다"까지만 적었다.
- 프레임 시간이 가장 느린 스레드보다 조금 긴 이유는 가르지 않았다.
- 8단계 화면 `step8-L8-q4final.webp`는 촬영 세션이 다시 찍는 중이다. 새 화면의 HUD 배치가 본문 설명과 맞는지 보지 않았다.

### 코디네이터용(00)

- **글 링크:** 02\~09번 글은 아직 커밋되지 않아 "여덟 단계의 지도" 목록에 제목만 적었다(`Posts/00-goals-and-method/README.md` 143\~150행). 글이 커밋되면 `../02-mass-logic/README.md`처럼 링크를 건다. 제목은 backlog의 이름을 줄여 썼으니 실제 글 제목으로 맞춘다.
- **README "글" 표와 `Posts/README.md`:** 00번 행을 더한다.
- **backlog.md 13행:** 00번 글 상태를 바꾼다.
- **01번 글:** "지표의 자세한 설명은 00번 글에 둔다"를 `../00-goals-and-method/README.md` 링크로 바꿀 수 있다.
- **표기 맞추기:** 최대 개체 수의 표기(25,958 대 "약 2만 6천")를 글 사이에서 정한다.
- **engine-notes.md:** 스레드 시간의 정의는 284행에 있다. 없는 것 둘을 더할지 정한다(이 세션은 그 파일을 고치지 않는다): `Waits`에 드는 대기는 `Sleep`과 이벤트 대기의 `FThreadIdleStats::FScopeIdle`(`Core/Private/Windows/WindowsPlatformProcess.cpp:1870,1940`), 게임 스레드는 N−1 렌더 스레드 프레임과 맞추고 추가 울타리가 RHI 스레드·스왑체인보다 너무 앞서지 않게 한다(`RenderCore/Private/RenderingThread.cpp:2458-2478` `FFrameEndSync`, 기본값 `r.OneFrameThreadLag` 1, `r.GTSyncType` 0 `:2435-2450`).

## 03 ISM + VAT(2026-11-13)

`Posts/03-ism-vat/`를 쓴다. 수치는 최종 측정(Test 패키지 08e45f2)의 `L2-mass`·`L3-vatcull` 행과 `full-n-at-60fps.csv`에서만 만든다. 에디터, 빌드, 패키징, 게임, 브라우저, Insights는 띄우지 않았다.

### 진행(03)

- 조사 끝(성공). 읽은 것: AGENTS.md, STATUS.md, posting.md, backlog.md, post-review.md "03 ISM + VAT"와 "02 움직이는 ISM과 Lumen", post-material.md "02 움직이는 ISM과 Lumen"·"03", measurement.md "확정값", engine-notes.md 3·11절, references.md, 01번 글 전체, `full-conditions.json` `L3-vatcull`(note "ISM + VAT, 200m 밖 Off"), plan.md M2와 사다리, code-tour.md, Worklog M2.md·M3.md·Q6.md 트레이스 절, `Measurements/M3/shots/vat-shadow/README.md`, `Source/HordeOptLab/`(LabVatProcessor.cpp/.h, LabCrowdSubsystem.cpp, LabMassTraits.cpp, LabMassTypes.h, HordeOptLab.cpp, LabConfig.cpp, LabCrowdSpawner.cpp), `Scripts/bake-vat.py`. 엔진 소스: `AnimToTextureBPLibrary.cpp:245,290,1108`, `MassUpdateISMProcessor.cpp:15-37`, `MassRepresentationTypes.h:105`. 다음은 초안.
- 초안 끝(성공). `README.md`: 요약, 시나리오 카드, 배경 지식(VAT, 인스턴스 커스텀 데이터, Representation LOD, 650자), 문제, 원리(예상 포함), 바꾼 것, 결과(화면 소절 포함), 틀렸던 가설(세 문단), 한 문장으로. 본문은 표·코드·그림·제목과 배경 지식을 빼고 약 3,770자다. 다음은 그림.
- 그림 끝(성공). `make-figures.py`가 SVG 다섯 개를 만든다(요약, 원리 도식, 문제, 예상과 실제, 최대 개체 수). 값은 `summary.csv`·`full-n-at-60fps.csv`에서 읽고 트레이스 값은 내보낸 표(`%TEMP%\horde-trace\F1-*`)의 타이머 합 ÷ 프레임 수를 상수로 둔다. 반올림은 CSV 문자열에서 반올림(12.35 → 12.4)이 되게 `Decimal`로 했다. SVG의 숫자 글자를 측정 기록 표와 하나씩 대조했다. 다음은 측정 기록.
- 측정 기록 끝(성공). `measurements.md`: 빌드와 묶음, 3단계 구현 위치(파일:줄), 본문 수치 표(1만 네 값, 인스턴스 수와 거리, 2단계 호드 총수별, 느린 쪽 5% 경계, 트레이스, 탑뷰), 트레이스 파일과 프레임 수, 측정 기록의 변경, HUD 차이, 그림, 엔진 소스, 발췌 줄, 경위의 근거, 확인하지 않은 것. 다음은 확인.
- 확인 끝(성공). 두 파일의 상대 링크가 모두 실제 파일을 가리킨다(00·02번 글은 체크아웃에 있으나 커밋 전). 코드 스팬 밖 `~`는 없다. 독자용 문장에 묶음 이름·조건 라벨·마일스톤 이름은 없다(이미지 경로에만 있다). 본문 수치는 모두 측정 기록 표에 있다. 엔진 코드 발췌는 3줄이다. 다음은 커밋.
- 커밋(성공). dbbdb71 `docs(posts): write post 03 on ISM + VAT and far culling`(`Posts/03-ism-vat/`, 이 절). 이 두 줄은 뒤 커밋에 든다.

### 결정(03)

- **3단계가 두 변수를 함께 바꾼 것을 본문에 밝힌다.** 사다리의 3단계(`L3-vatcull`)는 VAT와 200m 밖 지우기를 함께 바꿨고 최종 측정에 VAT만 바꾼 조건이 없다. "바뀐 변수가 하나임을 명시"를 지킬 수 없어 "바뀐 변수는 둘"과 "따로 잰 값이 없다"를 바꾼 것 절과 한 문장으로 절에 적었다. 버린 대안: 옛 M2 묶음의 VAT만 바꾼 값을 쓰기(묶음이 달라 규칙 위반), 빼 보기 `A8-farcull`로 200m 밖 지우기의 몫을 보이기(최종 구성 위의 값이라 3단계와 바탕이 다르다. 09번 글의 몫).
- **결과 그림을 "인스턴스 수에 비례하는 일의 예상과 실제"로 그린다.** 기하 계산(200m 안 40%)으로 예상을 세우고 트레이스 타이머 다섯 개(EndVisualChanges, PrepareDistanceFieldScene, ShadowDepths, BasePass, RenderVelocities)와 대조했다. 실제가 모두 예상보다 커서 "VAT가 더한 일 + 비례하지 않는 몫"을 말할 수 있다. 버린 대안: 프레임 시간만으로 예상(프레임은 비례하지 않는 고정 비용이 섞여 예상선을 세울 근거가 없다).
- **트레이스 값은 730ff18의 트레이스(`Full1trace*`)에서 쓴다.** 01번 글과 같은 이유다(Insights를 띄워야 내보낼 수 있고 촬영 중이다, 두 빌드의 `Source/`·`Config/`가 같다). 측정 기록 "측정 기록의 변경"에 적었다.
- **Lumen 항목(backlog "글에 넣을 것")은 이 글에서 다루지 않는다.** 미니언을 Lumen 장면에서 빼는 일은 2단계부터 모든 Mass 단계에 같고(`ExcludeVisualsFromLumen`, measurement.md "미니언과 Lumen, 그림자"), 02번 글이 이미 다룬다. 3단계에서 바뀐 것이 아니다. Mass ISM 그림자 기본값(`bCastShadows`)도 같은 이유로 02번 글의 몫이고, 이 글은 측정 기록의 엔진 소스 표에만 적었다.
- **칼날 그림자(바운드) 이야기를 결과의 "화면" 소절에 넣는다.** VAT가 들여온 품질 문제이고 엔진 소스로 원인을 댈 수 있다(`AnimToTextureBPLibrary.cpp:245,290`). 공개 이슈는 찾지 못해 그렇게 적었다. 해결 코드는 `Source/HordeOptLab/`가 아니라 `Scripts/bake-vat.py`에 있어 그 블록을 보였다(바운드는 굽기 산출물의 속성이다).
- **틀렸던 가설은 "VAT 비용의 자리를 두 번 잘못 짚었다"로 쓴다.** 옛 묶음 값(M2 E2, M3 `diag-vt*`)은 방향만 쓰고, 최종 측정이 받치는 값(직렬 트레이스의 프로세서 0.451 / 0.569ms, 탑뷰 GPU 6.51 → 7.20ms)만 수치로 썼다. M3b 프래그먼트 분리는 A/B가 갈리지 않아 본문에 쓰지 않고 측정 기록에만 적었다. 탑뷰 GPU 증가도 두 변수가 섞인 값이라 "방향 근거"로만 썼다.
- **2단계의 인스턴스 수 "1만"은 설정에서 적는다.** 2단계는 엔진 프로세서라 `Lab/IsmBand*`가 없다. 3단계의 37.6%가 기하 예상 40%와 맞아 청크 필터가 건너뛴 청크가 적다고 보고, 확인하지 않은 것에 적었다.
- **00·02번 글을 상대 링크로 잇는다.** 두 폴더(`Posts/00-goals-and-method/`, `Posts/02-mass-logic/`)의 README가 체크아웃에 있다(아직 커밋 전). 폴더 이름은 두 글의 Worklog 절을 따랐다.

### 확인하지 못한 것(03)

- SVG와 움직이는 WebP를 화면에 그려 보지 않았다(브라우저 금지). 글자 겹침은 좌표와 글자 수로만 확인했다.
- 3단계 화면에서 피격·죽기 동작이 보이는지. 이미지 설명은 "걷는다"까지만 적었다.
- 최종 측정 패키지의 VAT 메시 바운드가 수정 뒤 값인지(굽기 산출물은 git 밖).
- VAT만의 비용, 예상과 실제의 차이의 분해, 2단계가 실제로 올린 인스턴스 수, `L3-vatcull` 1만 P1 r1의 게임 스레드 4.14ms.
- 최종 측정 빌드의 트레이스(`Full2trace-L2/L3-*`)로 다시 뽑은 값.

### 코디네이터용(03)

- 촬영이 끝나면 03번 글의 트레이스 값도 `Full2trace-L2-mass-*`, `Full2trace-L3-vatcull-*`(병렬, 렌더 스레드·GPU)와 `Full2traceser-*`(직렬, 게임 스레드)에서 다시 뽑아 `make-figures.py`의 `TRACE` 상수와 측정 기록 "트레이스 값" 표를 고친다. 01번 글과 같은 일이다.
- 00·02번 글의 폴더 이름이 바뀌면 이 글의 상대 링크 둘(`../00-goals-and-method/README.md`, `../02-mass-logic/README.md`)을 고친다.
- 루트 README "글" 표에 03번 행을 더한다(코디네이터 몫).
- backlog.md "글에 넣을 것"의 Lumen 항목은 02번 글이 맡는다고 적어 두면 좋다. 03번 글은 다루지 않는다(위 결정).
- engine-notes.md에 더할 것: `MassRepresentationFragments.h:309` `BufferHysteresisOnDistancePercentage = 10.0f`(표현 LOD 경계 히스테리시스 기본값), `MassVisualizationComponent.cpp:566-571`(커스텀 데이터 개수 검사, 3절에 있으나 줄 범위만), `AnimToTextureBPLibrary.cpp:33`(`AnimationToTexture`가 두 `SetBoundsExtensions` 호출을 감싼다).

## 04 액터 하이드레이션과 애니메이션 예산(2026-11-13)

`Posts/04-hydration-anim-budget/`를 쓴다. 수치는 최종 측정(Test 패키지 08e45f2)의 `L3-vatcull`·`L4-mass`·`L8-q4final`·`A8-hydrate`·`A8-budget`·`A8-fix`·`S8-col` 행과 `full-n-at-60fps.csv`에서만 만든다. 에디터, 빌드, 패키징, 게임, 브라우저, Insights는 띄우지 않았다.

### 진행(04)

- 조사 끝(성공). 읽은 것: STATUS.md, posting.md, ADR-0014, backlog.md, post-review.md "핵심 인사이트"·"04 액터 하이드레이션", post-material.md "04", measurement.md "지표"·"확정값"·"품질 계획에서 바뀌는 값", engine-notes.md 3·4·18·19·20절, references.md, code-tour.md, 01·03번 글과 03번 측정 기록·make-figures.py, `full-conditions.json` `L4`·`L8`·`A8-hydrate`·`A8-budget`·`A8-fix`·`S8-col`, Worklog Q6.md 트레이스 절. 소스: LabCrowdSubsystem.cpp `BuildEntityConfig`, LabActorSyncProcessor.cpp/.h, LabMinion.cpp(하이드레이션 액터), LabConfig.h(전환 대책 비트, 상한·거리), LabGameMode.cpp `ApplyVariantCVars`. 엔진: `AnimationBudgetAllocator.cpp:327-342,558-561`, `AnimationBudgetAllocatorParameters.h:19`, `AnimationBudgetAllocatorCVars.cpp:7,195`, `MassActorSpawnerSubsystem.cpp:252-255,327-328`, `MassRepresentationActorManagement.cpp:30-56`, `MassRepresentationProcessor.cpp:272,428`, `MassRepresentationFragments.h:200-211`. 다음은 초안.
- 그림 끝(성공). `make-figures.py`가 SVG 여섯 개를 만든다(요약, 원리 도식, 전환 도식, 예상과 실제, 최대 개체 수, 빼 보기). 값은 `summary.csv`·`full-n-at-60fps.csv`에서 읽고, 트레이스 값은 쓰지 않았다. SVG의 숫자 글자를 출력해 측정 기록 표와 대조했다. 다음은 초안.
- 초안 끝(성공). `README.md`: 요약, 시나리오 카드, 배경 지식(하이드레이션, 화면 크기 중요도, 표현 전환의 준비 대기, 611자), 문제, 원리(예상 포함), 바꾼 것, 결과(빼 보기 소절 포함), 한 문장으로. 본문은 표·코드·그림·제목·코드 위 파일 줄과 배경 지식을 빼고 약 3,840자다. Insights 이미지 자리는 주석으로 두었다. 다음은 측정 기록.
- 측정 기록 끝(성공). `measurements.md`: 빌드와 묶음, 4단계 구현 위치(파일:줄), 본문 수치 표(1만 3·4단계, 여유·예상·실제, 호드 총수별, 빼 보기, 설정값), 전환 대책의 경위, 코디네이터가 찍을 Insights 화면, HUD 차이, 그림, 엔진 소스, 발췌 줄, 확인하지 않은 것. 다음은 확인.
- 확인 끝(성공). 두 파일의 상대 링크가 모두 실제 파일을 가리킨다(주석 속 `images/insights-actor-tick.jpg`만 아직 없다). 코드 스팬 밖 `~`는 없다. 독자용 문장에 묶음 이름·조건 라벨·마일스톤 이름은 없다(이미지 경로와 측정 기록에만 있다). 본문 수치는 모두 측정 기록 표에 있다. 엔진 코드 발췌는 14줄이다. 확인 중 "하이드레이션 액터는 이동 컴포넌트를 쓰지 않는다"를 소스로 다시 봤다(부모 `ALabMinionPawn`이 CharacterMovement 틱을 끈다, `LabMinion.cpp:199-212`). 다음은 커밋.
- 커밋(성공). 08cabed `docs(posts): write post 04 on actor hydration and the animation budget`(`Posts/04-hydration-anim-budget/`, 이 절). Posts.md는 다른 글의 절이 체크아웃에 함께 있어 이 절만 인덱스에 올려 커밋했다(`git update-index --cacheinfo`). 이 줄은 뒤 커밋에 든다.

### 결정(04)

- **4단계를 "품질을 사는 단계"로 쓰고, 프레임 +0.249ms 뒤의 게임 스레드 +4.66ms를 핵심 그림으로 둔다.** 프레임 시간만 보면 거의 공짜로 보이지만 병목이 옮겨 가고 최대 개체 수가 3만 넘음에서 25,636으로 준다. 요약 그림(네 막대)과 예상·실제 그림(여유 4.33 대 실제 4.66)으로 보였다. 버린 대안: 프레임 시간 막대만 두기(병목 이동이 안 보인다).
- **예상은 "3단계 게임 스레드의 여유 ÷ 1단계 액터 단가"로 세운다.** 4단계 액터만의 단가를 미리 알 근거가 최종 측정에 없다. 1단계 1천(17.0µs)은 이동 컴포넌트가 든 상한이라 "상한"으로만 읽게 측정 기록에 적었다. 버린 대안: 옛 M3 묶음의 액터당 µs(묶음이 달라 규칙 위반), 트레이스 단가(옛 패키지).
- **예산·콜리전·전환 대책의 몫은 최종 구성의 빼 보기(`A8-budget`, `S8-col`, `A8-fix`, `A8-hydrate`)로 보이고, 바탕이 다르다는 것을 본문과 한계에 밝힌다.** 최종 측정에 4단계 위에서 하나만 끈 조건이 없다. 4단계가 한 단계에 표현과 딸린 설정 셋을 함께 넣은 것은 "바꾼 것" 첫 문단에 적었다. 버린 대안: 옛 M3 묶음의 E3 값(묶음이 다르다).
- **전환 대책 이야기는 수치 없이 "바꾼 것"에 넣고, 엔진 동작 둘(풀이 바로 보이게 함, 엔티티 변환으로 옮김)은 엔진 발췌 + 이 레포의 해결 코드로 보인다.** 공개 이슈는 찾지 못해 그렇게 적었다. 대책별 점수(M3 개발 빌드)와 Full1 `transition-dev` 점수는 묶음이 달라 쓰지 않고 측정 기록의 "전환 대책의 경위"에서 가리킨다. 전환 도식(`hyd-transition.svg`)을 새로 그렸다. 버린 대안: `Measurements/M3/shots/transition/`의 띠 이미지를 본문에 넣기(옛 빌드·옛 조건의 화면이라 "지난 구성의 화면은 최신 빌드에서" 규칙에 맞지 않는다).
- **대형 액터 건너뜀 비율은 1만(1.29%)과 2만(49.5%)만 쓴다.** 3만은 회차가 0.088\~0.427로 갈려 본문에 쓰지 않았다. 대형만 센다는 것을 본문("대형 액터의 애니메이션 갱신 가운데")과 측정 기록에 적었다.
- **GPU가 준 원인(−0.597ms)은 짐작을 본문에 쓰지 않고 "가르지 않았다"로 둔다.** 새 트레이스를 읽을 수 없고, 옛 트레이스 값은 금지다.
- **Lumen 항목과 저품질 액터 단계, 래그돌·정밀 판정 일반론은 다루지 않는다.** post-review.md가 "하"로 둔 재료이고 최종 측정에 대응 조건이 없다.
- **다음 글은 링크 없이 예고한다.** `../05-simulation-lod-entity-pool/`이 아직 없다.

### 확인하지 못한 것(04)

- SVG와 움직이는 WebP를 화면에 그려 보지 않았다(브라우저 금지). 글자 겹침은 좌표와 글자 수로만 확인했다. 원리 도식의 0\~30m 빨간 칸은 폭이 39px로 좁아 글자를 아래 줄에 따로 적었다.
- 4단계 화면에서 액터의 AnimBP 동작과 전환이 보이는지. 이미지 설명은 "액터다"까지만 적었다.
- 4단계 GPU −0.597ms의 원인, 액터 하나 12.5µs의 내역, `S8-col`에서 프레임(+2.01)이 게임 스레드(+1.54)보다 더 는 이유, `A8-budget` +1.75ms의 출처.
- 최종 측정 빌드의 트레이스(`Full2traceser-L4-mass-*`)로 읽은 값.

### 코디네이터용(04)

- Insights 화면을 찍는다: `Full2traceser-L4-mass-N10000-P1-r1.utrace`(비교 `Full2traceser-L3-vatcull-*`), 25\~75초, GameThread, 타이머 `USkinnedMeshComponent_TickComponent`, `USkeletalMeshComponent_CompleteParallelAnimationEvaluation`, `LabActorSyncProcessor`, `Sync_RenderingThread`. 읽을 값과 고칠 곳은 `Posts/04-hydration-anim-budget/measurements.md` "코디네이터가 찍을 Insights 화면"에 있다. 찍으면 README의 주석을 풀고 캡션의 ○○를 채운다.
- 05번 글이 생기면 04번 README 끝 문단("다음 글에서는…")에 `../05-simulation-lod-entity-pool/README.md` 링크를 단다. 08번 글이 생기면 "최종 구성은 … (08번 글)"에도 링크를 단다.
- 루트 README "글" 표에 04번 행을 더한다(코디네이터 몫).
- engine-notes.md에 더할 것: `AnimationBudgetAllocator.cpp:558,561,566`(예산 ÷ 평균 갱신 시간, 넘는 몫이 있을 때만 줄임), `:339`(정렬, 3절의 `:327`은 주석 줄), `AnimationBudgetAllocatorParameters.h:19` `BudgetInMs = 1.0f`(CVar 도움말도 Default 1.0), `MassActorSpawnerSubsystem.cpp:252-255`(풀에서 꺼내자마자 보이게 함, 4절에 줄 번호가 없다), `MassRepresentationActorManagement.cpp:47-56`(`TeleportActor` 기본 구현), `:58-61`(준비됨 기본 참), `MassRepresentationProcessor.cpp:275-298`(준비 대기 조건), `MassRepresentationFragments.h:202,211`(두 플래그 기본 거짓), `Character.cpp:78`(캡슐 34 × 88).

## 06 ISKM(2026-11-13)

`Posts/06-iskm/`를 쓴다. 수치는 최종 측정(Test 패키지 08e45f2)의 `L5-m4offpool`·`L6-m5bounds`·`L8-q4final`·`A8-iskm`·`A8-bounds` 행과 `full-n-at-60fps.csv`에서만 만든다. 에디터, 빌드, 패키징, 게임, 브라우저, Insights는 띄우지 않았다.

### 진행(06)

- 조사 끝(성공). 읽은 것: AGENTS.md, STATUS.md, posting.md, ADR-0014, backlog.md, post-review.md(핵심 인사이트 5, 06 항목, 07 ISKM vs VAT 사실 확인), post-material.md "07 ISKM vs VAT", measurement.md(최대 개체 수, 차이 판정), engine-notes.md 3·14·15절, references.md, ADR-0007·0010, D22, code-tour.md, Worklog M5.md(설계 조사, 구현, `m5bounds`, 트레이스 분석·검토, 고정 바운드 실험·판단, 깜빡임 진단)·Q3.md, `full-conditions.json`(`L5`·`L6`·`L8`·`A8-iskm`·`A8-bounds`), 03번 글 세 파일, `Source/HordeOptLab/`(LabIskmProcessor.cpp, LabCrowdSubsystem.cpp, LabConfig.h/.cpp, HordeOptLab.cpp, LabMinion.h), `Scripts/create-iskm.py`. 엔진 소스: `MassVisualizationComponent.cpp:756-800,812-827,849-866,1126-1160,1323-1340,1536-1545`, `InstancedSkinnedMeshComponent.cpp:468-503,1249-1253`, `InstancedSkinnedMeshComponentHelper.h:188-236`, `InstancedSkinnedMeshSceneProxy.cpp:76`, `AnimSequenceTransformProviderData.h:104-127`, `StaticMeshSceneProxy.cpp:340`, `PrimitiveSceneProxy.cpp:394`. 다음은 초안.
- 그림 끝(성공). `make-figures.py`가 SVG 다섯 개를 만든다(요약, 원리 도식, 문제, 6단계 − 5단계 변화, 빼 보기). 값은 `summary.csv`·`full-n-at-60fps.csv`에서 읽고, 변화 그림의 "흔들림 안" 판정도 `_min`·`_max` 열로 계산한다. 트레이스 값은 쓰지 않았다. 반올림은 `Decimal` 반올림, 음수는 U+2212다. SVG의 숫자 글자를 측정 기록 표와 대조했다. 다음은 초안.
- 초안 끝(성공). `README.md`: 요약, 시나리오 카드, 배경 지식(ISKM, ASTPD와 트랙, 컴포넌트 바운드, 619자), 문제, 원리(엔진 발췌와 예상), 바꾼 것(Lab 코드 넷, 엔진 트랙 문제 소절), 결과(Insights 자리 주석, 빼 보기 소절, 화면 소절), 틀렸던 가설(세 문단), 한 문장으로. 본문은 표·코드·그림·제목·주석과 배경 지식을 빼고 약 4,260자다(같은 셈법으로 03번 글은 4,002자). 처음 초안이 5,860자라 줄였다. 다음은 측정 기록.
- 측정 기록 끝(성공). `measurements.md`: 빌드와 묶음, 6단계 구현 위치, 본문 수치 표(1만 네 값, 5단계 호드 총수별, 6단계 − 5단계 변화와 판정, 최대 개체 수와 탑뷰, 빼 보기, 설정·코드 값), 엔진 소스(파일:줄), 공개 이슈, 발췌 줄, 코디네이터가 찍을 Insights 화면, 그림, HUD 차이, 경위의 근거, 확인하지 않은 것. 다음은 확인.
- 확인 끝(성공). 두 파일의 상대 링크가 모두 실제 파일을 가리킨다(주석 안의 `images/iskm-insights-distance-field.jpg`만 아직 없다). 코드 스팬 밖 `~`는 없다. 독자용 문장에 묶음 이름·조건 라벨·마일스톤 이름은 없다(이미지 경로에만 있다). 본문 수치는 모두 측정 기록 표에 있다. 엔진 코드 발췌는 25줄, Lab 코드 발췌는 블록마다 15줄 이하다. 측정 기록 표는 다섯 열 이하다. 다음은 커밋.
- 커밋(성공). ccd1578 `docs(posts): write post 06 on ISKM and fixed bounds`(`Posts/06-iskm/`, 이 절). 이 줄은 뒤 worklog 커밋에 든다. 푸시하지 않았다.

### 결정(06)

- **사다리 6단계가 두 변수(ISKM, 고정 바운드)를 함께 바꾼 것을 본문에 밝히고, 최종 구성의 빼 보기 두 행으로 가른다.** `A8-iskm`(VAT)과 `A8-bounds`(기본 바운드 ISKM)의 차이는 표현 하나라(VAT에는 고정 바운드를 넣을 컴포넌트가 없다, `LabIskmBounds` 0) 한 변수 비교로 썼다. 버린 대안: 옛 M5 묶음의 `m5iskm`(기본 바운드) 값을 쓰기(묶음이 다르다), 빼 보기를 09번 글에 미루기(06의 틀렸던 가설이 최종 측정의 근거를 잃는다).
- **결과 그림을 "6단계 − 5단계의 호드 총수별 변화"로 그린다.** GPU는 N이 클수록 더 줄고 게임 스레드는 N이 클수록 더 느는 것이 이 단계의 핵심이고, 최대 개체 수가 30,000 이상에서 29,799로 준 이유도 이 그림이 보인다. 흔들림보다 작은 칸(5천·1만 게임 스레드, 2만 프레임)은 점선으로 그렸다. 버린 대안: 03번 글처럼 최대 개체 수 선 그림(값 하나만 갈려 변화 그림보다 정보가 적다), 트레이스 타이머의 예상과 실제(최종 측정 트레이스를 읽을 수 없다).
- **예상은 "프레임은 많아야 5단계의 프레임 − 게임 스레드 간격(0.321ms)만큼 준다"로 세운다.** 트레이스 없이 최종 측정 값만으로 세울 수 있는 예상이고, 실제 −0.148ms와 대조된다. 버린 대안: 디스턴스 필드 몫으로 GPU 감소를 예상(최종 측정 트레이스 값이 없다).
- **ISKM의 이점은 D22대로 GPU와 기능으로 적고, 프레임만 보면 VAT가 0.206ms 빠르다는 것을 결과에 그대로 적는다.** 3만에서 프레임이 0.615ms 길고 최대 개체 수가 준 것도 숨기지 않았다(한 문장으로의 세 번째 줄).
- **틀렸던 가설은 "ISKM이 GPU를 줄이면 프레임도 줄 줄 알았다"로 쓴다.** backlog의 방향(GPU는 줄었는데 프레임이 늘었고, 바운드를 고정하자 뒤집혔다)을 옛 M5 묶음·트레이스의 방향만으로 쓰고, 최종 측정이 받치는 값(기본 바운드 ISKM 대 VAT: GPU −1.22, 프레임 +0.977ms)만 수치로 썼다. 최종 측정에서는 "뒤집힘"이 1만까지만 성립하고 2만부터 다시 갈린다는 점을 고친 설명에 넣었다. posting.md가 적은 06의 가설(전환 우위가 다시 나오지 않았다)은 마지막 문장 하나로 받았다(Development 전환 점수는 옛 빌드의 참고값이라 수치 없이).
- **엔진 트랙 문제 둘은 "바꾼 것" 아래 소절로 둔다.** 둘 다 이 단계의 코드(`ULabIskmProcessor`의 트랙 다시 쓰기, 엔티티 풀)를 정한 이유라서다. 풀은 05번 글 몫이라 문제, 엔진 줄, 풀 재사용 블록 하나만 보였다. 엔진 발췌 하나(8줄)로 두 문제(클립 번호 조건, 돌려받는 곳 하나)를 함께 보였다.
- **공개 이슈 링크는 달지 않았다.** references.md "포럼 스레드"의 "Mass crowds using SkinnedMeshInstance leak GPU memory"는 제목만 있고 주소가 없다. engine-notes.md에도 URL이 없다. 링크를 지어내지 않고 본문에 "공개 이슈를 찾지 못했다"고 적고, 측정 기록에 포럼 글 제목과 엔진 파일·줄을 적었다. 웹 검색은 하지 않았다.
- **모션 블러 번짐은 이 글의 "결과 > 화면" 소절에 짧게 넣는다.** ADR-0007이 ISKM 글(당시 번호 07)에 "개발 빌드에서 관찰한 현상과 원인 미확정"을 적기로 정했다. 원인 미확정, Lab의 `PrevTransform`은 정상, 모션 블러 기본값 유지를 적고 캡처는 측정 기록에 링크했다(Development 캡처라 본문 그림으로 넣지 않았다). 37\~66초 구간 한정은 측정 기록에만 적었다.
- **본문 수치는 유효 숫자 세 자리로 쓴다.** 그래서 브리프의 −0.21 / +0.77 / −0.33ms는 −0.206 / +0.771 / −0.327ms로 썼다(posting.md "수치는 유효 숫자 세 자리").
- **디스턴스 필드가 빠지는 원리는 엔진 소스로만 적는다.** 정적 메시 프록시만 표현을 켜고(`StaticMeshSceneProxy.cpp:340`, 기본값 `PrimitiveSceneProxy.cpp:394`), 그림자를 그리면 Lumen에서 빼도 장면에 남는다(`SceneManagement.h:1069-1082`, `LabMinion.h:39-42`). 크기는 Insights 화면 자리로 남겼다.

### 확인하지 못한 것(06)

- 최종 측정 트레이스(`Full2trace-L5/L6`, `Full2traceser-L5/L6`)의 값. 디스턴스 필드가 실제로 얼마나 빠졌는지, 2만·3만에서 게임 스레드가 늘어난 타이머.
- `A8-bounds`의 프레임 증가 가운데 게임 스레드 밖 몫(0.472ms)이 프레임 끝 바운드 계산인지(트레이스 없음).
- 옛 고정 바운드 묶음(20k에서 게임 스레드가 VAT와 같음)과 최종 측정(2만 +0.545ms)이 갈린 이유.
- Test 패키지의 ASTPD 애니메이션 바운드 유무(로그 없음. 없어도 메시 바운드로 같은 반복이라 본문 설명은 같다).
- 모션 블러 번짐의 원인과 최종 측정 빌드에서도 나는지.
- SVG와 움직이는 WebP를 화면에 그려 보지 않았다(브라우저 금지). 글자 겹침은 좌표와 글자 수로만 확인했다.

### 코디네이터용(06)

- Insights 화면 하나를 찍어 넣는다: `Full2trace-L5-m4offpool-*`와 `Full2trace-L6-m5bounds-*`(병렬)의 렌더 스레드 `PrepareDistanceFieldScene`. 자세한 것은 `Posts/06-iskm/measurements.md` "코디네이터가 찍을 Insights 화면". 01·03번 글처럼 `Scripts/trace-export.ps1`로 값을 뽑으면 README의 주석 처리된 이미지 자리와 캡션의 `OO`를 채우고, 원하면 "원리"의 예상에 디스턴스 필드 몫 한 줄을 더한다.
- 05번 글이 커밋되면 이 글의 "5단계(05번 글)"와 "(05번 글)" 두 곳에 `../05-simulation-lod-entity-pool/README.md` 링크를 단다(폴더는 체크아웃에 있으나 커밋 전이다). 07번 글이 생기면 끝 문단("다음 글에서는…")에 `../07-niagara-crowd/README.md` 링크를 단다.
- 루트 README "글" 표에 06번 행을 더한다.
- engine-notes.md 고칠 것: 3절 "ISKM 블렌딩" 행의 파일을 `Engine/Classes/Animation/AnimSequenceTransformProviderData.h:104-127,152-174`로(지금은 `InstancedSkinnedMeshComponent.h`, post-review.md도 지적). 더할 것: `InstancedSkinnedMeshComponent.cpp:495-497`(프레임 끝 `FlushChanges` → `UpdateBounds`), `InstancedSkinnedMeshComponentHelper.h:202,227-235,245-251`(`bFastBuild = false`, 인스턴스 반복, 애니메이션 바운드가 없을 때의 메시 바운드 반복), `MassVisualizationComponent.cpp:777-787,868-878`(UAF 인계용 재생 상태 읽기, 끌 수 없음), `PrimitiveSceneProxy.cpp:394`·`StaticMeshSceneProxy.cpp:340`·`SceneManagement.h:1069-1082`(디스턴스 필드 장면 조건. 11절 "스플라인" 오류와 같은 절), `AnimToTextureBPLibrary.cpp:851-868`(LOD 하나만 옮김), `InstancedSkinnedMeshSceneProxy.cpp:76`(GPU 메시 LOD 선택 조건).
- references.md의 포럼 글 세 개에 주소를 더하면 06번 글 본문의 "공개 이슈를 찾지 못했다"를 링크로 바꿀 수 있다.
- STATUS.md "측정 결과"에 적을 만한 것: `L6` 2만·3만에서 게임 스레드가 `L5`보다 +0.545 / +1.13ms(흔들림보다 크다). 옛 M5 고정 바운드 묶음의 "게임 스레드가 같다"와 다르다.

## 05 Simulation LOD와 엔티티 풀(2026-11-13)

`Posts/05-simulation-lod-entity-pool/`를 쓴다. 수치는 최종 측정(Test 패키지 08e45f2)의 `L4-mass`·`L5-m4offpool`·`L8-q4final`·`A8-simlod`·`A8-pool` 행, `full-n-at-60fps.csv`, 16분 실행 묶음(`Measurements/Pool1/timeline.csv`)에서만 만든다. 에디터, 빌드, 패키징, 게임, 브라우저, Insights는 띄우지 않았다.

### 진행(05)

- 조사 끝(성공). 읽은 것: AGENTS.md, STATUS.md, posting.md, backlog.md, post-review.md(핵심 인사이트 4·9, 06 Simulation LOD 사실 확인), post-material.md "06 Simulation LOD", measurement.md("확정값", "총괄 묶음 규약", "엔티티 풀 묶음 규약"), engine-notes.md 3·5절, references.md, code-tour.md, plan.md M4·M5, Q6.md "Full2 재측정"·"Pre6 촬영과 풀 끔 떨림 확인", 03번 글 세 파일, `full-conditions.json`(`L4`·`L5`·`A8-simlod`·`A8-pool`). 소스: HordeOptLab.cpp, LabConfig.h/.cpp(`SimLod`, `UsesSimLod`, `bEntityPool`), LabMassProcessors.cpp(접근·Off 이동·관찰자·상태 프로세서), LabCrowdSubsystem.cpp(`SpawnEntities`, `RecycleAndRespawn`, `ReinitPooledEntity`). 엔진: `MassVisualizationComponent.cpp:754-775,812-827,849-866,1126-1158,1537-1547`, `MassSkinnedMeshRepresentationTypes.h:700-706`, ISKM `RemoveInstance` 호출자 검색(정지 표현 프로세서뿐). 다음은 초안.
- 그림 끝(성공). `make-figures.py`가 `summary.csv`·`full-n-at-60fps.csv`에서 SVG 다섯 개(`simlod-summary`, `-problem`, `-principle`, `-result`, `-ablation`)를 만든다. 상수 없음. 글자 16px 이상, 흰 배경. 그림 속 글자를 CSV 값과 대조했다(밴드 개수 5,996.5는 반올림을 반올림-위로 고쳐 5,997). 엔티티 풀 그림은 README의 `entity-pool.svg`를 상대 경로로 쓴다.
- 초안 끝(성공). README.md: 요약, 시나리오 카드, 배경 지식 셋(Simulation LOD, 가변 틱과 청크 필터, 엔티티 풀), 문제, 원리(+예상), 바꾼 것, 결과(+엔티티 풀 소절), 틀렸던 가설, 한 문장으로. 본문(배경 지식·코드·그림·제목 제외) 약 4,300자(공백 포함). 엔진 코드 발췌 22줄.
- 측정 기록 끝(성공). measurements.md: 본문·그림의 모든 수치에 조건·열·세 회차, 계산식, 16분 실행의 행 라벨과 열, 엔진 소스 위치, 발췌 줄 표, 코디네이터가 찍을 Insights 화면, 확인하지 않은 것.
- 확인 끝(성공). README·measurements의 상대 링크가 모두 실제 파일을 가리킨다(주석 속 Insights 자리 제외). 코드 스팬 밖 `~`는 모두 `\~`다. 독자용 문장에 묶음 이름·조건 라벨·마일스톤 이름이 없다(링크 주소에만 있다). 04·06번 README가 있어 링크했다. 엔진 발췌 22줄, 레포 발췌는 블록마다 15줄 이하.
- 커밋 끝(성공). 28d028a(글 폴더와 이 절). 푸시하지 않았다.

### 결정(05)

- **엔티티 풀 그림은 README 그림을 다시 쓴다.** 대안: `timeline.csv`에서 이 글 전용 그림을 만든다. 근거: README 그림이 같은 세 지표(트랙, GPU 메모리, 프레임)를 같은 조건 문장과 함께 이미 보이고, 같은 값의 그림이 두 벌이면 다시 잴 때 둘을 고쳐야 한다. 단점: 그림의 GPU 메모리 5,062(5,062.5를 반올림-짝수로)와 본문 5.06GB의 자릿수가 다르다. 측정 기록에 적었다.
- **5단계에서 풀이 하는 일과 최종 구성에서 필요한 이유를 나눠 썼다.** 5단계의 30\~200m는 VAT라 트랙이 없어 풀이 막을 누수가 없다고 적고, 풀을 넣은 이유는 "다음 단계에서 표현 하나만 바꾸기"(plan.md M5 결정 1, `m4offpool` 기준점)로 적었다. 16분 실행이 최종 구성(ISKM)이라는 것을 본문에 밝혔다.
- **공개 이슈 링크는 넣지 않았다.** references.md에 포럼 스레드 제목("Mass crowds using SkinnedMeshInstance leak GPU memory")만 있고 주소가 없다. 본문은 "공개 이슈의 주소는 확인하지 못했다"와 엔진 소스 발췌로 대신했다. 대안: 제목을 본문에 적는다. 버린 이유: 주소 없이 제목만 적으면 독자가 확인할 수 없다.
- **예상은 개체 수로만 세웠다.** 엔진 이동·조향이 처리하는 걷는 개체가 39%로 준다(5단계에서 센 밴드 개수). ms 예상은 트레이스 값이 필요하고, 옛 트레이스(`Full1*`) 값은 쓰지 않는 규칙이라 Insights 자리만 주석으로 두었다.
- **4 → 5단계 차이에 엔티티 풀이 섞인 것을 밝히고, 빼 보기 두 행으로 보완했다.** 풀만 뺀 5단계는 최종 측정에 없다. `A8-simlod`(+1.04ms)와 `A8-pool`(범위 안)을 한 그림에 두었다. 다른 구성(최종)이라 사다리 그림과 잇지 않았다.
- **문제 그림은 선 대신 막대로 그렸다.** 5천에서 세 값(4.53, 3.48, 5.73)이 가까워 선 그림의 값 글자가 겹친다. 막대는 값을 막대 끝에 적어 겹치지 않는다.
- **탑뷰의 게임 스레드 증가(+0.454ms)를 한계에 한 줄로 적었다.** 최종 측정의 5단계 결과라 빼면 유리한 값만 고른 것이 된다. 원인은 모른다고 적었다.
- **본문에 쓰지 않은 것:** 관찰자 `ULabOffMoveEnterObserver`, 저격·상태 타이머에 필터를 걸지 않은 이유의 사례, 풀 켬의 0.265ms 증가, 트랙이 두 배에서 멈추는 이유의 추정(컴포넌트 둘), 풀 끔 떨림. 측정 기록에만 있다.

### 확인하지 못한 것(05)

- 엔진 이동·조향 넷이 실제로 39% 근처로 줄었는지(최종 측정 빌드의 트레이스를 내보내지 않았다).
- 5단계에서 엔티티 풀만의 몫.
- 풀 켬 트랙이 약 2만에서 멈추는 이유, 풀 끔 13분째 GPU 메모리 계단의 원인, 풀 끔의 시간에 따른 프레임 변동(느린 쪽).
- 탑뷰에서 5단계 게임 스레드가 는 원인, 1만 P1 GPU +0.036ms의 원인.
- 4·5단계 WebP와 SVG를 화면에 그려 보지 않았다(브라우저 금지). 글자 겹침은 좌표와 글자 수로만 확인했다.
- 공개 이슈의 주소.

### 코디네이터용(05)

- Insights 화면: `Full2traceser-L4-mass-N10000-P1-r1.utrace`와 `Full2traceser-L5-m4offpool-N10000-P1-r1.utrace`, 25\~75초, 게임 스레드, 엔진 이동·조향 넷(`UMassApplyForceProcessor`, `UMassApplyMovementProcessor`, `UMassSteerToMoveTargetProcessor`, `UMassSmoothOrientationProcessor`)의 프레임당 합과 `LabOffMoveProcessor`. 찍어서 `Posts/05-simulation-lod-entity-pool/images/insights-movement-l4-l5.jpg`로 넣고, README "결과"의 주석을 풀어 캡션의 X.XX를 채운다. 값은 measurements.md "코디네이터가 찍을 Insights 화면"에 행으로 더한다.
- 공개 이슈: references.md의 포럼 스레드 "Mass crowds using SkinnedMeshInstance leak GPU memory"의 주소를 찾으면 references.md에 적고, 05번(그리고 06번) 글의 "공개 이슈의 주소는 확인하지 못했다" 문장을 링크로 바꾼다.
- 06번 글의 폴더가 아직 커밋되지 않았다(이 커밋 때 파일은 있다). 이름이 바뀌면 05번 README의 링크 둘(`../06-iskm/README.md`)을 고친다.
- 루트 README "글" 표에 05번 행을 더한다.
- `Docs/figures/readme/entity-pool.svg`를 바꾸면 05번 measurements.md "16분 실행"의 그림 값 문장도 고친다.
- engine-notes.md에 더할 것: ISKM 트랙 해제 입력(`EntitiesRequiringRemoval`)을 채우는 경로가 `MassStationaryISMSwitcherProcessor.cpp:127`뿐이고 그 쿼리가 `FMassStaticRepresentationTag`를 요구한다(`:30`). `DeallocateTrack` 호출은 MassGameplay 전체에서 `MassVisualizationComponent.cpp:1156` 하나다. 3절 행에 줄 번호를 보탤 수 있다. 엔진 조향 프로세서의 Off 쿼리 줄은 `MassSteeringProcessors.cpp:86`이다(5절은 85-87로 적었다, 같은 범위).

## 01·03 트레이스 값 재추출(2026-11-13)

- 끝(성공). 최종 측정 빌드(08e45f2)의 트레이스 `Full2traceser-L1~L3`, `Full2trace-L1~L3`에서 `Scripts/trace-export.ps1 -Command TimerStatistics`(스레드 `GameThread`, `RenderThread*`, `GPU0-Graphics0`, 구간 25\~75초)로 다시 뽑았다. 같은 방법으로 옛 `Full1*` 내보내기를 다시 계산하면 옛 값과 일치했다.
- 01번: 446.43 / 323.49 / 27.24 / 14.70 → 446.43 / 322.20 / 27.31 / 15.71(프레임 112). 합 41.94 → 43.02, 나머지 81.00 → 81.21, 남는 몫 404.49 → 403.41, 72.46% → 72.17%, 9.39% → 9.64%, 개체당 32.35 → 32.22µs. `make-figures.py`, `measurements.md`, `README.md`를 고쳤다.
- 03번: 프레임 수(직렬 5,038 / 6,412, 병렬 5,055 / 6,423) 기준으로 EndVisualChanges 2.47/1.21 → 2.52/1.23, 디스턴스 필드 2.61/1.35 → 2.65/1.36, 그림자 2.53/1.62 → 2.51/1.66, 색과 재질 1.41/0.83 → 1.40/0.83, 움직임 벡터 1.46/0.68 → 1.43/0.68, VAT 프로세서 0.451 → 0.458, 엔진 ISM 프로세서 0.569 → 0.573ms. 다섯 칸 모두 실제가 예상보다 크다는 결론은 그대로다.
- 남은 일: 01번 `images/insights-gamethread.jpg`와 캡션(448 / 322ms / 10,048회)은 옛 트레이스의 값이다. 코디네이터가 새 트레이스로 다시 찍는다.

## 07 Niagara 배경 무리(2026-11-13)

하위 에이전트(Opus)가 `Posts/07-niagara-crowd/`를 쓴다. 수치는 최종 측정(Test 패키지 08e45f2)에서만 만든다.

### 진행

- 조사 끝(성공). `full-conditions.json`의 `L6`→`L7`은 `LabCrowdFx` 0→1 하나만 다르다. 7단계는 원거리 그림자 켬(`LabFarShadow=1`)이라 엔티티 구동 입자가 그림자를 그린다(`SetupCrowdFx`, Q2.md "Q2c"). 다음은 트레이스 내보내기와 초안.

## 08 품질 올리기(2026-11-13)

하위 에이전트(Opus)가 `Posts/08-quality/`를 쓴다. 수치는 최종 측정(Test 패키지 08e45f2)의 `L7-q2crowd`·`L8-q4final`·`A8-farshadow`·`A8-fix`·`A8-hydrate` 행, `full-n-at-60fps.csv`, 참고 측정 `Ref1`(1280×720)의 `L8-q4final-aa`·`A8-hydrate` 행, 전환 점수 `Measurements/Full1/transition-dev/runs.csv`(Development 참고)에서만 만든다. 에디터, 빌드, 패키징, 게임, 브라우저, Insights는 띄우지 않았다.

### 진행(08)

- 조사 끝(성공). `L7`→`L8`은 `LabActorCap` 500→1000, `LabHydrateM` 30→40, `LabFarShadow` 1→0 세 값이 다르다. 80m 밖 그림자 스위치는 80\~200m ISKM과 200m 밖 엔티티 구동 입자를 함께 끈다(`LabCrowdSubsystem.cpp:168-200`, `LabGameMode.cpp:311`). 탑뷰(높이 100m)에는 40m 안 개체가 없어 `L7`→`L8` 차이가 그림자 하나다. 다음은 그림과 초안.
- 자료 확인 끝(성공). `L7`→`L8` 1만 P1: 프레임 p50 7.785→7.783(범위 안), Game 7.628→7.648(범위 안), GPU 6.096→5.720, 액터 373.79→458.13, 대형 건너뜀 0.0065→0.2073. 탑뷰 프레임 5.260→4.918(GPU 병목). `Ref1` `A8-hydrate` −3.732ms. 다음은 그림.

## 02·04·05·06 트레이스 값 채우기(2026-11-13)

- 02번 끝(성공). 직렬 `Full2traceser-L2-mass`의 게임 스레드(구간 25\~75초, 프레임 5,038): 프레임 9.92ms, Mass PrePhysics 3.01, `EndVisualChanges` 2.52, 가장 긴 Processor `MassSmoothOrientationProcessor` 0.93, `MassUpdateISMProcessor` 0.57ms. `UCharacterMovementComponent_TickComponent`는 없다. README의 주석 자리를 값 문장으로 바꿨다. 본문 주장은 그대로다.
- 04번 끝(성공). 직렬 `Full2traceser-L4-mass`(프레임 5,674)와 `L3-vatcull`(6,412) 게임 스레드: 메시 틱 1.05ms(257번), 평가 완료 0.83, `LabActorSyncProcessor` 0.51, `Sync_RenderingThread` 3단계 2.75 → 4단계 0.005, `Tick_Engine` 4.68 → 8.37ms. 메시 틱 호출 수 257이 액터 평균 373보다 작아 본문에 "액터 수와 같다"를 쓰지 않았다(원인 미확인).

## 09 총괄(2026-11-13)

하위 에이전트(Opus)가 `Posts/09-summary/`를 쓴다. 수치는 최종 측정(Test 패키지 08e45f2)의 `Measurements/Full2/summary.csv`·`runs.csv`, `Docs/figures/full2/full-n-at-60fps.csv`, 엔티티 풀 `Measurements/Pool1/timeline.csv`, 참고 측정 `Measurements/Ref1/summary.csv`(한 문장)에서만 만든다. 에디터, 빌드, 패키징, 게임, 브라우저, Insights는 띄우지 않았다.

### 진행(09)

- 조사 끝(성공). 사다리 여덟 행, 빼 보기 열 행과 `S8-col`, 최대 개체 수 CSV, `Pool1`, `Ref1` 값을 CSV에서 다시 읽어 브리프의 값과 맞음을 확인했다. README 그림 여섯 개의 글자를 읽었다(`ablation.svg`에는 `S8-col`이 없다). 다음은 초안.
- 05번 끝(성공). 직렬 `Full2traceser-L4-mass`(프레임 5,674)와 `L5-m4offpool`(6,286) 게임 스레드: 엔진 이동·조향 넷의 합 1.68 → 0.67ms(39.7