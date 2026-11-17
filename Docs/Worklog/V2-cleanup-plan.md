# v2 레포지토리 정리 조사

main 병합 전 정리의 후보를 모은 조사다(2026-11-17, 작업 브랜치 `v2`, HEAD 9bfc558). 이 조사는 읽기만 했다. 지우거나 옮기거나 고친 파일은 없다. 무엇을 할지는 사용자가 묶음마다 정한다.

## 사용자 결정

조사 뒤에 사용자가 묶음마다 정한 것이다(2026-11-17).

| 묶음 | 결정 | 근거 |
| --- | --- | --- |
| 1 `v1` 측정 스크린샷 | 건드리지 않는다 | 사용자 조건: 태그 `v1`으로 돌아갔을 때 자료가 모두 유지돼야 한다. 109장은 태그 `v1`에 바뀌지 않은 채 있으나, 원격 LFS에서 내려받아지는지는 확인하지 못했다 |
| 2 단계 화면 | 그대로 둔다 | W14 결정 3("다시 인코딩하지 않는다")을 유지한다 |
| 3 측정 묶음 | 건드리지 않는다 | 1을 두면 남는 것은 CSV 1.6MB뿐이고 가이드와 ADR이 인용한다 |
| 4 옛 그림 | 그대로 둔다 | 얻는 것이 폴더 겉모습뿐이고 ADR-0015가 인용한다 |

## 전제

- 기록 다시 쓰기와 강제 푸시는 하지 않는다. 그래서 정리의 효과는 둘이다. (1) 새 클론이 내려받는 LFS 크기가 준다. (2) 레포를 여는 사람이 보는 모습이 깔끔해진다.
- git 객체(LFS가 아닌 파일)는 지워도 기록에 남는다. 새 클론의 `.git` 크기는 줄지 않는다. 텍스트 파일(CSV, SVG, MD, 스크립트)을 지우는 효과는 (2)뿐이다.
- LFS는 체크아웃한 커밋의 파일만 내려받는다. main에 `v2`를 병합하면 새 클론은 병합 시점의 LFS 파일만 받는다. 아래의 "LFS 감소"는 그 양이다.
- `v1`의 결과는 태그 `v1`(62146e2)에 남는다. `v1` 묶음 25개, 옛 그림, `v1` 스크립트는 모두 그 태그에 있다(`git ls-tree v1`로 확인).
- 크기는 작업 트리의 파일 크기(1MB = 10^6 바이트)다. LFS 파일 183개가 모두 체크아웃돼 있다(`git lfs ls-files`의 `*`).

## 현황

| 항목 | 값 | 근거 |
| --- | --- | --- |
| 추적 파일 | 767개, 169.6MB | `git ls-files` |
| 그중 LFS | 183개, 161.4MB | `.gitattributes`의 확장자 |
| `Measurements/` | 416개, 105.9MB. 묶음 56개와 `README.md` | `v1` 묶음 25개 85.6MB(LFS 84.0MB), `v2` 묶음 31개 20.3MB(LFS 18.8MB) |
| `Docs/figures/` | 53개, 54.6MB(LFS 54.3MB) | 그중 단계 화면 8장이 54.2MB |
| `Docs/Worklog/` | 28개, 2.0MB | `V2.md` 670,823바이트 |
| GitHub `size` | 57,039KB | `gh api repos/hon454/unreal-horde-optimization-lab --jq .size`. 이 값은 git 객체의 크기이고 LFS를 넣지 않는 것으로 안다(GitHub 문서로 확인하지 않았다) |

## 요약

| 묶음 | 후보 | 크기(LFS 감소) | 끊어지는 링크 | 추천 | 이유 |
| --- | --- | --- | --- | --- | --- |
| 1 `v1` 측정 스크린샷 | `v1` 묶음 15개의 `shots/` 이미지 109장 | 84.1MB(84.1MB) | 57개(Worklog 51, `measurement.md` 2, `post-material.md` 2, M3 안 README 2) | **지운다** | 공개 자료가 한 장도 쓰지 않는다. 새 클론 LFS의 52%다. 태그 `v1`에 남는다 |
| 2 단계 화면 | `Docs/figures/steps/` 웹피 8장 | 54.2MB | 0(줄이기만 하면) | **줄인다** | 공개 자료가 쓴다. 다시 인코딩하면 장당 7.5MB를 2\~3MB로 줄일 수 있다(추정). W14 결정 3("다시 인코딩하지 않는다")을 다시 정해야 한다 |
| 3 측정 묶음 | `v1` 묶음 25개 통째(1의 이미지 포함) | 85.6MB(84.0MB) | 57개(1과 같다) | **이미지만 지우고 CSV는 둔다** | CSV는 1.6MB뿐이다. 가이드와 ADR이 `Full1`\~`Full3`, `M0`, `M5`, `Q1`, `Ref1`, `Pool1`, `Budget1`을 이름으로 인용한다. 승인된 ADR 본문은 고칠 수 없다 |
| 4 옛 그림 | `full1`, `full2`, `full3`, `budget1`, 최상위 `n-at-60fps.*`·`n-scaling-P1.svg` | 0.13MB(0) | 0 | **지운다**(가볍게) | 공개 자료와 가이드가 링크하지 않는다. LFS 효과는 없고 겉모습만 나아진다 |
| 5 스크립트 | `v1` 전용 12개, `Archive/` 19개 | 0.2MB(0) | 0(링크 없음, 글자 인용은 있다) | **`Archive/`로 모은다** 또는 둔다 | 지금 빌드에서 돌지 않는다. 지우면 `Scripts/README.md`와 `commands.md`의 표를 함께 고쳐야 한다 |
| 6 추적 밖과 작업 문서 | `PoolSmoke1`\~`3`, 빈 `Reh3`, 낡은 작업 문서 5개, `.gitignore` 한 쌍 | 45KB(추적 밖) | 0 | **옮긴다·표시한다** | `PoolSmoke*`는 `Reh3`·`Reh4`처럼 `Saved/Rehearsal/`로 옮긴다. `plan.md`, `backlog.md`, `post-material.md`는 `v1` 상태를 지금처럼 적고 있다 |

1과 2를 함께 하면 새 클론의 LFS가 161.4MB에서 약 39MB가 된다. 84.1MB를 지우고, 단계 화면을 2.3절 E안으로 54.2MB에서 약 16MB로 줄이는 경우다. 단계 화면 크기는 추정이다.

가장 큰 발견 셋:

1. `v1` 묶음의 스크린샷 84MB가 새 클론 LFS의 절반을 넘는다. `v2`의 공개 자료(README, 글 여덟 편, 사이트)는 그중 한 장도 쓰지 않는다. `M4/shots/` 하나가 26.9MB다.
2. 단계 화면 8장(54.2MB)은 녹화 원본(mkv)이 없다. `Saved/Capture/Full4/`에는 같은 웹피(품질 60)만 있다. 줄이려면 웹피를 다시 인코딩(화질 손실이 겹친다)하거나 Test 패키지로 다시 찍어야 한다.
3. 실제로 끊어진 상대 링크는 0개다. 다만 작업 문서 셋(`plan.md`, `backlog.md`, `post-material.md`)이 `v1`의 글 번호와 `Full1`\~`Full3`을 지금 상태처럼 적는다. `commands.md`는 `Archive/`로 옮긴 스크립트 셋을 옛 경로로 적는다.

---

## 1. `v1` 시절의 측정 스크린샷

`v1` 묶음 가운데 `shots/`가 있는 묶음은 15개다. 이미지 109장, 84.05MB이고 모두 LFS다. `v2`의 공개 자료가 쓰는 스크린샷은 `Full4`, `V2M6Fix`, `V2W9Flash`에만 있다(1.5절).

### 1.1 묶음별 표

인용은 파일 이름(또는 경로)이 나오는 곳이다. "링크"는 마크다운 링크의 대상이 그 파일인 것이다.

| 묶음 | 이미지 | MB | 공개 자료 | 가이드·ADR·참고 | 그 밖의 문서 | Worklog |
| --- | --- | --- | --- | --- | --- | --- |
| `M4` | 14 | 26.86 | 없음 | 없음 | `post-material.md:285`(글자, `far*-p1-20k.png`) | `M4.md` 외. 링크 0 |
| `Q2` | 35 | 13.68 | 없음 | 없음 | `Scripts/create-crowd-fx.md:43`, `:51`(글자) | 링크 37(`Q2.md`) |
| `M0` | 1 | 5.82 | 없음 | 없음 | 없음 | `M0.md:35`(글자) |
| `M2` | 1 | 5.50 | 없음 | 없음 | 없음 | **아무도 안 쓴다**(`git grep mass-N5000`으로 다시 확인) |
| `Q5` | 8 | 5.23 | 없음 | 없음 | 없음 | 링크 5 |
| `M3b` | 17 | 4.52 | 없음 | 없음 | 없음 | 글자 인용만 |
| `Budget1` | 4 | 4.44 | 없음 | 없음 | 없음 | `Q7.md` 글자 인용 |
| `CrowdFx1` | 4 | 3.15 | 없음 | 없음 | 없음 | 글자 인용 |
| `Q4` | 4 | 3.11 | 없음 | 없음 | 없음 | 링크 3 |
| `Pool1` | 5 | 3.06 | 없음 | 없음 | 없음 | 글자 인용 |
| `M5` | 5 | 2.79 | 없음 | `Decisions/0007-keep-default-motion-blur.md:14`(글자, `flicker_m5bounds_velocity_f3943.jpg`) | 없음 | 링크 2 |
| `CrowdFx2` | 6 | 2.65 | 없음 | 없음 | 없음 | 글자 인용 |
| `Q1` | 2 | 1.44 | 없음 | `Guides/measurement.md:134`(링크 2) | 없음 | 링크 2 |
| `M3` | 2(+README 2) | 0.95 | 없음 | 없음 | `post-material.md:135`, `:159`(링크), `Scripts/transition-strips.py:1` | 링크 3 |
| `Full2` | 1 | 0.85 | 없음 | 없음 | 없음 | 링크 1 |
| 합계 | 109 | 84.05 | 0 | 링크 2, 글자 1 | 링크 4 | 링크 51 |

### 1.2 지울 때 함께 고칠 곳

