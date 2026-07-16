#!/usr/bin/env python3

import hashlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

# Unity가 UPM 패키지 asset으로 import하는 디렉터리만 처리한다.
IMPORT_ROOTS = (
    ROOT / "Editor",
    ROOT / "Runtime",
    ROOT / "Tests",
)

ROOT_ASSETS = (
    ROOT / "package.json",
    ROOT / "README.md",
    ROOT / "CHANGELOG.md",
    ROOT / "LICENSE",
)


def deterministic_guid(path: Path) -> str:
    relative = path.relative_to(ROOT).as_posix()
    seed = f"com.lge-ros2.cloisim.vulkan-rt:{relative}"

    return hashlib.md5(
        seed.encode("utf-8")
    ).hexdigest()


def default_importer_meta(
    path: Path,
    *,
    folder: bool = False,
) -> str:
    folder_asset = "folderAsset: yes\n" if folder else ""

    return (
        "fileFormatVersion: 2\n"
        f"guid: {deterministic_guid(path)}\n"
        f"{folder_asset}"
        "DefaultImporter:\n"
        "  externalObjects: {}\n"
        "  userData: \n"
        "  assetBundleName: \n"
        "  assetBundleVariant: \n"
    )


def mono_importer_meta(path: Path) -> str:
    return (
        "fileFormatVersion: 2\n"
        f"guid: {deterministic_guid(path)}\n"
        "MonoImporter:\n"
        "  externalObjects: {}\n"
        "  serializedVersion: 2\n"
        "  defaultReferences: []\n"
        "  executionOrder: 0\n"
        "  icon: {instanceID: 0}\n"
        "  userData: \n"
        "  assetBundleName: \n"
        "  assetBundleVariant: \n"
    )


def assembly_definition_meta(path: Path) -> str:
    return (
        "fileFormatVersion: 2\n"
        f"guid: {deterministic_guid(path)}\n"
        "AssemblyDefinitionImporter:\n"
        "  externalObjects: {}\n"
        "  userData: \n"
        "  assetBundleName: \n"
        "  assetBundleVariant: \n"
    )


def native_linux_plugin_meta(path: Path) -> str:
    return (
        "fileFormatVersion: 2\n"
        f"guid: {deterministic_guid(path)}\n"
        "PluginImporter:\n"
        "  externalObjects: {}\n"
        "  serializedVersion: 3\n"
        "  iconMap: {}\n"
        "  executionOrder: {}\n"
        "  defineConstraints: []\n"
        "  isPreloaded: 0\n"
        "  isOverridable: 0\n"
        "  isExplicitlyReferenced: 0\n"
        "  validateReferences: 1\n"
        "  platformData:\n"
        "  - first:\n"
        "      Any: \n"
        "    second:\n"
        "      enabled: 0\n"
        "      settings: {}\n"
        "  - first:\n"
        "      Editor: Editor\n"
        "    second:\n"
        "      enabled: 1\n"
        "      settings:\n"
        "        CPU: x86_64\n"
        "        DefaultValueInitialized: true\n"
        "        OS: Linux\n"
        "  - first:\n"
        "      Standalone: Linux64\n"
        "    second:\n"
        "      enabled: 1\n"
        "      settings:\n"
        "        CPU: x86_64\n"
        "  userData: \n"
        "  assetBundleName: \n"
        "  assetBundleVariant: \n"
    )


def meta_content(path: Path) -> str:
    if path.is_dir():
        return default_importer_meta(
            path,
            folder=True,
        )

    suffix = path.suffix.lower()

    if suffix == ".cs":
        return mono_importer_meta(path)

    if suffix == ".asmdef":
        return assembly_definition_meta(path)

    if suffix == ".so":
        return native_linux_plugin_meta(path)

    return default_importer_meta(path)


def ensure_meta(path: Path) -> None:
    meta_path = Path(f"{path}.meta")

    if meta_path.exists():
        print(
            f"[KEEP] {meta_path.relative_to(ROOT)}"
        )
        return

    meta_path.write_text(
        meta_content(path),
        encoding="utf-8",
    )

    print(
        f"[ADD]  {meta_path.relative_to(ROOT)}"
    )


for import_root in IMPORT_ROOTS:
    if not import_root.exists():
        continue

    ensure_meta(import_root)

    entries = sorted(
        import_root.rglob("*"),
        key=lambda entry: (
            len(entry.parts),
            entry.as_posix(),
        ),
    )

    for entry in entries:
        if entry.name.endswith(".meta"):
            continue

        ensure_meta(entry)


for root_asset in ROOT_ASSETS:
    if root_asset.exists():
        ensure_meta(root_asset)
