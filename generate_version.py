import os
import json

CONFIG_FILE = "version_config.json"
OUTPUT_FILE = "../esp32-utilities/include/Utilities/VersionUtils.h"
TEMPLATE_FILE = "version_template.h"

# Load major, minor, patch
with open(CONFIG_FILE, "r") as f:
    version_data = json.load(f)

major = version_data["major"]
minor = version_data["minor"]
patch = version_data["patch"]

# Load and substitute template
with open(TEMPLATE_FILE, "r") as f:
    template = f.read()

output = (
    template
    .replace("__VERSION_MAJOR__", str(major))
    .replace("__VERSION_MINOR__", str(minor))
    .replace("__VERSION_PATCH__", str(patch))
    .replace("__VERSION_STRING__", f"{major}.{minor}.{patch}")
)

os.makedirs(os.path.dirname(OUTPUT_FILE), exist_ok=True)

with open(OUTPUT_FILE, "w") as f:
    f.write(output)

print(f"Generated version.h: {major}.{minor}.{patch}")

# Write version.json to build directory
BUILD_DIR = os.path.join("build", "version.json")
os.makedirs(os.path.dirname(BUILD_DIR), exist_ok=True)

with open(BUILD_DIR, "w") as f:
    json.dump({
        "major": major,
        "minor": minor,
        "patch": patch,
        "version": f"{major}.{minor}.{patch}"
    }, f, indent=4)