- `Docs/Guides/measurement.md:134`: `Q1` 두 장의 링크. 태그 `v1`의 GitHub 주소(`https://github.com/hon454/unreal-horde-optimization-lab/blob/v1/Measurements/Q1/shots/...`)로 바꾸거나 링크를 글자로 바꾼다.
- `Docs/post-material.md:135`, `:159`: `M3` 화면 링크. 6절에서 이 문서를 기록으로 돌리면 같은 방법으로 고친다.
- `Measurements/M3/shots/transition/README.md`, `vat-shadow/README.md`: 이미지를 지우면 이 README 둘도 함께 지운다. 이 경로를 가리키는 곳은 `post-material.md:135`, `:159`, `Scripts/transition-strips.py:1`, `Source/HordeOptLab/LabConfig.h:202`(주석)다. ADR은 가리키지 않는다(`git grep "shots/transition"`).
- `Docs/Decisions/0007-keep-default-motion-blur.md:14`: 승인된 ADR이라 본문을 고치지 않는다. 글자 인용이 태그 `v1`을 가리키는 것으로 읽힌다.
- Worklog의 링크 51개: Worklog는 경위의 기록이라 고치지 않는 선택지가 있다. 고친다면 상대 경로를 태그 `v1` 주소로 일괄 치환한다.
- `Scripts/create-crowd-fx.md:43`, `:51`: `Q2` 화면을 "만든 결과"의 근거로 적는다. 태그 `v1` 경로로 바꾼다.

### 1.3 선택지

| 선택지 | LFS 감소 | 득 | 실 |
| --- | --- | --- | --- |
| A. 109장 모두 지운다(추천) | 84.05MB | 새 클론 LFS가 절반 아래가 된다. `Measurements/`를 열 때 `v2` 묶음만 이미지가 있다 | 링크 57개가 끊긴다. 고칠 곳은 가이드 1곳, `post-material.md` 1곳, `create-crowd-fx.md` 1곳과 Worklog다 |
| B. 가이드가 링크하는 `Q1` 두 장만 남긴다 | 82.61MB | `measurement.md`를 고치지 않는다 | `v1` 이미지가 일부 남는다 |
| C. 큰 것만 지운다(`M4`, `Q2`, `M0`, `M2`) | 51.86MB | 링크가 덜 끊긴다(Worklog 37) | 효과가 작다 |
| D. 두다 | 0 | 손이 안 든다 | 새 클론이 공개 자료와 무관한 84MB를 받는다 |

### 1.4 확인하지 못한 것

- Worklog의 글자 인용(링크가 아닌 파일 이름)은 묶음별 개수만 셌다. 줄 목록은 `git grep -n <파일 이름> -- Docs/Worklog`로 다시 뽑는다.

### 1.5 `v2` 묶음의 Worklog 전용 스크린샷(참고)

`v2` 묶음에도 공개 자료가 쓰지 않는 스크린샷이 있다. 공개 자료가 쓰는 것은 `Full4` 13장, `V2M6Fix`의 `iskm-forceseq1-f3839-before-after.jpg`, `V2W9Flash`의 `p1-reserve0-f4469-4471.jpg`·`p1-reserve1-f4469-4471.jpg`다. `V2W9Flash`의 `vel-reserve0-f3604-3606.jpg`는 ADR-0018이 인용한다.

| 묶음 | 이미지 | MB | 인용 |
| --- | --- | --- | --- |
| `Spike1` | 7 | 5.34 | Worklog 링크 7 |
| `V2W8Orb` | 4 | 1.67 | Worklog 링크 6, `measurement.md:150`(묶음 이름) |
| `Spike2` | 2 | 1.18 | Worklog 링크 2 |
| `V2W1` | 1 | 0.90 | Worklog 링크 1 |
| `V2W4Cost` | 1 | 0.82 | Worklog |
| `V2M6Vanish` | 10 | 0.57 | Worklog 링크 10, ADR-0020(묶음 이름) |
| `V2W6Base` | 1 | 0.49 | Worklog |
| `V2M6Fix`의 나머지 | 7 | 약 1.0 | Worklog 링크 8 |

합계 약 12MB, 33장이다. `v2` 경위의 근거라 이 조사는 **둔다**를 추천한다. 지우면 `v2` 태그(`v2-full4`)에 남는다.

### 묶음 1의 물음

1. `v1` 묶음의 스크린샷 109장(84MB)을 지울까? 선택지는 A(모두), B(`Q1` 둘 남김), C(큰 것만), D(둠)다.
2. 지운다면 끊기는 링크를 태그 `v1` 주소로 바꿀까, Worklog는 그대로 둘까?

---

## 2. 단계 화면

### 2.1 파일

| 파일 | 크기(MB) | 해상도 | 프레임 | 길이 | 인용 |
| --- | --- | --- | --- | --- | --- |
| `step1-actors.webp` | 0.94 | 960×540 | 13 | 8.0초 | `README.md:35`, `:45` |
| `step2-mass-engine.webp` | 7.90 | 960×540 | 120 | 8.0초 | `README.md:46`, `Posts/02-mass-engine-path/README.md:9`, 그 `measurements.md:219` |
| `step3-separation.webp` | 7.54 | 960×540 | 119 | 8.1초 | `README.md:47`, `Posts/03-separation/README.md:9`, 그 `measurements.md:171`, `:173` |
| `step4-vat-budget.webp` | 7.28 | 960×540 | 117 | 8.0초 | `README.md:48`, `Posts/04-quality-by-distance/README.md:73`, 그 `measurements.md:137` |
| `step5-simlod-pool.webp` | 7.49 | 960×540 | 120 | 8.0초 | `README.md:49` |
| `step6-iskm.webp` | 7.44 | 960×540 | 120 | 8.1초 | `README.md:50` |
| `step7-crowd.webp` | 7.79 | 960×540 | 120 | 8.0초 | `README.md:51`, `Posts/04-quality-by-distance/README.md:11`, 그 `measurements.md:136` |
| `step8-final.webp` | 7.79 | 960×540 | 120 | 8.0초 | `README.md:35`, `:52`, `Posts/05-cost-cuts-and-contributions/README.md:11`, 그 `measurements.md:222` |
| 합계 | 54.17 | | | | |

- 모두 손실 압축(VP8) 프레임이다. 15fps, 프레임당 약 63KB다. 헤더를 직접 읽어 셌다.
- `Scripts/build-site.py:353`이 `Docs/figures/steps/step{N}-*.webp`를 글로브로 찾아 `Site/assets/stepN.webp`로 복사한다. `Site/assets/`는 `.gitignore`에 있고 병합 때 `gh-pages`에 따로 넣는다(STATUS.md "다음 할 일" 9). 그래서 사이트도 이 여덟 장을 쓴다.
- 대표 정지 화면은 따로 있다: `Measurements/Full4/shots/step1.jpg`\~`step8.jpg`(장당 172\~197KB, 글 01·07이 쓴다).

### 2.2 다시 만드는 스크립트와 원본

- 스크립트: `Scripts/capture-full.ps1`. 게임 창을 ffmpeg gdigrab으로 거의 무손실 mkv로 찍고, `fps=$Fps,scale=${Width}:-1`, `libwebp_anim -q:v $Quality`(기본 60)로 웹피를 만든다(64행).
- 원본: 중간 mkv는 남아 있지 않다. `Saved/Capture/Full4/`에는 같은 웹피 8개(SHA-256이 같다, `Docs/Worklog/V2.md` "W14-3")와 ffmpeg 로그만 있다.
- 이 PC의 ffmpeg 9.0.2에는 움직이는 웹피 디코더(`webp_anim`)와 인코더(`libwebp_anim`)가 있다(`ffmpeg -decoders`). 그래서 지금 웹피를 읽어 다시 인코딩할 수 있다. 실제로 돌려 보지는 않았다.
- W14 결정 3이 "단계 화면은 다시 인코딩하지 않는다(960×540, 15fps, 8초)"다(`Docs/Worklog/V2-w14-plan.md:35`, `:86`). 줄이려면 사용자가 이 결정을 다시 정해야 한다. ADR은 아니다.

### 2.3 선택지와 예상 크기

예상 크기는 프레임 수와 화소 수에 비례한다고 본 어림이다. 같은 품질로 다시 인코딩하면 손실이 한 번 더 겹쳐 크기가 어림과 다를 수 있다. 단계 1(0.94MB)은 줄일 필요가 작아 7장(53.2MB)만 센다.

| 선택지 | 장당(추정) | 7장 합(추정) | 득 | 실 |
| --- | --- | --- | --- | --- |
| A. 10fps(80프레임) | 약 5.1MB | 약 35MB | 해상도와 화질을 지킨다 | 움직임이 덜 매끄럽다 |
| B. 640×360(화소 0.44배) | 약 3.3MB | 약 23MB | 프레임을 지킨다 | 개체가 작아 표현 차이가 덜 보인다 |
| C. 품질 60 → 40 | 약 4.5\~5MB | 약 32\~35MB | 크기·프레임을 지킨다 | 압축 흔적이 겹친다 |
| D. 4초로 자르기 | 약 3.8MB | 약 27MB | 화질을 지킨다 | 오브가 한 바퀴의 일부만 돈다 |
| E. A + B(10fps, 640×360) | 약 2.2MB | 약 15MB | 가장 작다 | 위 둘의 손해가 겹친다 |
| F. Test 패키지로 다시 찍기(`capture-full.ps1 -Fps 10 -Width 640` 등) | E와 비슷 | 약 15MB | 손실이 한 번만이다 | 패키지와 촬영 시간이 든다. 측정 PC를 쓰는 동안 다른 세션은 멈춘다 |
| G. 두다(추천은 아님) | 7.5MB | 53.2MB | 손이 안 든다 | 새 클론 LFS의 3분의 1이다 |

이 조사의 추천은 A + B 수준(E)을 먼저 웹피 재인코딩으로 시험하고, 화면을 보고 정하는 것이다. 실제로 다시 만들지는 않았다.

### 2.4 지울 때나 줄일 때 함께 고칠 곳

- 이름을 그대로 두고 다시 인코딩하면 링크는 고칠 곳이 없다.
- 글의 `measurements.md` 넷이 "960×540, 15fps, 8초"와 "원본", "바이트 사본"을 적는다: `Posts/02-mass-engine-path/measurements.md:219`, `Posts/03-separation/measurements.md:173`, `Posts/04-quality-by-distance/measurements.md:136`, `Posts/05-cost-cuts-and-contributions/measurements.md:222`. 다시 인코딩하면 이 넷의 값과 "원본" 문장을 고친다.
- `Docs/Worklog/V2-w14-plan.md` 결정 3에 바뀐 결정을 한 줄 더한다.

### 묶음 2의 물음

1. W14 결정 3("다시 인코딩하지 않는다")을 바꿔 단계 화면을 줄일까?
2. 줄인다면 어느 방식으로 줄일까? 웹피 재인코딩(A\~E)과 다시 찍기(F) 가운데 고른다.

---

## 3. 측정 묶음

### 3.1 전체 표

"공개"는 `README.md`, `Posts/`, `Site/`다. "가이드"는 `Docs/Guides`, `Docs/Reference`, `Docs/Decisions`다. "그 밖의 문서"는 `Docs/plan.md`, `v2-spec.md`, `v2-plan.md`, `post-material.md`, `backlog.md`, `STATUS.md`, `AGENTS.md`, `Source/`다. 수는 줄 수이고 괄호는 앞의 파일 몇 개다. 묶음 이름이 셋 이하 글자(`M0`, `Q1` 따위)면 `` `M0` `` 또는 `Measurements/M0` 꼴만 셌다. 그 밖의 묶음은 낱말 경계로 셌다. 세는 스크립트는 저장소 밖 스크래치에 있다.

