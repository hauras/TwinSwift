# 콘텐츠 폴더 규칙

2026-10-25 정리. 아래 경로는 Unreal Content Browser의 Content 기준이다.

| 폴더 | 용도 |
|---|---|
| Assets/Characters/Warrior | 전사 메시·스켈레톤·ABP·몽타주·시퀀스·재질 |
| Assets/Characters/Mage | 마법사 캐릭터 리소스 |
| Assets/Characters/Boss/Golem | 골렘 캐릭터 리소스 |
| Assets/Characters/Boss/Conductor | 지휘자 캐릭터 리소스 |
| Assets/VFX/Common | 교체·표식·어시스트 등 공통 전투 이펙트 |
| Assets/VFX/Warrior, Mage | 직업별 이펙트 |
| Assets/VFX/Boss/Common, Golem, Conductor | 보스 공통 및 보스별 이펙트 |
| Assets/External | 내려받은 외부 팩과 팩의 데모 콘텐츠 |
| Blueprints/Core | BP_GameMode, BP_PlayerController |
| Blueprints/Characters | BP_Player |
| Blueprints/Boss | 보스·전투 진행·장판 예고·탄막 매니저 |
| Blueprints/AI/Golem, Conductor | 보스별 BB와 BT |
| Blueprints/Projectile | 투사체 BP |
| Blueprints/Assist | 어시스트 BP |
| Blueprints/Debug | 테스트 더미·벤치마크 BP |
| Abilities | GA·GE 등 능력 설정 |
| UI | WBP·UI 아이콘·UI 재질 |
| Input, Camera, Maps | 입력·카메라 설정·플레이 맵 |
| Developers/Shared/Backups | 사용 중인 리소스와 분리한 에디터 백업 |

새 외부 팩은 Assets/External 아래에 팩 단위로 넣는다. 실제 게임에 맞춰 복제·수정한 리소스는 담당 캐릭터 또는 VFX 폴더에 둔다.
ABP·몽타주·시퀀스는 해당 캐릭터의 Assets 폴더에 둔다. 새 Blueprint를 만들 때에는 위 역할별 Blueprints 폴더를 사용한다.
에셋 이동은 Content Browser의 Move 또는 Unreal AssetTools로 한다. 탐색기로 uasset을 옮기지 않는다. 이동 뒤 스크립트의 /Game 경로와 리다이렉터도 함께 정리한다.

## 이번 작업 확인

- 이동 45개, 작업 스크립트·문서 경로 갱신.
- 실제 에셋 546개 유지, 프로젝트 Blueprint 44개 컴파일.
- GameMode의 플레이어·컨트롤러 클래스, 골렘·지휘자의 메시·ABP·BT·시작 능력 참조 확인.
- PIE에서 BP_Player, ABP_Knight와 WBP_TwinHUD 인스턴스 로드 확인.
- C++ 소스 수정·빌드·커밋·푸시 없음.
- 이동 전 백업: Saved/ContentOrganizationBackup/20261025_130012 (Content, Source, Config, Tools, Docs).
- 이동 매핑: Saved/ContentOrganizationMoves.json. 에셋 검증: Saved/ContentOrganizationVerification.json.

## 기존 문제로 별도 확인할 항목

지휘자 BT는 폴더 정리 전 백업에서도 BB_Boss를 참조하고, 패턴 Sequence·공격·Wait 노드 인스턴스가 저장돼 있지 않았다. 정리 후 PIE에서도 타깃 탐색은 되지만 공격 몽타주와 탄막은 시작하지 않았다. 이번 폴더 정리는 기존 BT 구성을 변경하지 않았다. 수정 시 BT_Conductor의 실제 노드 인스턴스를 복원하고 에디터 재실행 후에도 유지되는지 확인해야 한다.

전사 base_basic_shaded의 /Game/Asset/Knight_Free/model 누락 참조 경고도 이동 전부터 존재했다. 해당 경로에 에셋을 새로 만들거나 임의 재질로 바꾸지 않았다.
