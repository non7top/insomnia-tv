import os
Import("env")

print("--- ENV DUMP (partial) ---")
for key in ["CC", "CXX", "AS", "LINK", "CCCOM", "CXXCOM"]:
    print(f"{key}: {env.get(key)}")
print("--------------------------")
