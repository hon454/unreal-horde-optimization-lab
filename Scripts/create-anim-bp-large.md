# ABP_LabMinionLarge 만들기 (M3b 2종)

M3b 2종의 대형(시즈 미니언) AnimBP `/Game/Lab/Anim/ABP_LabMinionLarge`를 만든 순서다(2026-11-09). 소형 `ABP_LabMinion`을 복제해 스켈레톤과 클립만 바꾸므로, 그래프 구조, 시작 시각 핀 연결([bind-anim-start.md](bind-anim-start.md)), 블렌드 리스트의 `ResetChildOnActivate`가 그대로 온다.

1. 복제와 스켈레톤(Python 커맨드렛, 에디터가 떠 있어도 된다): `LAB_UNIT=large`로 [create-anim-bp.py](create-anim-bp.py)를 돌린다. `EditorAssetLibrary.duplicate_asset`로 `ABP_LabMinion`을 복제하고 `target_skeleton`을 `Minion_Lane_Siege_Skeleton`으로 바꿔 저장한다(`UAnimBlueprint::TargetSkeleton`은 `EditAnywhere`, `AnimBlueprint.h:90-91`). Unreal MCP의 `ObjectTools`는 블루프린트 경로를 CDO로 바꿔 읽어 이 프로퍼티에 닿지 않는다.

   ```bash
   LAB_UNIT=large "G:/Epic Games/UE_5.8.4/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "G:/Epic Games/UE_5.8.4/Games/unreal-horde-optimization-lab/HordeOptLab.uproject" -run=pythonscript -script="G:/Epic Games/UE_5.8.4/Games/unreal-horde-optimization-lab/Scripts/create-anim-bp.py" -unattended -nosplash -nullrhi -stdout
   ```

2. 클립 바꾸기(Unreal MCP, 에디터): Python은 애님 그래프 노드에 닿지 않는다. 시퀀스 플레이어 4개(`ABP_LabMinionLarge.ABP_LabMinionLarge:AnimGraph.AnimGraphNode_SequencePlayer_N`)마다 `ObjectTools.set_properties`로 `values`에 `{"Node":{"sequence":{"refPath":"<애니 경로>"}}}`를 쓴다. `_0`·`_3` 걷기 `Animations/Siege/Jog_Fwd_NonCombat`, `_1` 피격 `Animations/Siege/HitReact_Front`, `_2` 죽기 `Animations/Siege/Death_A`(모두 `/Game/ParagonMinions/Characters/Minions/Down_Minions/` 아래). 노드 번호와 클립의 짝은 소형과 같다(`get_properties`의 `Node.sequence`로 확인).
3. `BlueprintTools.compile_blueprint`, `AssetTools.save_assets`(`asset_paths: ["/Game/Lab/Anim/ABP_LabMinionLarge"]`).

확인: 컴파일 로그에 오류 없음, `get_node_infos`로 각 플레이어의 `StartPosition` 입력이 `StartTime` 변수 노드에 이어져 있음, 에셋 태그 `TargetSkeleton`이 `Minion_Lane_Siege_Skeleton`, `ParentClass`가 `LabMinionAnimInstance`.
