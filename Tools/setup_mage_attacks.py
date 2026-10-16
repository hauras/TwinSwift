# 단계 7-B-1 마법사 투사체 공격 에셋: 몽타주(+발사 신호) + 투사체 블루프린트 + 능력 블루프린트 + 마법사 시작 능력
#   화염탄 (좌클릭): A_Wizard_1HMagicAttack_02 — 0.53초에 오른손을 앞으로 뻗음 (손 속도 최고)
#     0.25초부터 1.5배속 → 누르고 (0.53 - 0.25) / 1.5 = 0.19초에 발사. 쿨 0.45초
#   냉기탄 (3): A_Wizard_1HMagicAttack_01 — 0.67초 왼손, 0.80초 오른손이 앞으로 (오른손에서 발사)
#     0.3초부터 1.3배속 → 0.38초에 발사. 관통, 같은 적 0.5배 × 3번, 첫 타에 [빙결] 6초. 쿨 6초
# 이펙트: 화염탄 = Content/Assets/Effects/Projectiles의 NS_Fireball(비행)·NS_FireExplosion(명중), 발사 이펙트 없음
#         냉기탄(얼음 쇄도, USGA_CrystalTorrent) = NS_Crystal_Torrent. 투사체 없이, 얼음 앞쪽 끝까지의 띠를 0.05초마다 검사
#                 (얼음이 처음엔 느리고 점점 빨라져서 일정 속도 투사체로는 판정이 먼저 도착했음 → 잰 커브로 판정을 맞춤)
#         화염탄은 작고 빠르게, 화상·폭발 범위 없음 (표식 시스템과 겹치지 않게 보기만 불)
# 조건: USGA_ProjectileAttack, USGA_CrystalTorrent, ASProjectile이 들어간 C++이 빌드돼 있어야 함. 다시 실행해도 같은 결과
import unreal

eal = unreal.EditorAssetLibrary
anim_lib = unreal.AnimationLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

ANIM_DIR = "/Game/Assets/External/Characters/Wizard_Free/Animations"
MONTAGE_DIR = "/Game/Assets/Characters/Mage/Montages"
PROJECTILE_DIR = "/Game/Blueprints/Projectile"
ABILITY_DIR = "/Game/Abilities/Mage"
FX = "/Game/Assets/VFX/Mage"


def load(path):
    return unreal.load_asset(path + "." + path.split("/")[-1])


def tag(name):
    t = unreal.GameplayTag()
    t.import_text(f'(TagName="{name}")')
    return t


def tags(*names):
    c = unreal.GameplayTagContainer()
    c.import_text("(GameplayTags=(" + ",".join(f'(TagName="{n}")' for n in names) + "))")
    return c


def blueprint(path, parent):
    if eal.does_asset_exist(path):
        bp = load(path)
        # 부모 클래스가 바뀌었으면 (예: 냉기탄 투사체 → 얼음 쇄도) 다시 연결
        if not isinstance(unreal.get_default_object(bp.generated_class()), parent):
            unreal.BlueprintEditorLibrary.reparent_blueprint(bp, parent)
        return bp
    folder, name = path.rsplit("/", 1)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    return tools.create_asset(name, folder, unreal.Blueprint, factory)


def montage(name, anim_name, fire_time):
    path = f"{MONTAGE_DIR}/{name}"
    if eal.does_asset_exist(path):
        m = load(path)
    else:
        factory = unreal.AnimMontageFactory()
        factory.set_editor_property("source_animation", load(f"{ANIM_DIR}/{anim_name}"))
        m = tools.create_asset(name, MONTAGE_DIR, unreal.AnimMontage, factory)
    for prop, value in (("blend_in", 0.08), ("blend_out", 0.15)):
        blend = m.get_editor_property(prop)
        blend.set_editor_property("blend_time", value)
        m.set_editor_property(prop, blend)
    # 발사 신호: 다시 실행해도 겹치지 않게 트랙을 비우고 다시 깜
    if "Event" not in [str(n) for n in anim_lib.get_animation_notify_track_names(m)]:
        anim_lib.add_animation_notify_track(m, "Event")
    anim_lib.remove_animation_notify_events_by_track(m, "Event")
    notify = anim_lib.add_animation_notify_event(m, "Event", fire_time, unreal.AN_SendEvent)
    notify.set_editor_property("event_tag", tag("Event.Montage.Hit"))
    eal.save_loaded_asset(m, only_if_is_dirty=False)
    print(name, anim_name, "발사 신호", fire_time, "초")
    return m


def projectile(name, speed, radius, scale, flight_fx, impact_fx):
    bp = blueprint(f"{PROJECTILE_DIR}/{name}", unreal.SProjectile)
    cdo = unreal.get_default_object(bp.generated_class())
    movement = cdo.get_editor_property("movement")
    movement.set_editor_property("initial_speed", speed)
    movement.set_editor_property("max_speed", speed)
    cdo.get_editor_property("collision").set_editor_property("sphere_radius", radius)
    flight = cdo.get_editor_property("flight_effect")
    flight.set_editor_property("asset", load(flight_fx))
    flight.set_editor_property("relative_scale3d", unreal.Vector(scale, scale, scale))
    cdo.set_editor_property("impact_effect", load(impact_fx))
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp, only_if_is_dirty=False)
    print(name, "속도", speed, "반지름", radius, "크기", scale)
    return bp


