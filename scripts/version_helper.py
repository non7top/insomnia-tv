import subprocess
import os

def get_git_revision_short_hash():
    try:
        return subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).decode('ascii').strip()
    except:
        return 'unknown'

def is_git_dirty():
    try:
        subprocess.check_output(['git', 'diff', '--quiet', 'HEAD'], stderr=subprocess.DEVNULL)
        return False
    except subprocess.CalledProcessError:
        return True

revision = get_git_revision_short_hash()
if is_git_dirty():
    revision += '-dev'
print(f"Build Version: {revision}")

# Create data directory if it doesn't exist
if not os.path.exists("data"):
    os.makedirs("data")

# Write version to data/version.txt for filesystem upload
with open("data/version.txt", "w") as f:
    f.write(revision)

Import("env")
# Write a generated header instead of injecting via CPPDEFINES.
# CPPDEFINES changes the compiler invocation for every translation unit,
# causing ccache to miss on all of them whenever the git hash changes.
# A generated header limits recompilation to files that include it.
version_header = os.path.join("src", "version.h")
with open(version_header, "w") as f:
    f.write(f'#pragma once\n#define INSOMNIATV_VERSION "{revision}"\n')
