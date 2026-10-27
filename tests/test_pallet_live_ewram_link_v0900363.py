"""Rev1 linker regression: Pallet live-demo flag must be retained in RAM."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
field = (ROOT / "src/fieldmap.c").read_text()
ram_symbols = (ROOT / "sym_ewram.txt").read_text()
link_script = (ROOT / "ld_script.txt").read_text()
link_sed = (ROOT / "ld_script.sed").read_text()
assert re.search(r"static\s+EWRAM_DATA\s+bool8\s+sPalletLiveDemoNight\s*=\s*FALSE\s*;", field)
assert "static bool8 sPalletLiveDemoNight = FALSE;" not in field
assert '.include "src/fieldmap.o"' in ram_symbols
assert '<EWRAM>' in link_script
assert 'r sym_ewram.ld' in link_sed
assert "PalletTown_TryToggleLivePreview" in field
assert "#define PALLET_NIGHT_PREVIEW_MODE 3" in field
print("PASS: Pallet preview state explicitly lives in linker-retained EWRAM")
