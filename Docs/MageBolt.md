# 마법사 투사체 공격 (화염탄·냉기탄)

다른 세션이 처음 작성한 `UGA_MageBolt` / `AMageBoltProjectile`을 프로젝트 규칙에 맞게 정리함 (2026-10-16).

## 바뀐 점
| 처음 | 지금 | 이유 |
|---|---|---|
| `UGA_MageBolt` (마탄 전용 → 기본 공격은 화염탄으로 바꿈) | `USGA_ProjectileAttack` | 화염탄·냉기탄을 설정값만 바꿔 같은 클래스로 (회피 `USGA_Dodge`와 같은 방식) |
| `AMageBoltProjectile` | `ASProjectile` | 투사체 공통 부모. 속도·크기·이펙트는 블루프린트 설정 |
| 애니 시퀀스 + 동적 몽타주 + 발사 지연 타이머 | 몽타주 에셋 + 발사 신호 `AN_SendEvent` + `PlayMontageAndWait` | 재생 속도를 바꿔도 손 뻗는 프레임에 발사. 타이머·수동 정리 코드가 없어짐 |
| 생성자 `ConstructorHelpers`로 에셋 경로 고정 | 블루프린트 칸 | 경로가 바뀌어도 코드 수정 없음 |
| `PlayerCharacter`가 마탄을 자동 지급 | `BP_Player`의 `MageConfig` 시작 능력 | 다른 능력과 같은 경로 하나로 |
| 데미지 스펙 하나 | 스펙 배열 (데미지 + 표식) | 냉기탄 [빙결]을 같은 투사체로 |

## 그대로 가져온 것
- 투사체 충돌: 적(Pawn) 겹침 감지, 벽 막힘, 쏜 캐릭터·같은 편 통과, 명중 한 번만, 권한에서만 피해
- 발사 순간 스펙에 공격력이 담겨서 날아가는 중에 교체해도 마법사 공격력 유지
- 이펙트 `Content/VFX/Mage/NS_MageBolt_Launch/Flight/Impact` (`Tools/VFX/create_mage_bolt_fx.py`)

## 에셋 (`Tools/setup_mage_attacks.py`)
- 화염탄: `GA_Mage_FireBolt` + `AM_Mage_FireBolt`(`A_Wizard_1HMagicAttack_02`, 0.53초 발사) + `BP_Projectile_FireBolt` (1800cm/s, 이펙트 `Content/Assets/Effects/Projectiles`의 `NS_Fireball`·`NS_FireExplosion`)
- 냉기탄: `GA_Mage_FrostBolt` + `AM_Mage_FrostBolt`(`A_Wizard_1HMagicAttack_01`, 0.80초 발사) + `BP_Projectile_FrostBolt` (1400cm/s, [빙결] 6초)

## 다음
- 오브젝트 풀 `USProjectilePool`: `ASProjectile::Resolve`의 `SetLifeSpan` 대신 반납
