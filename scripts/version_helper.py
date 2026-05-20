import subprocess
import os

def get_git_revision_short_hash():
    try:
        return subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).decode('ascii').strip()
    except:
        return 'unknown'

revision = get_git_revision_short_hash()
print(f"Build Version: {revision}")

# Create data directory if it doesn't exist
if not os.path.exists("data"):
    os.makedirs("data")

# Write version to data/version.txt for filesystem upload
with open("data/version.txt", "w") as f:
    f.write(revision)

Import("env")
# We no longer append to CPPDEFINES to avoid breaking the build cache
# env.Append(CPPDEFINES=[("INSOMNIATV_VERSION", f'\\"{revision}\\"')])