def ability(name, parent, settings, montage_asset, launch_fx, extra):
    bp = blueprint(f"{ABILITY_DIR}/{name}", parent)
    cdo = unreal.get_default_object(bp.generated_class())
    for key, value in settings.items():
        if key == "ability_tags":
            # 능력 이름표: 엔진 버전에 따라 파이썬 이름이 ability_tags / asset_tags
            for prop in ("ability_tags", "asset_tags"):
                try:
                    cdo.set_editor_property(prop, value)
                    break
                except Exception:
                    continue
            continue
        cdo.set_editor_property(key, value)
    cdo.set_editor_property("cast_montage", montage_asset)
    cdo.set_editor_property("launch_effect", load(launch_fx) if launch_fx else None)
    for key, value in extra.items():
        cdo.set_editor_property(key, value)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp, only_if_is_dirty=False)
    print(name, cdo.get_editor_property("input_tag").export_text(), "쿨", cdo.get_editor_property("cooldown_duration"),
          "배율", cdo.get_editor_property("damage_multiplier"), "표식", cdo.get_editor_property("mark_tag").export_text())
    return bp


# ---------- 몽타주 ----------
fire_montage = montage("AM_Mage_FireBolt", "A_Wizard_1HMagicAttack_02", 0.53)
frost_montage = montage("AM_Mage_FrostBolt", "A_Wizard_1HMagicAttack_01", 0.80)

# ---------- 투사체 ----------
FIRE_FX = "/Game/Assets/External/VFX/Effects/Projectiles"
fire_projectile = projectile("BP_Projectile_FireBolt", 1800.0, 12.0, 0.5,
                             f"{FIRE_FX}/FireBolt/NS_Fireball", f"{FIRE_FX}/FireExplosion/NS_FireExplosion")

# 예전 냉기탄 투사체 (얼음 쇄도로 바뀌며 안 씀)
if eal.does_asset_exist(f"{PROJECTILE_DIR}/BP_Projectile_FrostBolt"):
    eal.delete_asset(f"{PROJECTILE_DIR}/BP_Projectile_FrostBolt")

# ---------- 능력 ----------
abilities = [
    ability("GA_Mage_FireBolt", unreal.SGA_ProjectileAttack, dict(
        input_tag=tag("InputTag.Attack"),
        ability_tags=tags("Ability.Mage.FireBolt"),
        cooldown_duration=0.45,
        cooldown_tags=tags("Cooldown.Mage.FireBolt"),
        # 스킬 시전 중(State.Attacking)에는 화염탄이 나가지 않음 → 안 막으면 화염탄 몽타주가 스킬 몽타주를 끊어서
        # 스킬은 쿨다운만 돌고 효과가 안 나감. 반대로 스킬은 화염탄을 끊고 나감 (CancelAbilitiesWithTag)
        activation_blocked_tags=tags("State.Stun", "State.Dead", "State.Attacking"),
        damage_multiplier=1.0,
        recovery_time=0.15,
        montage_play_rate=1.5,
        montage_start_time=0.25,
    ), fire_montage, None, dict(projectile_class=fire_projectile.generated_class())),
    # 얼음 쇄도: 입력·이름표·쿨·배율(0.5 × 3번)·표식·앞쪽 끝 커브는 C++ 생성자 기본값
    ability("GA_Mage_FrostBolt", unreal.SGA_CrystalTorrent, dict(
        input_tag=tag("InputTag.Mage.Skill1"),
        ability_tags=tags("Ability.Mage.FrostBolt"),
        cooldown_duration=6.0,
        cooldown_tags=tags("Cooldown.Mage.FrostBolt"),
        damage_multiplier=0.5,
        mark_tag=tag("Mark.Frost"),
        mark_duration=6.0,
        recovery_time=0.25,
        montage_play_rate=1.3,
        montage_start_time=0.3,
    ), frost_montage, None, dict(torrent_effect=load("/Game/Assets/External/VFX/Mixed_Magic_VFX_Pack/VFX/NS_Crystal_Torrent"))),
]

# ---------- 마법사 시작 능력에 추가 (이미 있으면 그대로) ----------
player_bp = load("/Game/Blueprints/BP_Player")
player = unreal.get_default_object(player_bp.generated_class())
config = player.get_editor_property("mage_config")
text = config.export_text()
current = [s for s in text.split("StartupAbilities=(")[1].split("),DefaultAttributesEffect")[0].split(",") if s] if "StartupAbilities=(" in text else []
for bp in abilities:
    path = bp.generated_class().get_path_name()
    if not any(path in c for c in current):
        current.append(f'"{path}"')
effect = text.split("DefaultAttributesEffect=")[1].rstrip(")") if "DefaultAttributesEffect=" in text else "None"
new = unreal.TwinSlotConfig()
# 구조체 칸이 EditDefaultsOnly라 텍스트로 한 번에 넣음
new.import_text(f"(StartupAbilities=({','.join(current)}),DefaultAttributesEffect={effect})")
player.set_editor_property("mage_config", new)
print("mage_config", player.get_editor_property("mage_config").export_text())
unreal.BlueprintEditorLibrary.compile_blueprint(player_bp)
eal.save_loaded_asset(player_bp, only_if_is_dirty=False)
