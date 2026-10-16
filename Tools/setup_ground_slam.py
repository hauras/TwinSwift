# 단계 7-2 내려찍기 에셋: 몽타주(+착지 신호) + 능력 블루프린트 + 전사 시작 능력에 추가
#   AM_Warrior_GroundSlam: A_Knight_JumpAttack (제자리 버전)
#     0~0.5초 준비 → 0.5~1.0초 작은 점프 → 1.0~1.23초 내려찍기 → 1.23초 칼끝이 바닥 (높이 20cm)
#     1.23초에 AN_SendEvent(Event.Montage.Hit) = 착지 신호
#   GA_Warrior_GroundSlam (부모 USGA_GroundSlam): 점프 시작(0.5초)부터 1배속
#     → 착지 신호까지 (1.23 - 0.5) / 1.0 = 0.73초. 날아가는 시간 0.7초 (최고 150cm 포물선, 신호 직전에 착지)
#     착지 후 0.45초에 능력 종료 → 뒤의 긴 마무리(1.23~2.17초)는 블렌드 아웃으로 자름
#   처음 버전(0.3초부터 1.2배속, 높이 없음)은 바닥을 미끄러지는 돌진처럼 보여서 바꿈
# 조건: USGA_GroundSlam, UAN_SendEvent가 들어간 C++이 빌드돼 있어야 함. 다시 실행해도 같은 결과
import unreal

eal = unreal.EditorAssetLibrary
anim_lib = unreal.AnimationLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

ANIM = "/Game/Assets/External/Characters/Knight_Free/Animations/A_Knight_JumpAttack"
MONTAGE = "/Game/Assets/Characters/Warrior/Montages/AM_Warrior_GroundSlam"
ABILITY = "/Game/Abilities/Warrior/GA_Warrior_GroundSlam"
IMPACT_TIME = 1.23
TRACK = "Event"
SETTINGS = dict(
    max_range=800.0,
    leap_duration=0.7,
    leap_height=150.0,
    recovery_time=0.45,
    radius=300.0,
    damage_multiplier=2.2,
    crack_duration=8.0,
    montage_play_rate=1.0,
    montage_start_time=0.5,
)


def load(path):
    return unreal.load_asset(path + "." + path.split("/")[-1])


# ---------- 몽타주 ----------
if eal.does_asset_exist(MONTAGE):
    montage = load(MONTAGE)
else:
    factory = unreal.AnimMontageFactory()
    factory.set_editor_property("source_animation", load(ANIM))
    folder, name = MONTAGE.rsplit("/", 1)
    montage = tools.create_asset(name, folder, unreal.AnimMontage, factory)
for prop, value in (("blend_in", 0.08), ("blend_out", 0.2)):
    blend = montage.get_editor_property(prop)
    blend.set_editor_property("blend_time", value)
    montage.set_editor_property(prop, blend)

# 착지 신호: 다시 실행해도 겹치지 않게 트랙을 비우고 다시 깜
if TRACK not in [str(n) for n in anim_lib.get_animation_notify_track_names(montage)]:
    anim_lib.add_animation_notify_track(montage, TRACK)
