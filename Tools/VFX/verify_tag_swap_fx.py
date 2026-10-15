import unreal as u
for name in ['ToMage','ToWarrior']:
    system=u.load_asset('/Game/VFX/Tag/NS_TagSwap_'+name)
    assert system
    report=u.TSWeaponTrailLibrary.tune_trail_timing(system,1.0,1.0)
    print('TAG_VERIFY',name,report)
    assert 'VALID=1' in report, report
    assert report.count('EMITTER ')==3, report
    assert u.EditorAssetLibrary.save_loaded_asset(system,only_if_is_dirty=False)
for name in ['Ring','Gather','Spark']:
    m=u.load_asset('/Game/VFX/Tag/M_TagSwap_'+name)
    assert m
    assert m.get_editor_property('blend_mode')==u.BlendMode.BLEND_ADDITIVE
    assert m.get_editor_property('used_with_niagara_sprites')
print('TAG_SWAP_VERIFIED: both systems valid, 3 emitters each, 3 additive sprite materials.')
