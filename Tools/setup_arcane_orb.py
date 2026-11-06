"""TwinSwiftEditor 빌드 후 실행. 구체 BP·GA와 기존 우클릭·HUD 연결."""
import unreal as u

eal = u.EditorAssetLibrary
assets = u.AssetToolsHelpers.get_asset_tools()
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
if editor.get_game_world():
    raise RuntimeError('PIE 종료 후 실행')

# DLL에 새 클래스·속성이 없는 상태에서는 에셋을 전혀 수정하지 않음
orb_native = u.load_class(None, '/Script/TwinSwift.SArcaneOrb')
ga_native = u.load_class(None, '/Script/TwinSwift.SGA_ArcaneOrb')
if not orb_native or not ga_native:
    raise RuntimeError('TwinSwiftEditor 빌드 필요: 마력 구체 클래스가 DLL에 없음')
for name in ['min_damage_multiplier', 'max_damage_multiplier']:
    u.get_default_object(orb_native).get_editor_property(name)
u.get_default_object(ga_native).get_editor_property('orb_class')

def tag(name):
    value = u.GameplayTag()
    value.import_text(f'(TagName="{name}")')
    if not u.GameplayTagLibrary.is_gameplay_tag_valid(value):
        raise RuntimeError('등록 안 된 태그: ' + name)
    return value

secondary = tag('InputTag.SecondaryAttack')
tag('Cooldown.Mage.ArcaneOrb')

def required(path):
    obj = u.load_asset(path)
    if not obj:
        raise RuntimeError('필수 에셋 없음: ' + path)
    return obj

player_bp = required('/Game/Blueprints/Characters/BP_Player')
parry_bp = required('/Game/Abilities/Warrior/GA_Warrior_Parry')
cfg = required('/Game/Input/DA_AbilityInputConfig')
hud_bp = required('/Game/UI/WBP_TwinHUD')
imc = required('/Game/Input/IMC_Default')
orb_mesh = required('/Engine/BasicShapes/Sphere')
player = u.get_default_object(player_bp.generated_class())
player.get_editor_property('mage_config')
entries = list(cfg.get_editor_property('ability_input_actions'))
matches = []
for entry in entries:
    action = entry.get_editor_property('input_action')
    if action and action.get_name() == 'IA_Warrior_Parry':
        matches.append(entry)
if len(matches) != 1:
    raise RuntimeError('기존 우클릭 입력 항목이 하나여야 함')
mapped = any(m.get_editor_property('action') == matches[0].get_editor_property('input_action')
             and str(m.get_editor_property('key').get_editor_property('key_name')) == 'RightMouseButton'
             for m in imc.get_editor_property('default_key_mappings').get_editor_property('mappings'))
if not mapped:
    raise RuntimeError('기존 우클릭 액션의 RightMouseButton 매핑 확인 필요')

backup = '/Game/Developers/Shared/Backups/BeforeArcaneOrb'
def back_up(obj):
    dst = backup + '/' + obj.get_name()
    if not eal.does_asset_exist(dst):
        if not eal.duplicate_asset(obj.get_path_name(), dst):
            raise RuntimeError('백업 실패: ' + obj.get_name())

mirror_bp = u.load_asset('/Game/Blueprints/Boss/BP_Boss_Mirror')
for obj in [player_bp, parry_bp, cfg, hud_bp, mirror_bp]:
    if obj:
        back_up(obj)

def blueprint(path, parent):
    if eal.does_asset_exist(path):
        obj = required(path)
        back_up(obj)
    else:
        folder, name = path.rsplit('/', 1)
        factory = u.BlueprintFactory()
        factory.set_editor_property('parent_class', parent)
        obj = assets.create_asset(name, folder, u.Blueprint, factory)
    if not obj:
        raise RuntimeError('BP 생성 실패: ' + path)
    u.BlueprintEditorLibrary.compile_blueprint(obj)
    return obj

# 전용 단색 구체 재질 (외부 VFX 경로에 의존하지 않는 기본 시각화)
material_path = '/Game/Assets/VFX/Mage/ArcaneOrb/M_ArcaneOrb'
mat = u.load_asset(material_path)
if not mat:
    folder, name = material_path.rsplit('/', 1)
    mat = assets.create_asset(name, folder, u.Material, u.MaterialFactoryNew())
    mat.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
    mel = u.MaterialEditingLibrary
    color = mel.create_material_expression(mat, u.MaterialExpressionConstant3Vector, -200, 0)
    color.set_editor_property('constant', u.LinearColor(0.03, 0.7, 2.5, 1))
    mel.connect_material_property(color, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat, only_if_is_dirty=False)