anim_lib.remove_animation_notify_events_by_track(montage, TRACK)
notify = anim_lib.add_animation_notify_event(montage, TRACK, IMPACT_TIME, unreal.AN_SendEvent)
tag = unreal.GameplayTag()
tag.import_text('(TagName="Event.Montage.Hit")')
notify.set_editor_property("event_tag", tag)
# 착지 충격: Knife_light 반달을 바닥에 눕혀 두 개(반대 방향)로 원을 만듦 → 타격 범위(반경 3m)가 보임
# 반달 이펙트는 XY 평면의 반지름 100 원을 반 바퀴 쓸고 지나감 (setup_slash_fx.py 참고) → 0도 + 180도 = 한 바퀴
# 캐릭터에 붙이지 않음(attached False) → 그 자리 바닥에 남음. 메시 원점 = 발밑
IMPACT_FX = "/Game/Assets/External/VFX/Knife_light/VFX/NE_attack05"
IMPACT_TRACK = "Impact"
ring_scale = SETTINGS["radius"] / 100.0
# 반달 하나에 트랙 하나 (같은 트랙에 같은 시간으로 두 개를 넣으면 엔진이 다른 트랙으로 옮겨 버림)
# 몽타주 기본 트랙("1")에 잘못 들어간 예전 충격 이펙트도 지움 (기본 트랙엔 다른 노티파이를 두지 않음)
anim_lib.remove_animation_notify_events_by_track(montage, "1")
for i, yaw in enumerate((0.0, 180.0)):
    track = f"{IMPACT_TRACK}{i + 1}"
    if track not in [str(n) for n in anim_lib.get_animation_notify_track_names(montage)]:
        anim_lib.add_animation_notify_track(montage, track)
    anim_lib.remove_animation_notify_events_by_track(montage, track)
    fx = anim_lib.add_animation_notify_event(montage, track, IMPACT_TIME - 0.02, unreal.AnimNotify_PlayNiagaraEffect)
    fx.set_editor_property("template", load(IMPACT_FX))
    fx.set_editor_property("attached", False)
    fx.set_editor_property("location_offset", unreal.Vector(0.0, 0.0, 5.0))
    fx.set_editor_property("rotation_offset", unreal.Rotator(0.0, 0.0, yaw))
    fx.set_editor_property("scale", unreal.Vector(ring_scale, ring_scale, ring_scale))

eal.save_loaded_asset(montage, only_if_is_dirty=False)
print(MONTAGE.split("/")[-1], "착지 충격 원", IMPACT_FX.split("/")[-1], "크기", ring_scale)
print(MONTAGE.split("/")[-1], "착지 신호", IMPACT_TIME, "초", notify.get_editor_property("event_tag").export_text())

# ---------- 능력 블루프린트 ----------
folder, name = ABILITY.rsplit("/", 1)
if eal.does_asset_exist(ABILITY):
    bp = load(ABILITY)
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.SGA_GroundSlam)
    bp = tools.create_asset(name, folder, unreal.Blueprint, factory)
cdo = unreal.get_default_object(bp.generated_class())
for key, value in SETTINGS.items():
    cdo.set_editor_property(key, value)
cdo.set_editor_property("slam_montage", montage)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
eal.save_loaded_asset(bp, only_if_is_dirty=False)
print(name, {k: round(cdo.get_editor_property(k), 2) for k in SETTINGS}, "몽타주", cdo.get_editor_property("slam_montage").get_name())

# ---------- 전사 시작 능력에 추가 (이미 있으면 그대로) ----------
player_bp = load("/Game/Blueprints/BP_Player")
player = unreal.get_default_object(player_bp.generated_class())
ability = bp.generated_class()
config = player.get_editor_property("warrior_config")
text = config.export_text()
current = [s for s in text.split("StartupAbilities=(")[1].split("),DefaultAttributesEffect")[0].split(",") if s] if "StartupAbilities=(" in text else []
if not any(ability.get_path_name() in c for c in current):
    current.append(f'"{ability.get_path_name()}"')
effect = text.split("DefaultAttributesEffect=")[1].rstrip(")") if "DefaultAttributesEffect=" in text else "None"
new = unreal.TwinSlotConfig()
# 구조체 칸이 EditDefaultsOnly라 텍스트로 한 번에 넣음
new.import_text(f"(StartupAbilities=({','.join(current)}),DefaultAttributesEffect={effect})")
player.set_editor_property("warrior_config", new)
print("warrior_config", player.get_editor_property("warrior_config").export_text())
unreal.BlueprintEditorLibrary.compile_blueprint(player_bp)
eal.save_loaded_asset(player_bp, only_if_is_dirty=False)
