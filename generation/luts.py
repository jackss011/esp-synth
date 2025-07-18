import numpy as np
from matplotlib import pyplot as plt
import math
from pathlib import Path

dest_path = Path("../src/audio/generated")
dest_path.mkdir(parents=False, exist_ok=True)

LUTGEN_RESOLUTION = 1024
LITERALS_PER_LINE = 16

waves = dict();
t = np.linspace(0, 1, LUTGEN_RESOLUTION, endpoint=False);


# sine wave
waves["sin"] = np.sin(t * 2 * math.pi)
# saw (rising)
waves["saw"] = 2.0 * t - 1.0
# saw reverse (falling)
waves["saw_rev"] = 1.0 - 2.0 * t
# triangle
waves["tri"] = 4.0 * np.abs(t - 0.5) - 1.0
# tri-saw hybrid
waves["tri_saw"] = np.where(
    t < 0.5,
    4.0 * t - 1.0,            # rising
    -2.0 * (t - 0.5)          # falling
)
# square
waves["square"] = np.where(t < 0.5, 1.0, -1.0)
# rect wide (25% duty)
waves["rect_wide"] = np.where(t < 0.25, 1.0, -1.0)
# rect narrow (10% duty)
waves["rect_narrow"] = np.where(t < 0.1, 1.0, -1.0)


# ======== FILE GENERATION =========
file_lines = [
    "#pragma once",
    '#include "esp_attr.h"',
    "",
    f"#define LUTGEN_RESOLUTION {LUTGEN_RESOLUTION}",
    "#define LUTEGEN_INDEX(x) ((int32_t)(x * LUTGEN_RESOLUTION) % LUTGEN_RESOLUTION)",
    "",
]

for name, values in waves.items():
    # variable name
    file_lines.append(f"IRAM_ATTR float lutgen_{name}[{LUTGEN_RESOLUTION}] = {{")

    # literals table
    for i in range(0, len(values), LITERALS_PER_LINE):
        end_i = min(len(values), i + LITERALS_PER_LINE)
        values_slice = values[i:end_i]
        literals = [f"{x:.10f}f" for x in values_slice];
        file_lines.append(f"    {",".join(literals)},")

    # closing bracket
    file_lines.append("};\n\n")
    print(f"added {name}!")

# write to file
file_lines = [x + '\n' for x in file_lines] # add endline character

with open(dest_path / "luts.hpp", 'w') as f:
    f.writelines(file_lines)

print("generated file!")


# ======== PLOTS =========
plot_path = Path("./figures")
plot_path.mkdir(exist_ok=True)

print("\ngenerating graphs...")

for name, values in waves.items():
    plt.figure()
    plt.grid()
    plt.tight_layout()
    plt.title(name)

    # t2 = np.concat((t, [2]))
    # v2 = np.concat((values, [values[0]]))
    plt.plot(t, values)

    fn = plot_path / f"{name}.jpg"
    plt.savefig(fn)
    print(f"generated {fn}!")

print("all done!")
