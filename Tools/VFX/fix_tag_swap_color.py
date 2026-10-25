import unreal as u
from pathlib import Path
for name in ['ToMage','ToWarrior']:
    path='/Game/Assets/VFX/Common/Tag/NS_TagSwap_'+name
    backup='/Game/Developers/Shared/Backups/TagSwap_BeforeColorFix/NS_TagSwap_'+name
    if u.EditorAssetLibrary.does_asset_exist(path):
        if not u.EditorAssetLibrary.does_asset_exist(backup):
            assert u.EditorAssetLibrary.duplicate_asset(path,backup)
        assert u.EditorAssetLibrary.delete_asset(path)
exec(Path('D:/Unreal Projects/TwinSwift/Tools/VFX/create_tag_swap_fx.py').read_text(encoding='utf-8'))
