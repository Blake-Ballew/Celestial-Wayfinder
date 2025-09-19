import os
import json

CONFIG_FILE = "version_config.json"
BUILD_NUMBER_FILE = "build_number.txt"
OUTPUT_FILE = "../esp32-utilities/include/Utilities/VersionUtils.h"
TEMPLATE_FILE = "version_template.h"

# Load major, minor, patch
with open(CONFIG_FILE, "r") as f:
    version_data = json.load(f)

major = version_data["major"]
minor = version_data["minor"]
patch = version_data["patch"]

# Read or increment build number
if os.path.exists(BUILD_NUMBER_FILE):
    with open(BUILD_NUMBER_FILE, "r") as f:
        build = int(f.read().strip()) + 1
else:
    build = 1

with open(BUILD_NUMBER_FILE, "w") as f:
    f.write(str(build))

# Load and substitute template
with open(TEMPLATE_FILE, "r") as f:
    template = f.read()

output = (
    template
    .replace("__VERSION_MAJOR__", str(major))
    .replace("__VERSION_MINOR__", str(minor))
    .replace("__VERSION_PATCH__", str(patch))
    .replace("__BUILD_NUMBER__", str(build))
    .replace("__VERSION_STRING__", f"{major}.{minor}.{patch}.{build}")
)

os.makedirs(os.path.dirname(OUTPUT_FILE), exist_ok=True)

with open(OUTPUT_FILE, "w") as f:
    f.write(output)

print(f"Generated version.h: {major}.{minor}.{patch}.{build}")

# Write version.json to build directory
BUILD_DIR = os.path.join("build", "version.json")
os.makedirs(os.path.dirname(BUILD_DIR), exist_ok=True)

with open(BUILD_DIR, "w") as f:
    json.dump({
        "major": major,
        "minor": minor,
        "patch": patch,
        "build": build,
        "version": f"{major}.{minor}.{patch}.{build}"
    }, f, indent=4)

