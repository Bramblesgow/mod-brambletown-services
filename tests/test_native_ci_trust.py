#!/usr/bin/env python3
"""Check repository CI policy; not a substitute for runner access configuration."""
from pathlib import Path
import re

workflow = (Path(__file__).resolve().parents[1] / ".github/workflows/static-checks.yml").read_text()
assert "branches: [main]" in workflow
assert "pull_request:" in workflow
assert "contents: read" in workflow
assert workflow.count("persist-credentials: false") == 2
assert workflow.count("runs-on: self-hosted") == 1
native = workflow.split("  native-core-validation:", 1)[1]
assert re.search(r"if:.*github.event_name == 'push'.*github.ref == 'refs/heads/main'.*github.ref_protected", native)
# Assert the exact environment passed to the step that executes the checks.
# A variable defined on a different step must not satisfy this test.
step_header = "      - name: Compile reviewed NPC Services code and apply/check native patch\n"
assert native.count(step_header) == 1
step_tail = native.split(step_header, 1)[1]
step = re.split(r"(?m)^      - |^  [a-z][a-z-]*:", step_tail, maxsplit=1)[0]
env_match = re.search(r"(?m)^        env:\n((?:^          [^\n]+\n)+)", step)
assert env_match, "Native validation step must declare its own env mapping"
env_lines = env_match.group(1).splitlines()
env = dict(line.strip().split(": ", 1) for line in env_lines)
assert len(env) == len(env_lines), "Duplicate keys in the native step env mapping"
for variable in ("AZEROTHCORE_COMPILE_COMMANDS", "BRAMBLETOWN_BUILD_DIR",
                 "BRAMBLETOWN_NATIVE_BASE_ROOT"):
    assert env.get(variable) == f"${{{{ vars.{variable} }}}}", (
        f"Missing/incorrect {variable} on the native validation step"
    )

run_match = re.search(r"(?m)^        run: \|\n((?:^          [^\n]*\n)+)", step)
assert run_match, "Native step must execute the validation scripts"
commands = [line.strip() for line in run_match.group(1).splitlines()]
for script in ("check_cpp_syntax.py", "build_isolated_objects.py",
               "check_scribe_price_patch_cpp.py"):
    assert f"python3 tests/{script}" in commands
print("PASS: protected-main native CI passes exact path variables to the three validation scripts")
