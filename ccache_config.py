import os
Import("env")

# Check if ccache is available in the system
if os.system("ccache --version > /dev/null 2>&1") == 0:
    # Prefix compiler calls with ccache
    for tool in ["CC", "CXX", "AS", "LINK"]:
        env[tool] = "ccache " + env[tool]
    print("ccache enabled successfully via direct string prefix.")
else:
    print("ccache NOT found, skipping configuration.")