| 묶음 | 시기 | 파일 | MB | LFS MB | 공개 | 가이드 | 그 밖의 문서 | Worklog 줄 | 스크립트 | 갈래 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `AB1` | v1 | 4 | 0.01 | 0.00 |  |  |  | 11 |  | Worklog·스크립트만 |
| `Budget1` | v1 | 8 | 4.50 | 4.44 |  | 4(0016-animation-budget-value.md, commands.md) |  | 24 | 8(README.md, check-budget.py…) | 가이드 |
| `CrowdFx1` | v1 | 5 | 3.15 | 3.15 |  |  |  | 1 |  | Worklog·스크립트만 |
| `CrowdFx2` | v1 | 7 | 2.65 | 2.65 |  |  |  | 4 | 1(create-crowd-fx.md) | Worklog·스크립트만 |
| `Full1` | v1 | 7 | 0.21 | 0.00 |  | 36(0008-game-scene-baseline-and-target-spec.md, 0009-flat-scenario-quality-by-distance.md, 0010-final-representation-bands.md…) | 9(plan.md, post-material.md) 1(AGENTS.md) | 111 | 9(capture-full.ps1, plot.py…) | 가이드 |
| `Full2` | v1 | 7 | 1.06 | 0.85 |  | 12(0010-final-representation-bands.md, 0011-entity-driven-crowd-particles.md, 0013-full-batch-runner-and-condition-table.md…) | 1(plan.md) | 85 | 2(pool-timeline.py, summarize.py) | 가이드 |
| `Full3` | v1 | 7 | 0.22 | 0.00 |  | 7(0016-animation-budget-value.md, 0017-roaming-orb-and-equal-function-ladder.md, measurement.md) | 1(backlog.md) | 72 | 2(plot-post02-manager.py) | 가이드 |
| `Full4` | v2 | 22 | 6.24 | 5.92 | 96(00/README.md, 00/make-figures.py, 00/measurements.md…) | 34(0020-far-stream-respawn-handoff.md, 0021-unified-actor-cap-and-hydrate-distance.md, commands.md…) | 13(STATUS.md, v2-plan.md, v2-spec.md) | 216 | 28(README.md, ablation-check.py…) | 공개 |
| `M0` | v1 | 3 | 5.84 | 5.82 |  | 9(0002-freeze-measurement-infra.md, 0006-three-measurement-tiers.md, 0015-readme-figures.md…) | 1(post-material.md) | 6 |  | 가이드 |
| `M1` | v1 | 2 | 0.01 | 0.00 |  | 1(engine-notes.md) | 1(post-material.md) | 5 |  | 가이드 |
| `M2` | v1 | 3 | 5.52 | 5.50 |  |  |  | 2 | 1(run-e2.ps1) | Worklog·스크립트만 |
| `M3` | v1 | 6 | 0.98 | 0.95 |  | 2(commands.md, engine-notes.md) | 2(post-material.md) 1(LabConfig.h) | 9 | 2(run-e3.ps1, transition-strips.py) | 가이드 |
| `M3b` | v1 | 22 | 4.85 | 4.54 |  | 3(0006-three-measurement-tiers.md, glossary.md) |  | 9 | 6(m3b-report.py, run-m3b-all.ps1…) | 가이드 |
| `M4` | v1 | 22 | 27.00 | 26.85 |  |  | 1(post-material.md) | 18 | 5(run-m4-all.ps1, run-m4-e4d.ps1…) | Worklog·스크립트만 |
| `M5` | v1 | 10 | 2.87 | 2.79 |  | 6(0007-keep-default-motion-blur.md, 0009-flat-scenario-quality-by-distance.md, commands.md…) |  | 24 | 6(run-m5-all.ps1, run-m5-bounds.ps1) | 가이드 |
| `Mgr1` | v1 | 4 | 0.02 | 0.00 |  |  | 1(plan.md) | 31 | 6(README.md, check-mgr.py…) | Worklog·스크립트만 |
| `MgrT1` | v1 | 4 | 0.10 | 0.00 |  |  |  | 9 | 9(README.md, mgr-trace-table.py…) | Worklog·스크립트만 |
| `Pool1` | v1 | 10 | 3.06 | 3.05 |  | 6(0015-readme-figures.md, glossary.md, measurement.md) |  | 24 | 1(run-pool.ps1) | 가이드 |
| `Pool2` | v2 | 7 | 0.01 | 0.00 | 25(05/make-figures.py, 05/measurements.md, 06/make-figures.py…) | 6(commands.md, measurement.md) | 3(STATUS.md) | 32 | 7(README.md, build-site.py…) | 공개 |
| `Pre4` | v1 | 1 | 0.00 | 0.00 |  |  |  | 3 |  | Worklog·스크립트만 |
| `Pre5` | v1 | 1 | 0.00 | 0.00 |  |  |  | 7 |  | Worklog·스크립트만 |
| `Pre6` | v1 | 1 | 0.00 | 0.00 |  |  |  | 8 |  | Worklog·스크립트만 |
| `Q1` | v1 | 2 | 1.44 | 1.44 |  | 2(glossary.md, measurement.md) |  | 3 |  | 가이드 |
| `Q2` | v1 | 35 | 13.66 | 13.66 |  |  |  | 32 | 2(create-crowd-fx.md) | Worklog·스크립트만 |
| `Q4` | v1 | 6 | 3.19 | 3.11 |  | 1(0010-final-representation-bands.md) |  | 9 |  | 가이드 |
| `Q5` | v1 | 8 | 5.21 | 5.21 |  |  |  | 6 |  | Worklog·스크립트만 |
| `Ref1` | v1 | 6 | 0.05 | 0.00 |  | 4(glossary.md, measurement.md) |  | 20 |  | 가이드 |
| `Spike1` | v2 | 13 | 5.42 | 5.33 |  |  | 4(v2-spec.md) 4(LabCrowdSpawner.cpp, LabCrowdSpawner.h, LabOrb.cpp…) | 25 | 4(README.md, run-spike.ps1…) | Worklog·스크립트만 |
| `Spike2` | v2 | 8 | 1.22 | 1.18 |  |  | 4(v2-spec.md) | 10 | 1(run-spike.ps1) | Worklog·스크립트만 |
| `V2Hyd35` | v2 | 5 | 0.01 | 0.00 |  | 6(0021-unified-actor-cap-and-hydrate-distance.md, measurement.md) | 2(LabConfig.h) | 10 |  | 가이드 |
| `V2M3Avoid` | v2 | 5 | 0.03 | 0.00 | 6(02/measurements.md) | 2(engine-notes.md) |  | 14 | 2(run-spike.ps1) | 공개 |
| `V2M6Cost` | v2 | 5 | 0.02 | 0.00 | 1(06/measurements.md) | 1(0021-unified-actor-cap-and-hydrate-distance.md) |  | 9 |  | 공개 |
| `V2M6Fix` | v2 | 13 | 1.15 | 1.13 | 2(06/README.md, 06/measurements.md) | 1(0021-unified-actor-cap-and-hydrate-distance.md) |  | 29 | 1(run-spike.ps1) | 공개 |
| `V2M6Split` | v2 | 5 | 0.02 | 0.00 | 1(06/measurements.md) | 2(0021-unified-actor-cap-and-hydrate-distance.md) |  | 22 | 1(run-spike.ps1) | 공개 |
| `V2M6Trace` | v2 | 3 | 0.00 | 0.00 | 1(06/measurements.md) |  |  | 6 |  | 공개 |
| `V2M6Vanish` | v2 | 10 | 0.55 | 0.55 |  | 1(0020-far-stream-respawn-handoff.md) |  | 9 |  | 가이드 |
| `V2SepWait` | v2 | 3 | 0.00 | 0.00 | 3(03/measurements.md) |  |  | 6 |  | 공개 |
| `V2SmoothOri` | v2 | 5 | 0.01 | 0.00 | 2(03/measurements.md) | 3(0021-unified-actor-cap-and-hydrate-distance.md, measurement.md) |  | 17 | 1(run-spike.ps1) | 공개 |
| `V2W1` | v2 | 7 | 0.93 | 0.90 |  |  |  | 10 | 1(summarize.py) | Worklog·스크립트만 |
| `V2W10Ladder` | v2 | 5 | 0.02 | 0.00 |  | 1(measurement.md) |  | 26 | 1(equivalence-check.py) | 가이드 |
| `V2W11Cost` | v2 | 6 | 0.09 | 0.00 |  | 4(0019-step8-actor-cap-distance-and-fixed-skel-bounds.md, engine-notes.md) | 1(LabConfig.h) | 10 |  | 가이드 |
| `V2W11Sweep` | v2 | 6 | 0.07 | 0.00 |  | 3(0019-step8-actor-cap-distance-and-fixed-skel-bounds.md, 0021-unified-actor-cap-and-hydrate-distance.md) |  | 26 |  | 가이드 |
| `V2W12Dry` | v2 | 7 | 0.02 | 0.00 |  | 7(commands.md, measurement.md) | 2(v2-plan.md) | 31 | 3(equivalence-check.py, full-conditions.json…) | 가이드 |
| `V2W13Pre` | v2 | 8 | 0.05 | 0.00 |  |  |  | 12 |  | Worklog·스크립트만 |
| `V2W1Still` | v2 | 6 | 0.01 | 0.00 |  |  |  | 6 |  | Worklog·스크립트만 |
| `V2W2Check` | v2 | 6 | 0.02 | 0.00 |  |  |  | 3 |  | Worklog·스크립트만 |
| `V2W3Cost` | v2 | 6 | 0.06 | 0.00 |  |  |  | 12 | 1(full-conditions.json) | Worklog·스크립트만 |
| `V2W3Trace` | v2 | 6 | 0.02 | 0.00 |  |  |  | 7 |  | Worklog·스크립트만 |
| `V2W4Cost` | v2 | 7 | 0.90 | 0.82 |  |  |  | 14 | 1(full-conditions.json) | Worklog·스크립트만 |
| `V2W4Trace` | v2 | 6 | 0.02 | 0.00 |  |  |  | 6 |  | Worklog·스크립트만 |
| `V2W5Cost` | v2 | 6 | 0.04 | 0.00 |  | 1(0019-step8-actor-cap-distance-and-fixed-skel-bounds.md) |  | 7 |  | 가이드 |
| `V2W5Trace` | v2 | 5 | 0.01 | 0.00 |  |  |  | 9 |  | Worklog·스크립트만 |
| `V2W6Base` | v2 | 7 | 0.54 | 0.49 |  |  |  | 9 | 1(full-conditions.json) | Worklog·스크립트만 |
| `V2W7Switch` | v2 | 6 | 0.11 | 0.00 |  | 2(0019-step8-actor-cap-distance-and-fixed-skel-bounds.md, measurement.md) |  | 18 |  | 가이드 |
| `V2W8Orb` | v2 | 10 | 1.74 | 1.66 |  | 1(measurement.md) |  | 14 |  | 가이드 |
| `V2W9Flash` | v2 | 10 | 0.90 | 0.86 | 4(06/README.md, 06/measurements.md) | 5(0018-iskm-flash-track-reserve.md, 0019-step8-actor-cap-distance-and-fixed-skel-bounds.md, 0020-far-stream-respawn-handoff.md…) | 2(post-material.md) | 25 |  | 공개 |

