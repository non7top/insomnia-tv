import os
import shutil
Import("env")

# Check if ccache is available in the system
ccache_path = shutil.which("ccache")
if ccache_path:
    # Use Prepend to ensure ccache is at the beginning
    env.Prepend(
        CC="ccache ",
        CXX="ccache ",
        AS="ccache "
    )
    print(f"ccache prepended for {env.get('PIOENV')}")
else:
    print("ccache NOT found, skipping configuration.")
