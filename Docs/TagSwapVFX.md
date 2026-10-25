# 태그 교체 이펙트

## 에셋
- `/Game/Assets/VFX/Common/Tag/NS_TagSwap_ToMage`: 주황 수렴광 → 파란 파동과 작은 입자.
- `/Game/Assets/VFX/Common/Tag/NS_TagSwap_ToWarrior`: 파란 수렴광 → 주황 파동과 작은 입자.
- 공통 머티리얼: `M_TagSwap_Ring`, `M_TagSwap_Gather`, `M_TagSwap_Spark`.

수렴광 0.1초, 파동은 0.04초 뒤 시작해서 0.22초 유지.
단발 입자 총 14개(수렴광 1, 파동 1, 작은 입자 12), CPU 시뮬레이션.
원 크기는 190cm 쿼드 내부에서 최대 반경 약 86cm.
외부 텍스처 없이 UV와 ParticleRelativeTime으로 원의 확산·감쇠를 계산한다.
각 방향은 같은 제작 스크립트와 머티리얼을 공유하고 입자 색만 다르다.

## 캐릭터 연결
`APlayerCharacter::SwapTo()`에서 상태가 바뀐 직후 한 번 재생한다.
같은 캐릭터 선택·공격 중 교체 차단 시 효과가 발생하지 않는다.
월드 발밑에 재생하므로 이동해도 원이 캐릭터를 따라오지 않는다.
BeginPlay에서 두 에셋을 읽어 첫 교체 시 파일 로딩을 피한다.
BP_Player → Twin / VFX에서 두 방향의 에셋을 바꾸거나 비워 연출을 끌 수 있다.

## 적용 범위와 확인
새 C++ 연결은 TwinSwiftEditor 빌드가 필요하다.
CLAUDE.md의 '빌드는 사용자가 한다'에 따라 C++ 빌드를 실행하지 않았다.
새 C++의 실제 태그 재생과 최종 시각 확인은 빌드 후 필요하다.
어시스트 분신·공격 판정·타이밍 교체 슬로 모션은 이번 효과에 포함되지 않는다.

제작: `Tools/VFX/create_tag_swap_fx.py`.
제작 때만 엔진 CascadeToNiagaraConverter API를 임시 로드한다.
최종 에셋은 기본 Niagara 모듈만 사용하며 프로젝트에 플러그인을 영구 활성화하지 않았다.
