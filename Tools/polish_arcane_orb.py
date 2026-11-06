"""구체용 원본 복사·몽타주 제작. 새 DLL 반영 후 VFX 정리와 BP 연결까지 실행."""
import unreal as u
eal = u.EditorAssetLibrary
tools = u.AssetToolsHelpers.get_asset_tools()
if u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world():
    raise RuntimeError('PIE 종료 후 실행')

folder = '/Game/Assets/VFX/Mage/ArcaneOrb'
source = '/Game/Assets/External/VFX/Mixed_Magic_VFX_Pack/VFX'
backup_folder = '/Game/Developers/Shared/Backups/BeforeArcaneOrbPolish'

def backup(asset):
    path = backup_folder+'/'+asset.get_name()
    if not eal.does_asset_exist(path):
        if not eal.duplicate_asset(asset.get_path_name(), path):
            raise RuntimeError('백업 실패: '+path)

systems = []
for original, name in [('NS_Dark_Solo_Projectile','NS_ArcaneOrb_Charge'),
                       ('NS_Magic_Big_Bubbles_Explosion','NS_ArcaneOrb_Explosion')]:
    path = folder+'/'+name
    if not eal.does_asset_exist(path):
        system = eal.duplicate_asset(source+'/'+original,path)
        if not system:
            raise RuntimeError('VFX 복사 실패')
        report = u.TSWeaponTrailLibrary.tint_system_hue(system,205.0,True)
        if 'ERROR' in report:
            raise RuntimeError(report[-200:])
        eal.save_loaded_asset(system,only_if_is_dirty=False)
    systems.append(u.load_asset(path))
    print('VFX 복사본:',path)

def montage(name, anim_name):
    directory = '/Game/Assets/Characters/Mage/Montages'
    path = directory+'/'+name
    animation = u.load_asset('/Game/Assets/External/Characters/Wizard_Free/Animations/'+anim_name)
    if not animation:
        raise RuntimeError('애니메이션 없음: '+anim_name)
    if eal.does_asset_exist(path):
        result = u.load_asset(path)
        backup(result)
    else:
        factory = u.AnimMontageFactory()
        factory.set_editor_property('source_animation',animation)
        result = tools.create_asset(name,directory,u.AnimMontage,factory)
    for prop, time in [('blend_in',0.08),('blend_out',0.12)]:
        blend = result.get_editor_property(prop)
        blend.set_editor_property('blend_time',time)
        result.set_editor_property(prop,blend)
    lib = u.AnimationLibrary
    if 'OrbCast' not in [str(n) for n in lib.get_animation_notify_track_names(result)]:
        lib.add_animation_notify_track(result,'OrbCast')
    lib.remove_animation_notify_events_by_track(result,'OrbCast')
    notify = lib.add_animation_notify_event(result,'OrbCast',0.78,u.AN_SendEvent)
    tag = u.GameplayTag()
    tag.import_text('(TagName="Event.Montage.Hit")')
    notify.set_editor_property('event_tag',tag)
    eal.save_loaded_asset(result,only_if_is_dirty=False)
    print('몽타주:',path,'발동 0.78초')
    return result

install = montage('AM_Mage_ArcaneOrb_Install','A_Wizard_2HCastSpell_01')
detonate = montage('AM_Mage_ArcaneOrb_Detonate','A_Wizard_1HCastSpell_01')
orb_bp = u.load_asset('/Game/Blueprints/Skills/BP_ArcaneOrb')
ga_bp = u.load_asset('/Game/Abilities/Mage/GA_Mage_ArcaneOrb')
orb = u.get_default_object(orb_bp.generated_class())
ga = u.get_default_object(ga_bp.generated_class())
try:
    orb.get_editor_property('orb_effect')
    ga.get_editor_property('install_montage')
    prepare = u.TSWeaponTrailLibrary.prepare_orb_effect
except Exception:
    print('ARCANE_ORB_PRESENTATION_PREPARED_BUILD_REQUIRED: 새 C++ 빌드 후 같은 스크립트 재실행')
else:
    keeps = [
        ['Dark_Core','Radial_Wave','Lightning','Start_Dark_Core','Start_Radial_Wave','Start_Refraction'],
        ['Brust_Bom_1','Start_Refraction','Start_Spark','Star_Glow','Ring_Reflect',
         'Magic_Smoke_Explosion','Brust_Bom','Sparks_Color']
    ]
    for system,names,cap in zip(systems,keeps,[1.0,0.7]):
        report = prepare(system,names,cap)
        print(system.get_name(),report)
        if 'ERROR' in report or 'ENABLED=0' in report:
            raise RuntimeError('VFX 구성 확인 실패: '+report)
        eal.save_loaded_asset(system,only_if_is_dirty=False)
    for bp in [orb_bp,ga_bp]:
        backup(bp)
    effect = orb.get_editor_property('orb_effect')
    effect.set_editor_property('asset',systems[0])
    effect.set_editor_property('relative_scale3d',u.Vector(0.7,0.7,0.7))
    orb.set_editor_property('explosion_effect',systems[1])
    orb.set_editor_property('explosion_effect_scale',0.8)
    ga.set_editor_property('install_montage',install)
    ga.set_editor_property('detonate_montage',detonate)
    ga.set_editor_property('install_start_time',0.35)
    ga.set_editor_property('detonate_start_time',0.5)
    ga.set_editor_property('cast_play_rate',1.4)
    ga.set_editor_property('cast_recovery_time',0.18)
    for bp in [orb_bp,ga_bp]:
        u.BlueprintEditorLibrary.compile_blueprint(bp)
        eal.save_loaded_asset(bp,only_if_is_dirty=False)
    print('ARCANE_ORB_PRESENTATION_CONNECTED')
