# ABP_LabMinion 시작 시각 핀 연결 (M3)

M3에서 VAT 인스턴스가 액터로 바뀌는 순간, 액터의 애니메이션을 VAT가 재생하던 시각부터 이어서 재생하려고 `ABP_LabMinion`을 고친 순서다(2026-11-08, Unreal MCP를 HTTP로 직접 불러 실행). 다시 만들 때 같은 순서를 따른다.

전제: `ULabMinionAnimInstance::StartTime`(C++, `Source/HordeOptLab/LabMinionAnimInstance.h`)이 빌드돼 있고, 에디터가 MCP 서버를 연 상태다(`-nullrhi`도 된다, [commands.md](../Docs/Guides/commands.md) "Unreal MCP를 HTTP로 직접 부르기"). 툴은 `call_tool`에 `toolset_name`·`tool_name`·`arguments`로 부른다.

1. 그래프의 시퀀스 플레이어 4개(`AnimGraph.AnimGraphNode_SequencePlayer_0`\~`_3`. 걷기 둘, 피격, 죽기)마다 `ObjectTools.get_properties`로 `ShowPinForProperties`를 읽고, `propertyName`이 `StartPosition`인 항목의 `bShowPin`을 `true`로 바꿔 `ObjectTools.set_properties`(`values`에 JSON 문자열)로 쓴다. 노드에 입력 핀 `StartPosition`이 생긴다.
2. `BlueprintTools.create_node`로 `Variables|Lab|GetStartTime` 노드를 플레이어마다 하나씩 만들고(`pos`는 정수 좌표), `BlueprintTools.connect_pins`로 그 출력 핀을 플레이어의 `StartPosition` 입력 핀(index 0)에 잇는다. 노드 종류 이름은 `BlueprintTools.find_node_types`(`type_id_filter` "StartTime", `context_pins`에 그 입력 핀)로 찾았다.
3. `AnimGraphNode_BlendListByEnum_0`의 `Node.childUpateMode`(엔진 철자 그대로)를 `ResetChildOnActivate`로 바꾼다(`set_properties`의 `values`에 `{"Node":{"childUpateMode":"ResetChildOnActivate"}}`). 기본값 `Default`는 다시 활성화된 클립을 처음부터 시작하지 않아(`AnimNode_BlendListBase.h:23-33`), 풀에서 다시 쓰는 액터의 죽기 클립이 끝난 자리에서 이어진다. 클립이 초기화될 때 `StartPosition`부터 재생한다(`AnimNode_SequencePlayer.cpp:61`).
4. `BlueprintTools.compile_blueprint`, `AssetTools.save_assets`(`asset_paths: ["/Game/Lab/Anim/ABP_LabMinion"]`).

M0 `full`에서는 `StartTime`이 늘 0이고 각 클립이 처음 활성화될 때만 재생되므로 동작이 그대로다.
