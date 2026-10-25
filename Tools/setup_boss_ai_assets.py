"""TSBossAIEditLibrary 빌드 후 기존 에디터에서 실행. 기존 작업을 덮어쓰지 않음."""
import unreal as u

eal = u.EditorAssetLibrary
tools = u.AssetToolsHelpers.get_asset_tools()
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
assert not editor.get_game_world(), 'Stop PIE before setup'
assert hasattr(u, 'TSBossAIEditLibrary'), 'Build TwinSwiftEditor with TSBossAIEditLibrary first'

def make_asset(path, cls):
    asset = u.load_asset(path)
    if asset:
        return asset
    folder, name = path.rsplit('/', 1)
    if cls == u.BehaviorTree:
        factory = u.new_object(u.load_class(None, '/Script/BehaviorTreeEditor.BehaviorTreeFactory'))
    else:
        factory = u.DataAssetFactory()
        factory.set_editor_property('data_asset_class', cls)
    return tools.create_asset(name, folder, cls, factory)

bb = make_asset('/Game/Blueprints/AI/Golem/BB_Boss', u.BlackboardData)
u.get_editor_subsystem(u.AssetEditorSubsystem).open_editor_for_assets([bb])
tree = make_asset('/Game/Blueprints/AI/Golem/BT_Golem', u.BehaviorTree)
assert bb and tree
result = u.TSBossAIEditLibrary.configure_tree(tree, bb)
print('TREE_SETUP', result)
assert result.startswith('OK:'), result
assert eal.save_loaded_asset(bb, only_if_is_dirty=False)
# エディター自身がランタイムの木から編集可能なグラフを作る
u.get_editor_subsystem(u.AssetEditorSubsystem).open_editor_for_assets([tree])
assert eal.save_loaded_asset(tree, only_if_is_dirty=False)

boss = u.load_asset('/Game/Blueprints/Boss/BP_Boss_Golem')
assert boss
cdo = u.get_default_object(boss.generated_class())
cdo.set_editor_property('behavior_tree', tree)
cdo.set_editor_property('ai_controller_class', u.load_class(None, '/Script/TwinSwift.SAIController'))
cdo.set_editor_property('auto_possess_ai', u.AutoPossessAI.PLACED_IN_WORLD_OR_SPAWNED)
u.BlueprintEditorLibrary.compile_blueprint(boss)
assert eal.save_loaded_asset(boss, only_if_is_dirty=False)

world = editor.get_editor_world()
assert world.get_path_name().startswith('/Game/Maps/L_Dev.'), 'Open L_Dev before navigation setup'
actor_subsystem = u.get_editor_subsystem(u.EditorActorSubsystem)
actors = actor_subsystem.get_all_level_actors()
floors = []
for actor in actors:
    if isinstance(actor, u.StaticMeshActor):
        center, extent = actor.get_actor_bounds(False)
        if extent.x > 500 and extent.y > 500 and extent.z < 200:
            floors.append((center, extent))
assert floors, 'Could not identify a large flat floor; no map changes made'
lo_x = min(c.x - e.x for c, e in floors)
hi_x = max(c.x + e.x for c, e in floors)
lo_y = min(c.y - e.y for c, e in floors)
hi_y = max(c.y + e.y for c, e in floors)
z = max(c.z + e.z for c, e in floors)
center = u.Vector((lo_x + hi_x) / 2, (lo_y + hi_y) / 2, z + 200)
volume = next((a for a in actors if isinstance(a, u.NavMeshBoundsVolume) and a.get_actor_label() == 'BossNavBounds'), None)
if not volume:
    volume = actor_subsystem.spawn_actor_from_class(u.NavMeshBoundsVolume, center)
    volume.set_actor_label('BossNavBounds')
volume.set_actor_location(center, False, False)
size = u.Vector(hi_x - lo_x + 100, hi_y - lo_y + 100, 800)
assert u.TSBossAIEditLibrary.build_nav_box(volume, size)
u.SystemLibrary.execute_console_command(world, 'RebuildNavigation')
u.SystemLibrary.execute_console_command(world, 'ShowFlag.Navigation 1')
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
assert u.get_default_object(boss.generated_class()).get_editor_property('behavior_tree') == tree
print('BOSS_AI_ASSETS_SAVED', bb.get_path_name(), tree.get_path_name(), 'Nav volume', size)
print('Navigation rebuild requested; validate generated coverage and chase separately.')
