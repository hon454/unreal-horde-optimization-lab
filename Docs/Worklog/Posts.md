# 글 작업 기록

글 하나에 절 하나다. 단계(조사, 초안, 그림, 측정 기록, 확인, 커밋)가 끝날 때마다 한 줄을 더한다.

## 01 액터 군중 기준선(2026-11-12)

- 조사 끝(성공). `Measurements/Full1/summary.csv`·`runs.csv`의 `L1-full`·`S1-budget`·`S1-uro` 행, `full-n-at-60fps.csv`, Q6 트레이스 표를 다시 읽었다. 트레이스 내보내기 CSV(`%TEMP%\horde-trace\F1-traceser-L1-full-GT.csv`)에서 446.43 / 323.49ms를 다시 확인했다. 새로 확인한 엔진 사실: 1만에서는 월드 델타가 0.4초로 잘린다(`BaseGame.ini:199` `MaxUndilatedFrameTime=0.4`, `WorldSettings.cpp:334-343`). 그래서 이동 컴포넌트가 50ms씩 여덟 번, 곧 상한 8회까지 나눈다.
- 초안 끝(성공). `Posts/01-actor-baseline/README.md`. 틀은 요약, 시나리오 카드, "쓰인 것" 다섯 행, 원리(Mermaid 도식, 하위 스텝, 예상), 문제(트레이스 그림, Insights 자리, 개체당 Game 표), 결과(N@16.7ms 797, URO·Budget 표), 한 문장으로. URO가 Budget보다 덜 준 이유는 "확인하지 않았다"로 두었다(post-review의 미확인 설명을 쓰지 않음).
- 그림 끝(성공). `images/actor-gamethread-breakdown.svg`(446ms 구성, 16.7ms 막대를 같은 축척으로), `images/actor-frame-vs-n.svg`(프레임 p50·p95 로그 축 + 개체당 Game µs). 생성 스크립트는 저장소 밖 스크래치에 두었다(값을 손으로 넣은 정적 SVG). 코디네이터가 넣은 `insights-gamethread.jpg`를 보고 옆 표를 그 화면의 값(한 프레임 447ms, CharacterMovement 321.66ms, 10,048회)과 구간 평균으로 고쳤다.
- 측정 기록 끝(성공). `Posts/01-actor-baseline/measurements.md`: 빌드와 묶음, 본문 수치 표(세 회차 값), 계산식, 그림, 엔진 소스(0.4초 델타 상한 포함), 경위 근거(M0.md), 확인하지 않은 것.
- 확인 끝(성공). 상대 링크와 이미지 경로 13개가 모두 실제 파일을 가리킨다(`insights-gamethread.jpg`도 코디네이터가 넣어 있다). 본문 산문은 표·코드·이미지·제목을 빼고 2,446자(공백 포함). 본문 수치를 `summary.csv`·`runs.csv`·트레이스 내보내기 CSV와 다시 대조했다. 본문에 `Full1`, 일정·면접 표현 없음. 코드 스팬 밖 `~`는 `\~`.

### 확인하지 못한 것

- 하위 스텝 횟수(1만 8회)는 엔진 소스와 0.4초 델타 상한으로 계산했고, 실행에서 세지 않았다. 끄고 다시 잰 실험도 없다.
- Budget·URO 실행의 트레이스가 없어 Budget이 줄인 41.4ms의 내역과 URO가 덜 줄인 이유를 모른다.
- Insights 스크린샷이 병렬·직렬 어느 트레이스인지(측정 기록에 "코디네이터 확인"으로 남김).
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