orb_bp = blueprint('/Game/Blueprints/Skills/BP_ArcaneOrb', orb_native)
orb = u.get_default_object(orb_bp.generated_class())
mesh = orb.get_editor_property('orb_mesh')
mesh.set_editor_property('static_mesh', orb_mesh)
mesh.set_material(0, mat)
# 능력이 바닥 위 45cm에 생성. 루트에는 추가 오프셋을 주지 않음
mesh.set_editor_property('relative_location', u.Vector(0, 0, 0))
mesh.set_editor_property('relative_scale3d', u.Vector(0.7, 0.7, 0.7))
for name, value in [('charge_duration', 1.0), ('life_time', 4.0), ('explosion_radius', 300.0),
                    ('min_damage_multiplier', 1.0), ('max_damage_multiplier', 2.0)]:
    orb.set_editor_property(name, value)
u.BlueprintEditorLibrary.compile_blueprint(orb_bp)
eal.save_loaded_asset(orb_bp, only_if_is_dirty=False)

ga_bp = blueprint('/Game/Abilities/Mage/GA_Mage_ArcaneOrb', ga_native)
ga = u.get_default_object(ga_bp.generated_class())
ga.set_editor_property('orb_class', orb_bp.generated_class())
ga.set_editor_property('input_tag', secondary)
u.BlueprintEditorLibrary.compile_blueprint(ga_bp)
eal.save_loaded_asset(ga_bp, only_if_is_dirty=False)
ga_class = ga_bp.generated_class()

mage_config = player.get_editor_property('mage_config')
startup = list(mage_config.get_editor_property('startup_abilities'))
if ga_class not in startup:
    startup.append(ga_class)
mage_config.set_editor_property('startup_abilities', startup)
player.set_editor_property('mage_config', mage_config)
u.BlueprintEditorLibrary.compile_blueprint(player_bp)
eal.save_loaded_asset(player_bp, only_if_is_dirty=False)

parry = u.get_default_object(parry_bp.generated_class())
parry.set_editor_property('input_tag', secondary)
u.BlueprintEditorLibrary.compile_blueprint(parry_bp)
eal.save_loaded_asset(parry_bp, only_if_is_dirty=False)

new_entry = u.SInputActionConfig()
action_path = matches[0].get_editor_property('input_action').get_path_name()
new_entry.import_text(f'(InputAction="{action_path}",InputTag=(TagName="InputTag.SecondaryAttack"))')
for index, entry in enumerate(entries):
    if entry.get_editor_property('input_action').get_path_name() == action_path:
        entries[index] = new_entry
matches[0] = new_entry
cfg.set_editor_property('ability_input_actions', entries)
eal.save_loaded_asset(cfg, only_if_is_dirty=False)

if mirror_bp:
    mirror = u.get_default_object(mirror_bp.generated_class())
    startup = list(mirror.get_editor_property('startup_abilities'))
    if ga_class not in startup:
        startup.append(ga_class)
    mirror.set_editor_property('startup_abilities', startup)
    u.BlueprintEditorLibrary.compile_blueprint(mirror_bp)
    eal.save_loaded_asset(mirror_bp, only_if_is_dirty=False)

# 기존 마법사 아이콘을 임시 사용. 전용 아이콘은 후속 연출 작업에서 교체
hud = u.get_default_object(hud_bp.generated_class())
icons = dict(hud.get_editor_property('skill_materials'))
fallback = icons.get(u.Name('FrostBolt')) or icons.get(u.Name('MagicBolt'))
if fallback:
    icons[u.Name('ArcaneOrb')] = fallback
    hud.set_editor_property('skill_materials', icons)
u.BlueprintEditorLibrary.compile_blueprint(hud_bp)
eal.save_loaded_asset(hud_bp, only_if_is_dirty=False)

print('구체:', orb_bp.get_path_name())
print('능력:', ga_bp.get_path_name())
print('우클릭:', matches[0].get_editor_property('input_tag').export_text())
print('ARCANE_ORB_SETUP_COMPLETE')
