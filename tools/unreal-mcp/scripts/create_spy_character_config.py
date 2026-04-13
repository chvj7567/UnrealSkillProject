"""
SpyCharacterConfig DataAsset 생성 및 기본값 설정 스크립트
MCP execute_python 툴에 이 스크립트 내용을 전달해서 실행.

전제조건: SpyCharacterConfig 클래스가 컴파일된 상태여야 함.
생성 경로: /Game/Data/DA_SpyCharacterConfig
"""
import unreal

ASSET_NAME = "DA_SpyCharacterConfig"
ASSET_PATH = "/Game/Data"

# 이미 존재하면 로드, 없으면 생성
full_path = f"{ASSET_PATH}/{ASSET_NAME}"
config = unreal.EditorAssetLibrary.load_asset(full_path)

if config is None:
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    config = tools.create_asset(
        ASSET_NAME,
        ASSET_PATH,
        unreal.SpyCharacterConfig,
        None
    )

# Collision
config.set_editor_property("capsule_radius", 42.0)
config.set_editor_property("capsule_half_height", 96.0)

# Movement
config.set_editor_property("rotation_rate_yaw", 1080.0)
config.set_editor_property("jump_z_velocity", 700.0)
config.set_editor_property("air_control", 0.35)
config.set_editor_property("max_walk_speed", 500.0)
config.set_editor_property("min_analog_walk_speed", 20.0)
config.set_editor_property("braking_deceleration_walking", 2000.0)
config.set_editor_property("braking_deceleration_falling", 1500.0)

# Camera
config.set_editor_property("camera_arm_length", 400.0)

# UI
config.set_editor_property("hp_bar_offset", unreal.Vector(0.0, 0.0, 200.0))

# Weapon
config.set_editor_property("weapon_spawn_delay", 1.0)

unreal.EditorAssetLibrary.save_asset(full_path)

print(f"[OK] {full_path} 생성/업데이트 완료")
