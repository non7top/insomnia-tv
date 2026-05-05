import subprocess

def get_git_revision_short_hash():
    try:
        return subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).decode('ascii').strip()
    except:
        return 'unknown'

revision = get_git_revision_short_hash()
print(f"Build Version: {revision}")

Import("env")
env.Append(CPPDEFINES=[
    ("INSOMNIATV_VERSION", f'\\"{revision}\\"')
])