### 3.2 갈래

- **공개 자료가 쓰는 묶음(10개, 모두 `v2`)**: `Full4`, `Pool2`, `V2M3Avoid`, `V2M6Cost`, `V2M6Fix`, `V2M6Split`, `V2M6Trace`, `V2SepWait`, `V2SmoothOri`, `V2W9Flash`. **둔다.**
- **가이드·ADR이 쓰는 묶음(22개)**: `v1` 13개(`Budget1`, `Full1`, `Full2`, `Full3`, `M0`, `M1`, `M3`, `M3b`, `M5`, `Pool1`, `Q1`, `Q4`, `Ref1`)와 `v2` 9개(`V2Hyd35`, `V2M6Vanish`, `V2W10Ladder`, `V2W11Cost`, `V2W11Sweep`, `V2W12Dry`, `V2W5Cost`, `V2W7Switch`, `V2W8Orb`). 대부분 묶음 이름으로 인용한다. 파일 링크는 `measurement.md:134`의 `Q1` 둘뿐이다.
- **Worklog·스크립트·작업 문서만 쓰는 묶음(24개)**: `v1` 12개(`AB1`, `CrowdFx1`, `CrowdFx2`, `M2`, `M4`, `Mgr1`, `MgrT1`, `Pre4`, `Pre5`, `Pre6`, `Q2`, `Q5`)와 `v2` 12개(`Spike1`, `Spike2`, `V2W1`, `V2W13Pre`, `V2W1Still`, `V2W2Check`, `V2W3Cost`, `V2W3Trace`, `V2W4Cost`, `V2W4Trace`, `V2W5Trace`, `V2W6Base`).
- **아무도 안 쓰는 묶음: 없다.** 모든 묶음이 Worklog에 한 번 이상 나온다. 파일 하나로는 `M2/shots/mass-N5000-P1.png`(5.50MB)가 어디에도 인용되지 않는다(`git grep`으로 다시 확인).

### 3.3 `v1` 묶음 통째로 지울 때 함께 고칠 곳

`v1` 묶음 25개는 191파일, 85.6MB다. 이미지(84.05MB)를 빼면 CSV·로그·JSON 약 1.6MB다. 통째로 지워도 LFS 감소는 1번과 거의 같다.

- 가이드의 이름 인용: `commands.md:25`, `:29`(`M0`, `M5`), `:35`(`M3`), `:200`(`Budget1`), `measurement.md:113`, `:119`, `:140`(`Full3`), `:216`\~`:222`(`Ref1`), `:227`\~`:236`(`Pool1`), `glossary.md:156`, `:158`, `engine-notes.md:40`, `:175`, `troubleshooting.md:15`. 이 문장은 "태그 `v1`의 `Measurements/<묶음>`"으로 바꿔야 독자가 찾을 수 있다.
- 승인된 ADR의 본문(0002, 0006, 0007, 0008, 0009, 0010, 0011, 0013, 0015, 0016, 0017)은 `Measurements/Full1/summary.csv` 같은 경로를 적는다. ADR 본문은 고치지 않는 규칙이라, 지우면 ADR의 근거가 이 브랜치에서 사라진다. `Docs/Decisions/README.md`에 "`v1` 묶음은 태그 `v1`에 있다" 한 줄을 더하는 것으로 갈음할 수 있다.
- 스크립트: `summarize.py:49`(`Full2`), `pool-timeline.py:1`, `plot.py:549`(도움말 예), `spikes.py`, `capture-full.ps1:11`, `run-transition.ps1:6`, `Scripts/Archive/*`가 `v1` 묶음 이름을 적는다. 주석과 도움말이라 돌리는 데는 지장이 없다.
- `Source/HordeOptLab/LabConfig.h:202`(`M3`), `LabCrowdSpawner.*`, `LabOrb.*`(`Spike1`)는 주석이다. `Spike1`은 `v2` 묶음이라 지우지 않는다.

### 3.4 선택지

| 선택지 | LFS 감소 | 득 | 실 |
| --- | --- | --- | --- |
| A. `v1` 묶음의 이미지만 지우고 CSV는 둔다(추천, 1번 A와 같다) | 84.05MB | 가이드·ADR이 이름으로 인용하는 `summary.csv`가 남는다 | `Measurements/`에 `v1` 폴더 25개가 남는다 |
| B. `v1` 묶음 25개를 통째로 지운다 | 약 84MB(파일 전체 85.6MB) | `Measurements/`에 `v2` 묶음만 남는다 | ADR 11개와 가이드 10여 곳의 근거 경로가 이 브랜치에서 사라진다 |
| C. `v1` 묶음을 `Measurements/v1/`로 옮기고 이미지는 지운다 | 84.05MB | 폴더 목록이 깔끔하다 | 경로를 적은 모든 곳(가이드, ADR, 스크립트, Worklog)이 틀어진다. 추천하지 않는다 |
| D. 두다 | 0 | | |

`Measurements/README.md`는 "마일스톤마다 `M<N>/`에"라고 적는 `v1` 설명이다. 어느 선택지든 `v2`의 묶음 이름(`Full4`, `Pool2`, `V2*`, `Spike*`)으로 고친다.

### 3.5 확인하지 못한 것

- 낱말 경계 검색은 `Full1`처럼 긴 이름에는 정확하지만, `Q4`·`M5`처럼 마일스톤 이름과 같은 묶음은 백틱이나 경로로 쓴 것만 셌다. 실제 인용은 더 있을 수 있다.

### 묶음 3의 물음

1. `v1` 묶음 25개를 어떻게 할까? 이미지만 지우기(A)와 통째로 지우기(B) 가운데 고른다.

---

## 4. 옛 그림

| 그림 | 파일 | 크기 | 인용(공개 / 가이드 / Worklog) | 만드는 스크립트 |
| --- | --- | --- | --- | --- |
| `Docs/figures/full1/` | 7(SVG 4, CSV 3) | 21KB | 0 / `Decisions/0015-readme-figures.md:24`(글자) / `Posts.md`, `Q6.md` | `plot.py --batch Full1`(지금 조건 표로는 다시 만들 수 없다) |
| `Docs/figures/full2/` | 7 | 21KB | 0 / `0015-readme-figures.md:37`(글자) / `Posts.md`, `Q6.md` | `plot.py --batch Full2`(같다) |
| `Docs/figures/full3/` | 7 | 21KB | 0 / 0 / `Q7.md` | `plot.py --batch Full3`. `Scripts/plot-post02-manager.py:3`이 읽는다 |
| `Docs/figures/budget1/` | 5(SVG 4, CSV 1) | 34KB | 0 / `0016-animation-budget-value.md:32`는 `Q7.md`를 가리킨다 / `Q7.md:290` | `Scripts/plot-budget.py`(`v1` 전용) |
| `Docs/figures/n-at-60fps.svg`, `.csv`, `n-scaling-P1.svg` | 3 | 17KB | 0 / `commands.md:18`(설명) / `M3b.md`, `Q6.md:261`("그대로 뒀다. README는 더 가리키지 않는다") | `plot.py`를 `--batch` 없이 돌리면 `Measurements/*/summary.csv` 전부로 다시 쓴다(`plot.py:574`\~`:577`) |

- `full4/`, `readme/`, `code-tour/`, `steps/`는 지금 쓰인다.
- 검색할 때 `full-n-at-60fps.csv`와 `full-ladder.csv`는 `full1`\~`full4` 폴더에 같은 이름이 있다. 그래서 폴더 경로(`figures/full1/`)로 다시 셌다. 공개 자료의 링크는 모두 `full4/`를 가리킨다.
- 모두 텍스트 파일이라 LFS 효과는 없다. 지우는 효과는 `Docs/figures/`를 열 때의 모습뿐이다.
- 함께 고칠 곳: `commands.md:18`의 `plot.py` 설명(최상위 그림 셋), `Scripts/plot.py` 1\~7행 docstring. `--batch` 없는 모드를 남기면 다음에 돌릴 때 최상위 그림이 다시 생긴다. 그 모드가 `v2`의 `Measurements/`에서 도는지는 확인하지 못했다.

### 묶음 4의 물음

1. 옛 그림 다섯 무리(`full1`, `full2`, `full3`, `budget1`, 최상위 셋)를 지울까?
2. 지운다면 `plot.py`의 `--batch` 없는 모드도 없앨까?

---

## 5. 스크립트

### 5.1 전체 표

열은 다음과 같다. "갈래"는 이 조사의 판정이다. 숫자는 그 파일 이름이 나오는 횟수다. 셈에서 자기 파일은 뺐다. "다른 스크립트"는 `Scripts/`의 다른 파일, "훅"은 `.claude/`, "공개"는 `README.md`·`Posts/`·`Site/`다. 설명은 파일 첫 주석의 앞부분이다.

