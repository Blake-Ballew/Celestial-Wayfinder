from pathlib import Path
import json
import os

# Paths and config
CONFIG_FILE = "version_config.json"
BUILD_NUMBER_FILE = "build_number.txt"
OUTPUT_HEADER = "../esp32-utilities/include/Utilities/VersionUtils.h"
TEMPLATE_FILE = "version_template.h"

def generate_version(env):
    # Load config
    with open(CONFIG_FILE, "r") as f:
        version_data = json.load(f)

    major = version_data["major"]
    minor = version_data["minor"]
    patch = version_data["patch"]

    # Update build number
    if os.path.exists(BUILD_NUMBER_FILE):
        with open(BUILD_NUMBER_FILE, "r") as f:
            build = int(f.read().strip()) + 1
    else:
        build = 1

    with open(BUILD_NUMBER_FILE, "w") as f:
        f.write(str(build))

    version_string = f"{major}.{minor}.{patch}.{build}"

    # Generate version.h
    with open(TEMPLATE_FILE, "r") as f:
        template = f.read()

    output = (
        template
        .replace("__VERSION_MAJOR__", str(major))
        .replace("__VERSION_MINOR__", str(minor))
        .replace("__VERSION_PATCH__", str(patch))
        .replace("__BUILD_NUMBER__", str(build))
        .replace("__VERSION_STRING__", version_string)
    )

    os.makedirs(os.path.dirname(OUTPUT_HEADER), exist_ok=True)
    with open(OUTPUT_HEADER, "w") as f:
        f.write(output)

    # Emit version.json into the correct .pio/build/<env> dir
    build_dir = Path(env.subst("$BUILD_DIR"))
    build_dir.mkdir(parents=True, exist_ok=True)

    with open(build_dir + "/version.json", "w") as f:
        json.dump({
            "major": major,
            "minor": minor,
            "patch": patch,
            "build": build,
            "version": version_string
        }, f, indent=4)

    print(f"[VersionGen] Generated version: {version_string}")


def before_build(env):
    generate_version(env)