| 파일 | 크기 | 생긴 때 | 갈래 | STATUS | commands.md | Scripts/README | 다른 스크립트 | 훅 | 공개 | Worklog | 하는 일 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `Archive/fg-probe.ps1` | 2KB | v1부터 | v1 기록(Archive, 그 자리에서 안 돎) |  |  | 1 |  |  |  | 1 | 포그라운드 창 프로브(진단용). 2초마다 현재 포그라운드 창의 시각·PID·프로세스 이름·창 제목을 Saved/Lab/fg-probe.log에 덧붙인다. |
| `Archive/m3b-report.py` | 11KB | v1부터 | v1 기록(Archive, 그 자리에서 안 돎) |  |  | 1 | 1 |  |  |  | run-m3b-all.ps1 의 마무리: 묶음 1·2 결과 사실 요약을 쓴다 (해석 없음). |
| `Archive/run-e0.ps1` | 1KB | v1부터 | v1 기록(Archive, 그 자리에서 안 돎) |  |  | 1 |  |  |  | 1 | M0 실험 E0 묶음. Test 패키지로 조건을 연달아 잰다(2026-11-07 사용자가 고른 축소안). |
| `Archive/run-e1.ps1` | 1KB | v1부터 | v1 기록(Archive, 그 자리에서 안 돎) |  | 1 | 1 |  |  |  | 1 | M1 실험 E1 (a) 묶음: 컨테이너 전환(액터 → Mass). Test 패키지로 조건을 연달아 잰다. plan.md "M1". |
| `Archive/run-e1bc.ps1` | 1KB | v1부터 | v1 기록(Archive, 그 자리에서 안 돎) |  |  | 1 |  |  |  |  | M1 실험 E1 (b)(c) 묶음. Test 패키지. plan.md "M1". |
| `Archive/run-e1d.ps1` | 1KB | v1부터 | v1 기록(Archive, 그 자리에서 안 돎) |  |  | 1 |  |  |  |  | M1 실험 E1 (d) 피해 적용 방식 묶음. Test 패키지. plan.md "M1". |
| `Archive/run-e2.ps1` | 1KB | v1부터 | v1 기록(Archive, 그 자리에서 안 돎) |  | 1 | 1 |  |  |  | 2 | M2 실험 E2 묶음. Test 패키지로 조건을 연달아 잰다. plan.md "M2". |
| `Archive/run-e3.ps1` | 2KB | v1부터 | v1 기록(Archive, 그 자리에서 안 돎) |  |  | 1 |  |  |  | 2 | M3 실험 E3 묶음. Test 패키지로 조건을 연달아 잰다. plan.md "M3". |
| `Archive/run-flicker.ps1` | 1KB | v1부터 | v1 기록(Archive, 그 자리에서 안 돎) |  |  | 1 |  |  |  | 2 | 미니언 깜빡임 진단(Worklog/M5.md "미니언 깜빡임 진단"): 개발 빌드 20k P1을 고정 1/60초로 돌려 ShotAt부터 Burst 프레임을 연속  |
| `Archive/run-m3b-all.ps1` | 5KB | v1부터 | v1 기록(Archive, 그 자리에서 안 돎) |  |  | 1 | 1 |  |  | 1 | M3b 묶음 1·2 전체 러너. 한 번에 순서대로 돈다(같은 날 연달아가 규칙). 사용자가 "시작"을 정한 뒤에만 띄운다. |
| `Archive/run-m3b.ps1` | 9KB | v1부터 | v1 기록(Archive, 그 자리에서 안 돎) |  | 1 | 1 | 1 |  |  | 9 | M3b 묶음 러너(2026-11-09\~10에 돌렸다. 다시 돌리지 않는다). plan.md "M3b" 측정 방법, measurement.md "벤치마크 조건". |
| `Archive/run-m4-all.ps1` | 7KB | v1부터 | v1 기록(Archive, 그 자리에서 안 돎) |  |  | 1 | 2 |  |  | 2 | M4 마일스톤 묶음 러너(ADR-0006 마일스톤 묶음). 한 번에 순서대로 돈다. 사용자가 "시작"을 정한 뒤에만 띄운다(묶음 동안 PC를 쓰지 않는다). |
| `Archive/run-m4-e4d.ps1` | 6KB | v1부터 | v1 기록(Archive, 그 자리에서 안 돎) |  |  | 1 |  |  |  | 2 | E4 (d) 묶음 러너(Worklog/M4.md "측정 세션: E4 (d) 묶음"). 한 번에 순서대로 돈다. 사용자가 "시작"을 정한 뒤에만 띄운다(묶음 동안  |
| `Archive/run-m4-far.ps1` | 5KB | v1부터 | v1 기록(Archive, 그 자리에서 안 돎) |  |  | 1 |  |  |  | 2 | 그리는 거리" 묶음 러너(Worklog/M4.md "측정 세션: 그리는 거리 묶음"). m4offmove(M4, 2종) P1에서 -LabFarM 200/300/5 |
| `Archive/run-m4-trace2.ps1` | 2KB | v1부터 | v1 기록(Archive, 그 자리에서 안 돎) |  |  | 1 |  |  |  | 2 | M4 추가 직렬 트레이스 3회(Worklog/M4.md "트레이스 분석" 둘째 절): P2 m3mass, P2 m4mass, P1 m3mass. 20k, 2종,  |
| `Archive/run-m4-trace3.ps1` | 2KB | v1부터 | v1 기록(Archive, 그 자리에서 안 돎) |  |  | 1 |  |  |  | 5 | (-Tag로 라벨 태그를 바꿀 수 있다. 수정 검증 트레이스는 -Tag traceparfix) M4 병렬 트레이스 2회(Worklog/M4.md "병렬 트레이스  |
| `Archive/run-m5-all.ps1` | 7KB | v1부터 | v1 기록(Archive, 그 자리에서 안 돎) |  |  | 1 | 3 |  |  | 2 | M5 마일스톤 묶음 러너(ADR-0006 마일스톤 묶음). 한 번에 순서대로 돈다. 사용자가 "시작"을 정한 뒤에만 띄운다(묶음 동안 PC를 쓰지 않는다). |
| `Archive/run-m5-bounds.ps1` | 6KB | v1부터 | v1 기록(Archive, 그 자리에서 안 돎) |  |  | 1 |  |  |  | 1 | M5 고정 바운드 실험 러너(Worklog/M5.md "측정 세션"). 사용자가 "시작"을 정한 뒤에만 띄운다(묶음 동안 PC를 쓰지 않는다). |
| `Archive/run-m5-trace.ps1` | 2KB | v1부터 | v1 기록(Archive, 그 자리에서 안 돎) |  |  | 1 |  |  |  | 1 | M5 트레이스 4회(Worklog/M5.md "트레이스 분석"): 20k P1, 2종. m4offpool(M4) 병렬, m5iskm(M5) 병렬, m4offpoo |
| `README.md` | 10KB | v1부터 | 매번(v2) |  |  |  | 5 | 3 | 116 | 131 | 스크립트 |
| `ablation-check.py` | 9KB | v1부터 | 매번(v2) |  | 8 | 1 | 17 |  |  | 59 | 측정 실행의 CSV 메타데이터를 조건 표의 기대값과 대조하고 프레임 지표를 한 줄로 보인다(v2 W12). |
| `bake-vat.py` | 25KB | v1부터 | 에셋 복구 |  | 1 | 1 | 20 |  | 1 | 55 | M2 VAT(정점 애니메이션 텍스처) 굽기. 미니언의 걷기·피격·죽기를 텍스처에 굽고, 그 텍스처를 읽는 머티리얼과 정적 메시를 만든다. |
| `batch-check.py` | 7KB | v2에서 새로 | 매번(v2) |  | 2 | 2 | 6 |  | 1 | 25 | 총괄 묶음 끝 검사 하나(리뷰 M4, Docs/Guides/measurement.md "묶음 판정"). runs.csv(summarize.py가 만든다)만 읽는다 |
| `batch_rules.py` | 3KB | v2에서 새로 | 매번(v2) |  |  | 2 | 5 |  |  | 6 | 총괄 묶음의 판정 규칙 중 요약과 묶음 끝 검사가 함께 쓰는 것(리뷰 M4, Docs/Guides/measurement.md "느린 실행"). |
| `bind-anim-start.md` | 2KB | v1부터 | 에셋 복구 |  |  | 1 | 2 |  |  | 3 | ABP_LabMinion 시작 시각 핀 연결 (M3) |
| `breakdown.py` | 2KB | v1부터 | 진단 도구(빌드 무관) |  |  | 1 |  |  |  | 2 | 두 조건(라벨 접두사)의 CSV 프로파일에서 스탯 열마다 3회 중앙값의 중앙값을 구해, 차이가 큰 순서로 보여 준다. |
| `build-site.py` | 22KB | v1부터 | 매번(v2) | 1 |  | 2 |  |  | 3 | 44 | Builds the one-page results site in Site/ (GitHub Pages). |
| `build.ps1` | 0KB | v1부터 | 매번(v2) | 1 | 1 | 1 | 1 | 1 |  | 53 | . "$PSScriptRoot\common.ps1 `.claude/` 훅이 명령 이름으로 안다 |
| `capture-full.ps1` | 15KB | v1부터 | 매번(v2) |  | 4 | 1 | 1 |  | 1 | 26 | 총괄 묶음 촬영(v2 W12. v1은 Docs/Worklog/Q5.md "2단위"). 측정이 끝난 뒤 따로 돈다. 사다리 8단계 x 목표 총수 x 이동 조건(V0 |
| `check-budget.py` | 3KB | v1부터 | **v1 전용** |  |  | 1 | 3 |  |  | 10 | 참고 묶음 Budget1 실행의 CSV 메타데이터를 기대값과 대조한다(Docs/Worklog/Q7.md "측정: Budget1"). |
| `check-iskm.py` | 0KB | v1부터 | **v1 전용** |  |  | 1 | 3 |  |  | 2 | run-m5-all.ps1의 메타데이터 검사용. CSV의 Lab/IskmInstances 중앙값을 찍는다. 열이 없으면 'nocol', 중앙값이 0 이하면 종료  `Archive/run-m5-all.ps1`만 부른다 |
| `check-mgr.py` | 3KB | v1부터 | **v1 전용** |  |  | 1 | 3 |  |  | 2 | 참고 묶음 Mgr1 실행의 CSV 메타데이터를 기대값과 대조한다(Docs/Worklog/Q8.md "측정: Mgr1"). |
| `check-tilde.py` | 1KB | v1부터 | 매번(v2) | 1 |  | 1 |  | 4 |  | 24 | Finds "\~" that is not written as "\\~" in the git-tracked Markdown files. 훅 3개가 부른다 |
| `common.ps1` | 1KB | v1부터 | 매번(v2) |  | 1 | 1 | 17 |  | 1 | 3 | 다른 스크립트가 불러 쓴다. 프로젝트 경로와 엔진 경로를 정한다. |
| `create-anim-bp-large.md` | 2KB | v1부터 | 에셋 복구 |  |  | 1 | 1 |  |  | 2 | ABP_LabMinionLarge 만들기 (M3b 2종) |
| `create-anim-bp.py` | 2KB | v1부터 | 에셋 복구 |  |  | 1 | 3 |  |  | 4 | 미니언 AnimBP 에셋 /Game/Lab/Anim/ABP_LabMinion을 만든다(부모 ULabMinionAnimInstance, 스켈레톤 Minion_Lan |
| `create-band-floor.md` | 3KB | v1부터 | 에셋 복구 |  |  | 1 | 1 |  |  | 2 | M_LabBandFloor 만들기 (M3b, 결정 6) |
| `create-band-floor.py` | 5KB | v1부터 | 에셋 복구 |  |  | 1 | 3 |  |  | 4 | 개발 빌드용 바닥 머티리얼 /Game/Lab/Materials/M_LabBandFloor를 만든다(backlog.md 결정 6, 오브 중심 거리 띠). |
| `create-bench-map.py` | 2KB | v1부터 | 에셋 복구 |  | 1 | 1 | 1 |  |  | 7 | 벤치마크 맵 /Game/Lab/Maps/L_Bench를 엔진 Basic 템플릿(Template_Default: 평평한 바닥, 해, 하늘, 안개)으로 만든다. |
| `create-bolt-fx.md` | 1KB | v1부터 | 에셋 복구 |  |  | 1 |  |  |  | 3 | NS_LabBolt 만들기 |
| `create-crowd-fx.md` | 24KB | v1부터 | 에셋 복구 |  |  | 1 | 2 |  | 3 | 32 | NS_LabCrowd 만들기 |
| `create-iskm.py` | 18KB | v1부터 | 에셋 복구 |  | 1 | 1 | 5 |  | 2 | 18 | M5 ISKM(InstancedSkinnedMeshComponent) 에셋 만들기. 30\~200m 원거리 표현을 VAT ISM에서 ISKM으로 바꾸는 데 쓴다( |
| `death-clip-sheet.py` | 5KB | v1부터 | 진단 도구(빌드 무관) |  |  | 1 | 1 |  |  | 2 | 죽기 클립 후보 비교 시트와 표를 만든다. 입력은 Scripts/dump-death-clips.py가 쓴 Saved/Lab/death_clips.json. |
| `dump-death-clips.py` | 3KB | v1부터 | 진단 도구(빌드 무관) |  |  | 1 | 1 |  |  | 3 | 죽기 클립 후보의 길이·루트 모션·뼈 위치를 JSON으로 뽑는다(2026-11-10, 대형 죽기가 공중에서 멈춘 것처럼 보이는 문제의 후보 비교). |
| `equivalence-check.py` | 21KB | v1부터 | 매번(v2) |  | 4 | 2 | 8 |  |  | 53 | 총괄 묶음의 단계 사이 동등성을 검사한다(v2 W12. 계획은 Docs/Worklog/V2.md "W12 측정 준비"). |
| `flash-scan.py` | 3KB | v2에서 새로 | v2 작업 단위 도구(끝남) |  |  | 1 | 2 |  |  | 5 | v2 W9 ISKM 번쩍임: 연속 캡처(Scripts/run-flash.ps1, f<프레임>.png와 run.log)에서 한 프레임만 튀는 프레임을 센다(Docs |
| `flicker-scan.py` | 2KB | v1부터 | 진단 도구(빌드 무관) |  |  | 1 | 1 |  |  | 2 | 연속 캡처(-LabShotBurst, f<프레임>.png)에서 한두 프레임만 튀는 밝기 변화를 찾는다(Worklog/M5.md "미니언 깜빡임 진단"). |
| `full-conditions.json` | 21KB | v1부터 | 매번(v2) | 1 | 4 | 3 | 32 |  | 2 | 103 | { |
| `lab_conditions.py` | 2KB | v2에서 새로 | 매번(v2) |  |  | 2 |  |  |  | 11 | 조건 표(Scripts/full-conditions.json, v2)를 읽는 공용 코드. ablation-check.py, equivalence-check.py, |
| `lab_unit_colors.py` | 1KB | v1부터 | 에셋 복구 |  |  | 1 | 2 |  |  | 2 | 미니언 색 값(선형 RGB). bake-vat.py(VAT·액터 머티리얼)와 make-static-minion.py(M1 대형 정적 메시 머티리얼)가 함께 읽는다 |
| `make-static-minion.py` | 5KB | v1부터 | 에셋 복구 |  | 4 | 1 | 6 |  |  | 14 | static 변형(M0)과 M1 애니 없는 ISM이 같이 쓰는 미니언 정적 메시를 만든다. 레퍼런스 포즈 그대로다. |
| `mgr-trace-table.py` | 2KB | v1부터 | **v1 전용** |  |  | 1 |  |  |  | 2 | 참고 묶음 MgrT1 트레이스의 타이머를 프레임당 ms로 모아 Measurements/MgrT1/timers.csv를 만든다(Docs/Worklog/Q8.md " |
| `migrate-assets.py` | 4KB | v1부터 | 에셋 복구 |  | 2 | 1 | 4 |  |  | 7 | Fab 원본 팩(AssetHost)에서 쓰는 에셋과 그 의존성만 HordeOptLab/Content로 옮긴다. |
| `noise-table.py` | 1KB | v1부터 | 진단 도구(빌드 무관) |  | 2 | 1 |  |  |  |  | 같은 조건 실행들의 프레임·Game·Render p50 값을 실행별로 늘어놓고 중앙값·최소\~최대·표준편차를 낸다 (잡음 진단용, Markdown 표 출력). |
| `package.ps1` | 1KB | v1부터 | 매번(v2) | 1 | 3 | 1 | 6 | 1 | 1 | 31 | Test(기본) 구성 Win64 패키지를 만든다. 수치는 이 패키지에서만 잰다(ADR-0001). |
| `plot-budget.py` | 8KB | v1부터 | **v1 전용** |  |  | 1 |  |  |  | 2 | Makes the SVG figures of the reference batch Budget1 in Docs/figures/budget1/ (Docs/Worklo |
| `plot-code-tour.py` | 7KB | v1부터 | 매번(v2) |  |  | 1 |  |  |  | 12 | Makes the frame-flow figure of Docs/Guides/code-tour.md in Docs/figures/code-tour/. |
| `plot-post02-manager.py` | 15KB | v1부터 | **v1 전용** |  |  | 1 |  |  |  | 3 | Makes the four extra SVG figures of post 02 in Posts/02-mass-logic/images/. 돌리면 지운 `Posts/02-mass-logic/images/`를 만든다 |
| `plot-readme.py` | 53KB | v1부터 | 매번(v2) | 1 |  | 3 | 5 |  | 8 | 39 | Makes the SVG figures of the root README in Docs/figures/readme/. |
| `plot.py` | 39KB | v1부터 | 매번(v2) | 1 | 2 | 1 | 7 | 1 | 35 | 73 | Measurements/*/summary.csv로 Docs/figures/의 그래프를 다시 그린다. 표준 라이브러리만 써서 SVG를 직접 쓴다(PC에 matplo |
| `pool-timeline.py` | 3KB | v1부터 | 매번(v2) |  | 2 | 2 | 4 |  | 2 | 8 | 엔티티 풀 묶음(Pool<n>)의 시간 추이를 1분 구간으로 정리한다(Docs/Worklog/Q6.md "Full2 재측정", v2는 Docs/Worklog/V2 |
| `restore-assets.manifest.txt` | 11KB | v1부터 | 에셋 복구 |  |  | 1 | 4 |  |  | 2 | restore-assets.ps1이 끝에 대조하는 git 밖 에셋 목록(Content/ 기준 상대 경로). |
| `restore-assets.ps1` | 18KB | v1부터 | 에셋 복구 | 1 | 1 | 1 | 2 |  | 2 | 11 | 깨끗한 클론에서 git 밖 에셋을 한 번에 복구한다. 가이드는 Docs/Guides/setup.md. |
| `run-bench.ps1` | 9KB | v1부터 | **v1 전용** |  | 11 | 1 | 26 | 1 |  | 62 | Test 패키지를 -LabBench로 띄워 조건 하나를 반복해 잰다. -Milestone이 단계(-LabStage)다(ADR-0004). `-LabVariant`를 넘겨 지금 빌드는 종료 코드 2 |
| `run-budget.ps1` | 12KB | v1부터 | **v1 전용** |  | 1 | 2 | 2 |  |  | 6 | 참고 묶음 Budget1 러너(Docs/Worklog/Q7.md "측정: Budget1"). 최종 구성 q4final(-LabStage=M5)에서 -LabBudg |
| `run-dev-stages.ps1` | 2KB | v2에서 새로 | 매번(v2) |  | 3 | 2 |  |  |  | 29 | 공용 코드를 고친 뒤의 확인: 사다리 단계마다 작은 실행(run-dev.ps1 -Small)을 차례로 돌리고 종료 코드를 모은다(AGENTS.md "완료 확인"  |
| `run-dev.ps1` | 1KB | v1부터 | 매번(v2) | 1 | 9 | 1 | 9 | 1 |  | 102 | Development 에디터 빌드를 -game으로 실행한다. 개발·동작 확인·오버레이 스크린샷용이고 수치는 비교에 쓰지 않는다(ADR-0001). |
| `run-flash.ps1` | 2KB | v2에서 새로 | v2 작업 단위 도구(끝남) |  |  | 1 | 2 |  |  | 11 | v2 W9 ISKM 번쩍임 진단(Docs/Worklog/V2.md "W9"): 개발 빌드 최종 구성(1만, 오브 궤도 기본값 2m/s, P1)을 고정 1/60초로 |
| `run-full.ps1` | 36KB | v1부터 | 매번(v2) | 1 | 10 | 3 | 18 |  | 4 | 84 | 총괄 묶음 러너(v2 W12). Test 패키지 exe를 직접 부른다. v1의 run-full.ps1(태그 v1)을 v2의 사다리 단계(-LabStage=1\~8) |
| `run-mgr.ps1` | 13KB | v1부터 | **v1 전용** |  | 1 | 2 | 1 |  |  | 6 | 참고 묶음 Mgr1 러너(Docs/Worklog/Q8.md "측정: Mgr1"). 변형 manager와 2단계(Mass 로직), 2단계 직렬을 같은 묶음에서 잰다 |
| `run-pool.ps1` | 22KB | v1부터 | 매번(v2) |  | 6 | 1 | 4 |  |  | 16 | 엔티티 풀 긴 실행 러너(v2, 묶음 Pool<n>). 최종 구성(단계 8, 조건 s8, 풀 켬)과 풀을 끈 조건(a8pool, -LabEntityPool=0)을 |
| `run-q4.ps1` | 1KB | v1부터 | **v1 전용** |  | 2 | 2 |  |  |  | 4 | Q4 품질 올리기 묶음(Docs/Worklog/Q4.md). run-bench.ps1을 조건마다 차례로 부른다. run-bench.ps1은 고치지 않는다. |
| `run-spike.ps1` | 24KB | v2에서 새로 | v2 작업 단위 도구(끝남) |  | 7 | 1 | 2 |  |  | 80 | 작은 묶음 러너(ADR-0017, Docs/Worklog/V2.md "시험 실행"). 떠다니는 오브의 궤도 속도 × 구성을 Test 패키지에서 잰다. |
| `run-transition.ps1` | 3KB | v1부터 | **v1 전용** |  | 1 | 2 |  |  |  | 5 | Development 전환 점수 묶음(ADR-0012 결정 4). 최종 구성과 ISKM 끔, 전환 대책 끔, 둘 다 끔을 시드마다 한 번씩 찍고 점수를 낸다. `run-dev.ps1 -Stage M5 -Variant`를 불러 지금은 인자 검사에서 실패한다 |
| `spike-check.py` | 1KB | v2에서 새로 | v2 작업 단위 도구(끝남) |  | 1 | 1 | 2 |  |  | 7 | 시험 실행 묶음(Spike<n>) 실행 하나의 CSV 메타데이터를 기대값과 대조한다(run-spike.ps1이 부른다). |
| `spike-timeline.py` | 6KB | v2에서 새로 | v2 작업 단위 도구(끝남) |  |  | 1 |  |  |  | 14 | 시험 실행 묶음(Spike<n>)의 가까운 개체 수를 시간 구간으로 정리한다(Docs/Worklog/V2.md "시험 실행"). |
| `spikes.py` | 5KB | v1부터 | 매번(v2) |  | 4 | 2 | 17 |  | 1 | 47 | 한 묶음의 주기적 스파이크를 찾는다 (measurement.md "묶음 검사"). |
| `summarize.py` | 39KB | v1부터 | 매번(v2) | 1 | 10 | 4 | 36 | 1 | 7 | 112 | 벤치마크 CSV를 마일스톤 측정 결과로 정리한다. |
| `trace-export.ps1` | 1KB | v1부터 | 진단 도구(빌드 무관) |  |  | 1 | 3 |  | 5 | 22 | Insights 트레이스(.utrace)에서 표를 뽑는다(UnrealInsights.exe -NoUI -AutoQuit, 명령줄 내보내기). M4 트레이스 분석( |
| `trace-order.py` | 2KB | v1부터 | 진단 도구(빌드 무관) |  |  | 1 |  |  |  | 3 | trace-timeline.py와 같은 입력(TimingEvents CSV)으로 프레임마다 MassVisualizationLODProcessor의 끝이 다른 프로 |
| `trace-overlap.py` | 3KB | v1부터 | 진단 도구(빌드 무관) |  |  | 1 |  |  |  | 2 | 병렬 트레이스의 프로세서 이벤트(TimingEvents CSV: ThreadName,TimerName,StartTime,Duration,Depth)에서 프레임마다 |
| `trace-report.py` | 2KB | v1부터 | 진단 도구(빌드 무관) |  |  | 1 |  |  |  | 4 | trace-export.ps1 TimerStatistics 결과(GameThread)를 프레임당 ms 표로 만든다. M4 트레이스 분석(Docs/Worklog/M |
| `trace-timeline.py` | 2KB | v1부터 | 진단 도구(빌드 무관) |  |  | 1 | 1 |  |  | 1 | trace-export.ps1 TimingEvents 결과(열 ThreadName,TimerName,StartTime,Duration,Depth)에서 프레임마다  |
| `transition-score.py` | 3KB | v1부터 | 진단 도구(빌드 무관) |  | 2 | 1 | 1 |  |  | 10 | 전환(하이드레이션) 튐 점수. 60fps 연속 캡처(-LabShotBurst, -benchmark -fps=60)와 그 실행의 개발 로그(lab_event hyd |
| `transition-strips.py` | 2KB | v1부터 | **v1 전용** |  |  | 1 |  |  |  |  | M3 전환 대책 조합별 60fps 캡처에서 같은 전환 사건 주변을 잘라 조합마다 한 줄씩 붙인다(Measurements/M3/shots/transition/REA M3 캡처 전용 |
| `turn-fill.py` | 3KB | v1부터 | 진단 도구(빌드 무관) |  | 1 | 1 |  |  |  | 2 | 빠른 회전 뒤 가까운 액터가 채워지는 시간(Q4 품질 지표 (나), 개발 빌드 참고). |
| `vanish-scan.py` | 6KB | v2에서 새로 | v2 작업 단위 도구(끝남) |  |  |  |  |  |  | 3 | v2 리뷰 M6: 입자 → ISKM 전환의 사라짐 검사(Docs/Worklog/V2.md "M6"). `Scripts/README.md`에 없다 |

글 폴더의 `make-figures.py` 8개(`Posts/00-goals-and-method/`\~`Posts/07-summary/`)는 모두 `Full4`·`Pool2`를 읽어 자기 글의 `images/`에 그림을 쓴다. 각 글의 `measurements.md`가 실행 방법을 적는다. 정리 대상이 아니다.

### 5.2 갈래별 요약

| 갈래 | 수 | 판정 |
| --- | --- | --- |
| 매번(v2) | 23 | 둔다 |
| 에셋 복구 | 15 | 둔다. `restore-assets.ps1`이 부르고 `setup.md`가 설명한다 |
| 진단 도구(빌드 무관) | 12 | 둔다. CSV·트레이스·캡처만 읽어 지금도 돈다 |
| v2 작업 단위 도구(끝남) | 6 | 둔다. `vanish-scan.py`만 `Scripts/README.md`에 없어 표에 한 줄을 더한다 |
| **v1 전용** | 12 | 지금 빌드에서 돌지 않는다. `Archive/`로 옮기거나 지운다 |
| `v1` 기록(`Archive/`) | 19 | 이미 `Archive/`다. 그 자리에서 돌지 않는다(`Scripts/README.md`) |

`v1` 전용 판정의 근거: `Source/HordeOptLab/LabConfig.cpp:27`\~`:44`가 `-LabStage=`에 `1`\~`8` 밖의 값이나 `-LabVariant=`를 받으면 실행을 무효로 한다(주석 "받으면 종료 코드 2"). `run-bench.ps1`, `run-q4.ps1`, `run-budget.ps1`, `run-mgr.ps1`, `check-mgr.py`가 옛 이름을 넘긴다. `run-transition.ps1:36`은 `run-dev.ps1 -Stage M5 -Variant`를 부르는데 `run-dev.ps1:8`의 `ValidateSet`이 `1`\~`8`만 받는다. `plot-post02-manager.py:16`은 지운 `Posts/02-mass-logic/images/`에 쓴다. `transition-strips.py`는 `M3` 캡처 전용이다.

### 5.3 선택지

| 선택지 | 득 | 실 |
| --- | --- | --- |
| A. `v1` 전용 12개를 `Scripts/Archive/`로 옮긴다(추천) | `Scripts/` 최상위가 지금 쓰는 것만 남는다. 기록은 남는다 | `Scripts/README.md`, `commands.md`의 경로를 고친다. `commands.md`가 이미 옛 경로로 적는 `run-m3b.ps1`(`:21`), `run-e1.ps1`(`:278`), `run-e2.ps1`(`:279`)도 함께 고친다 |
| B. `v1` 전용 12개와 `Archive/` 19개를 지운다 | `Scripts/`가 56개로 준다 | 태그 `v1`에 있지만, `v2` Worklog의 글자 인용(`run-bench.ps1` 62줄 등)이 이 브랜치에서 가리킬 곳을 잃는다 |
| C. `plot-post02-manager.py` 하나만 지운다 | 돌렸을 때 지운 글 폴더를 다시 만드는 위험만 없앤다 | 나머지는 그대로다 |
| D. 두다 | | `Scripts/README.md`의 "`v1` 전용" 절이 이미 설명한다 |

### 5.4 확인하지 못한 것

- 표의 횟수는 파일 이름의 글자 검색이다. 짧은 이름(`plot.py`)은 `plot.py`를 포함한 다른 이름(`plot-readme.py`의 설명 속 `plot.py`)과 섞이지 않게 낱말 경계로 셌지만, 설명문 속 언급과 실제 호출을 가르지 않았다.
- 진단 도구 12개가 지금의 CSV 열 이름으로 도는지는 돌려 보지 않았다.

### 묶음 5의 물음

1. `v1` 전용 스크립트 12개를 어떻게 할까? `Archive/`로 옮기기(A), 지우기(B), `plot-post02-manager.py`만 지우기(C) 가운데 고른다.

---

## 6. 추적되지 않는 것과 작업 문서

### 6.1 추적되지 않는 것과 무시되는 것

`git status --short --ignored`의 결과다. 레포 폴더 안에 쌓인 크기를 함께 적는다.

| 경로 | 상태 | 크기 | 판단 |
| --- | --- | --- | --- |
| `Measurements/PoolSmoke1/` | 추적 밖 | 24KB(7파일) | 러너 시험 묶음. `V2.md:4689`가 "커밋하지 않는다"고 적었다 |
| `Measurements/PoolSmoke2/` | 추적 밖 | 4KB(4파일) | 실패 경로 시험(시간 초과) |
| `Measurements/PoolSmoke3/` | 추적 밖 | 17KB(7파일) | 실패 경로 시험(연속 실패 중단 끔) |
| `Measurements/Reh3/` | 빈 폴더(git은 보지 않는다) | 0 | `Q6.md:85`의 예행 묶음. 내용은 이미 `Saved/Rehearsal/`로 옮겼다(`Q8.md:320`) |
| `Saved/` | 무시 | 131GB | 프레임 CSV, 트레이스, 캡처. 정리하면 디스크만 준다 |
| `Intermediate/` | 무시 | 19GB | 빌드 산출물 |
| `Binaries/` | 무시 | 2.2GB | |
| `Packaged/` | 무시 | 2.1GB | Test 패키지 |
| `Content/ParagonMinions/` | 무시 | 292MB | Fab 원본(재배포 금지) |
| `Site/assets/` | 무시 | 52MB | `build-site.py`가 복사한 단계 화면. 병합 때 `gh-pages`에 넣는다 |
| `Content/NiagaraExamples/` | 무시 | 27MB | Fab 원본 |
| `Content/Lab/VAT/`, `ISKM/`, `Derived/` | 무시 | 7.3MB, 5.7MB, 0.6MB | 베이크 산출물(안에 `Baked/`만 있다) |
| `Build/`, `DerivedDataCache/`, `Scripts/__pycache__/` | 무시 | 2.1MB, 1.6MB, 0.3MB | |
| `.mcp.json`, `.claude/settings.local.json`, `.vs/`, `.vsconfig`, `*.sln`, `*.slnx` | 무시 | 작다 | 로컬 설정 |

그 밖의 빈 폴더: `Content/Collections`, `Content/Developers/hon45/Collections`(에디터가 만든다. git은 빈 폴더를 보지 않으므로 레포의 모습과 무관하다).

`PoolSmoke1`\~`3`과 `Reh3`의 선택지:

| 선택지 | 득 | 실 |
| --- | --- | --- |
| A. `PoolSmoke1`\~`3`을 `Saved/Rehearsal/`로 옮기고 빈 `Reh3`를 지운다(추천) | `Reh3`·`Reh4`의 처리와 같다(`Q8.md:320`). 작업 트리가 깨끗해진다 | `commands.md:358`의 예시 명령 `-Batch PoolSmoke1`은 그대로 둬도 된다(다시 돌리면 거부되는 이름이라는 점만 남는다) |
| B. 지운다 | 가장 간단하다 | 실패 경로 시험의 기록(`failures.csv`, `progress.log`)이 사라진다. 경위는 `V2.md:4684`\~`:4689`에 있다 |
| C. `.gitignore`에 `Measurements/*Smoke*/`를 더한다 | 다음 시험 묶음도 자동으로 빠진다 | 측정 인프라 규칙과 엮인다. `.gitignore`를 고치는 일이 하나 는다 |

### 6.2 작업 문서

하위 에이전트(Sonnet, 읽기만)가 8개 문서를 읽었고, 이 세션이 주요 줄을 다시 열어 확인했다. 확인하지 못한 한 항목(`post-material.md`가 ADR-0018을 "제안됨"이라 적는다는 지적)은 해당 줄에서 찾지 못해 뺐다.

| 문서 | 판정 | 낡은 곳(예) |
| --- | --- | --- |
| `Docs/plan.md`(44KB) | `v1` 계획의 기록 | `:3` "지금 계획은 아래 '품질 계획'", `:276` "지금 묶음은 `Full2`", `:300` `v1`의 글 02. 사다리 표(`:237`\~`:262`)와 측정 목록이 `v1`이다. `v2-spec.md:12`가 이 문서의 "이점의 원칙" 절을 인용한다 |
| `Docs/v2-plan.md` | 작업 문서(맞다) | 작업 단위 표(`:36`\~`:51`)에 끝남 표시가 없다. `:55`의 시제 |
| `Docs/v2-spec.md` | 작업 문서(맞다) | 시제 몇 곳(`:34`) |
| `Docs/backlog.md`(13줄) | 거의 쓸모없다 | `:3`이 `plan.md` "품질 계획"을 가리킨다. `:7` "글 00\~09"(`v2`는 00\~07). `:11` "지금 묶음은 `Full3`". `AGENTS.md:20`과 `posting.md:3`이 이 문서를 "글 대기열"로 가리킨다 |
| `Docs/post-material.md`(64KB) | `v1` 글 재료의 기록 | `:7` "`Full1`의 값으로", `:9` 절 번호가 `v1` 글 번호(`## 01`\~`## 08`), `:191` "08 총괄", `:211` 전환 거리 40m(ADR-0021로 35m), `:333`\~`:351` `Full1` 값. `v2`의 W9 번쩍임 재료(`:307`\~`:327`)만 지금 것이다. `AGENTS.md:31`이 지금 쓰는 문서로 가리킨다 |
| `Docs/Decisions/README.md` | 작업 색인(맞다), 아래 표는 기록 | `:3` "plan.md에", `:48`\~`:49` 고정 중심 오브와 고정 카메라 프리셋(ADR-0017이 바꿨다), `:20`·`:29` 밴드 0\~40m(ADR-0021로 35m). ADR 0001\~0021과 표가 1:1로 맞는다 |
| `Docs/Worklog/README.md` | 작업 색인, 빠진 것 있음 | 링크 11개는 모두 살아 있다. `autonomous-decisions.md`, `Q5-hud.md`가 목록에 없다. `V2*` 파일 무리를 묶어 말하는 문장이 없다 |
| `Docs/Guides/commands.md` | 작업 문서, `v1` 절 섞임 | `:21` `run-m3b.ps1`, `:278` `run-e1.ps1`, `:279` `run-e2.ps1`은 `Scripts/Archive/`에만 있다. `:274`\~`:281` "벤치마크 실행(구현됨)"이 `v1` 표시 없이 `-Milestone M0`을 적는다 |
| `Measurements/README.md` | 낡음 | "마일스톤마다 `M<N>/`에" |

`v1` 글 폴더(`02-mass-logic` 등)를 가리키는 곳: Worklog 밖에서는 `Scripts/plot-post02-manager.py`(1, 7, 16행)뿐이다. Worklog에서는 `Posts.md` 32줄, `V2-w14-plan.md` 7줄, `V2.md` 6줄이다(기록이라 둔다).

선택지(작업 문서):

| 선택지 | 득 | 실 |
| --- | --- | --- |
| A. `plan.md`·`post-material.md` 머리에 "`v1` 기록, 지금 계획은 `v2-plan.md`" 한 줄을 넣고, `backlog.md`를 지우고, 색인 셋(`Worklog/README.md`, `Decisions/README.md`, `Measurements/README.md`)과 `commands.md`의 경로 셋을 고친다(추천) | 문서를 여는 사람이 무엇이 지금 것인지 안다. 작업량이 작다 | `backlog.md`를 가리키는 `AGENTS.md:20`, `posting.md:3`, `plan.md:10`을 함께 고친다 |
| B. `plan.md`·`post-material.md`를 `Docs/Worklog/`로 옮긴다 | `Docs/` 최상위가 `v2` 문서만 남는다 | 두 문서를 가리키는 링크(`v2-spec.md:12`, `AGENTS.md`, 가이드)를 모두 고친다 |
| C. 두다 | | 공개 레포를 여는 사람이 `v1` 계획을 지금 계획으로 읽는다 |

### 6.3 `Docs/Worklog/V2.md`(670,823바이트, 5,281줄)

- `##` 절 51개다. 크기는 W12 측정 준비 약 200KB, W14 절 25개 약 108KB, `Full4` 절 7개 약 92KB, W1\~W11 절이 각 4\~23KB다.
- 다른 파일이 304번 가리킨다(57개 파일). 그중 56번은 공개 글의 `measurements.md`(`Posts/0N-*/measurements.md`)다. 절은 앵커가 아니라 따옴표 속 절 이름("W12 측정 준비")으로 가리킨다(`V2.md#` 링크 0개).
- GitHub은 이 파일을 지금 그대로 렌더한다. `v2` 브랜치의 GitHub 페이지에서 `##` 51개가 모두 렌더된 것을 확인했다(마지막 절 "W14 팩트체크 결정과 표기 정리").

| 선택지 | 득 | 실 |
| --- | --- | --- |
| A. 두다(추천) | 304개의 인용이 그대로 맞다. GitHub도 렌더한다 | 한 파일이 크다 |
| B. `V2.md`를 색인으로 두고 W12·`Full4`·W14를 `V2-W12.md`, `V2-full4.md`, `V2-W14.md`로 나눈다 | 파일마다 100\~200KB가 된다 | 절 이름 인용 304개 가운데 옮긴 절을 가리키는 것을 찾아 고친다. 공개 글 `measurements.md` 8개도 고친다 |
| C. 끝난 W14 절만 `V2-W14.md`로 옮긴다 | 가장 최근 절(공개 자료 작업)이 한곳에 모인다 | B보다 작지만 같은 종류의 수정이 든다 |

### 6.4 `AGENTS.md` "문서 지도"

- 지도의 링크는 모두 실제 파일을 가리킨다.
- 지도가 이름으로 말하지 않는 Worklog 파일: `CodeTour.md`, `G1.md`, `Q5-hud.md`. `Worklog/README.md`로 넘기는 문장이 있어 빠진 것은 아니다.
- 6.2의 결정에 따라 고칠 곳: "글 대기열 → `backlog.md`"(`:20`), "글 재료 → `post-material.md`"(`:31`), "목표 사양 … 끝낸 단계의 목표와 실험 → `plan.md`"(`:17`).

### 6.5 `.gitignore`와 `.gitattributes`

- `.gitignore:8`\~`:9`: `Build/` 다음의 `!Build/**/*.ico`는 효과가 없다. 부모 폴더 `Build/`를 통째로 무시하면 git은 그 안의 예외를 보지 않는다(`git check-ignore -v`가 `.gitignore:8 Build/`를 보인다). `Build/`에 `.ico`도 없다. 두 줄을 `Build/` 하나로 줄이거나 그대로 둔다.
- `.gitignore`의 `Measurements/**/raw/`, `trace/`, `video/`: 지금 그런 폴더는 없다. 앞으로를 위한 줄이라 둔다.
- `.gitattributes`의 `*.gif`, `*.mp4`: 추적 파일이 0개다. `AGENTS.md`가 이미지(`.gif` 포함)를 LFS로 정하므로 둔다.
- 나머지 줄은 실제 경로와 맞다.

### 6.6 끊어진 상대 링크

추적되는 `.md`·`.html` 전부에서 상대 링크와 `src`·`href` 1,310개를 검사했다. 코드 블록 안은 뺐다.

| 위치 | 대상 | 판단 |
| --- | --- | --- |
| `Docs/Worklog/CodeTour.md:40` | `Docs/Guides/code-tour.md` | 다른 문서의 표 줄을 백틱 안에 옮겨 적은 것이다. 링크가 아니다 |
| `Docs/Worklog/Posts.md:157` | `…` | 백틱 안의 예시다 |
| `Docs/Worklog/Posts.md:546` | `Posts/09-summary/README.md` | 백틱 안의 제안 문장(`v1` 글 09)이다 |

실제로 끊어진 링크는 0개다. 셋은 모두 인라인 코드 안이라 GitHub에서 링크로 그려지지 않는다.

### 6.7 공개 레포에 남기에 어색한 것

- 추적되는 `progress.log` 39개, `failures.csv` 39개, `batch.json` 34개(`Measurements/`): `AGENTS.md`의 커밋 대상(`summary.csv`, `runs.csv`, 대표 스크린샷)보다 넓다. 텍스트라 LFS에는 영향이 없다. 지우면 `batch-check.py`나 `spikes.py`가 그 파일을 읽는지 먼저 확인해야 한다(확인하지 못했다).
- 공개 글의 `measurements.md` 8개가 `Docs/Worklog/V2.md`를 56번 링크한다. 그래서 독자가 작업 경위에 닿는다. 근거 규칙(`AGENTS.md` "모든 수치와 설정값에 근거를 적는다")에 따른 것이라 이 조사는 문제로 보지 않는다.
- `Docs/Worklog/autonomous-decisions.md`(127KB)는 코디네이터의 내부 결정 기록이다. 개인 정보나 비밀은 찾지 못했다(이름 검색만 했다).
- `Content/Developers/hon45/`는 추적 밖이라 공개되지 않는다.
- 임시 파일(`.tmp`, `.bak`, `.orig`, `~`)은 추적 파일에 없다.

### 묶음 6의 물음

1. `PoolSmoke1`\~`3`과 빈 `Reh3`를 어떻게 할까? `Saved/Rehearsal/`로 옮기기(A), 지우기(B), `.gitignore`에 더하기(C) 가운데 고른다.
2. 작업 문서를 어떻게 할까? 머리 표시와 색인 고치기(A), `Worklog/`로 옮기기(B), 두기(C) 가운데 고른다. `V2.md`를 나눌지(6.3)는 그다음에 정한다.

---

## 조사 방법

- 크기와 파일 목록: `git ls-files`, `git lfs ls-files -s`, `du`.
- 인용: 추적되는 텍스트 파일(`.md`, `.py`, `.ps1`, `.json`, `.html`, `.js`, `.h`, `.cpp` 등)을 줄 단위로 검색했다. 묶음은 이름, 스크린샷과 그림은 파일 이름(그림은 폴더 경로로 다시)으로 셌다.
- 링크: 마크다운 `[..](..)`와 HTML `src`·`href`의 상대 경로를 파일 위치 기준으로 풀어 `git ls-files`와 대조했다.
- 웹피: RIFF 청크(`VP8X`, `ANMF`)를 직접 읽어 해상도와 프레임 수를 셌다.
- `v1` 여부: `git ls-tree v1`에 있는지로 갈랐다.
- "아무도 안 쓴다"는 판정(`M2/shots/mass-N5000-P1.png`)은 `git grep`으로 한 번 더 확인했다.
- 세는 스크립트는 저장소 밖 스크래치 폴더에 있고 커밋하지 않았다.
