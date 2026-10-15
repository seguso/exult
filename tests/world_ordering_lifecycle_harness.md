# World-ordering lifecycle harness

This diagnostic executable path drives Exult's real BG runtime and records the
actual Map_chunk, Game_object and Game_render ordering lifecycle. It does not
contain a second implementation of the comparator or dependency builder.

The harness is an opt-in Exult diagnostic on branch
codex/world-ordering-m2; it is not part of devmix and must not be merged there
without review. The reference source revision is
0a9b64a6c4f02aafb552fe98d591e9e7d96c8815.

## Build and run on Windows

Build the game target from this checkout:

~~~powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' msvcstuff\vs2019\Exult.vcxproj /p:Configuration=Release /p:Platform=x64 /m /v:minimal
~~~

The diagnostic path bypasses the interactive intro in code. The supplied
baseline-exult.cfg also sets config/gameplay/skip_splash. Set both SDL drivers
to `dummy`: video stays headless and MIDI/audio cannot become audible. In
PowerShell, run the harness (it initializes the real BG game data, runs the
fixture instead of entering the event loop, then exits):

~~~powershell
$env:SDL_VIDEODRIVER = 'dummy'
$env:SDL_AUDIODRIVER = 'dummy'
$fixture = 'C:\Users\Maurizio\Software\ult\tests\Ult.Engine.Tests\Fixtures\WorldOrdering\world-ordering-v1.input.jsonl'
$trace = 'C:\Users\Maurizio\Software\ult\tests\Ult.Engine.Tests\Fixtures\WorldOrdering\world-ordering-v1.exult-reference.jsonl'
$p = Start-Process -FilePath '.\Exult.exe' -ArgumentList @('--bg', '--nomenu', '-c', '.\baseline-exult.cfg', '--world-ordering-fixture', $fixture, '--world-ordering-output', $trace) -PassThru -Wait -WindowStyle Hidden -RedirectStandardOutput '.\runner.stdout.tmp' -RedirectStandardError '.\runner.stderr.tmp'
if ($p.ExitCode -ne 0) { throw "Exult fixture run failed: $($p.ExitCode)" }
~~~

The diagnostic arguments must be supplied together with an explicit game
selection. Play() runs the fixture instead of entering the interactive event
loop, then shuts down Exult normally.

## Input schema, version 1

The input is JSON Lines. Every record has schemaVersion: 1.

- scenario: stable scenario ID, game (bg or si) and a tile position used to
  center the real Exult viewport.
- object: stable fixture object ID, native shape/frame, absolute tile/lift,
  native Exult 3D footprint/height and occlusion flag. The harness checks these
  dimensions against the installed game's real shape metadata; it never edits
  Shape_info.
- operation: add, remove, move, compare or paint. add and move call the real
  Map_chunk::add / Game_object::move; remove calls Map_chunk::remove; compare
  calls Game_object::compare in both argument directions using native
  Ordering_info; every operation is followed by the real Game_render::paint_map.
- endScenario: closes the scenario after all fixture objects have been
  removed.

The current input contains scenarios A-H. The optional sleeping Actor/bed,
Actor-cycle, Serpent Isle depression and moving-barge scenarios remain open:
they require native runtime Actor/Barge setup rather than static fixture objects.

## Output

The first trace row records the Exult source commit, the harness/build commit,
and game. Each following step records the current object list and non-flat insertion order for inspected
chunks, both dependency directions, added/removed edges since the previous
operation, from_below/from_right/from_below_right counters, the actual
Game_object::compare calls made inside Map_chunk::add_dependencies, the
diagonal chunk visits, and fixture object visits from the actual recursive
paint_object traversal.

Fixture objects use scenario:id; pre-existing installed-world objects use
stable native:chunkX:chunkY:ordinal IDs assigned before fixture insertion.
The paint callback runs after Exult has recursively visited dependencies and
immediately before the real object's paint call. It observes the production
traversal; it does not supply a replacement painter.

The H input reuses the tile pair from the earlier abstract non-antisymmetric
comparison fixture. With the installed game's real shapes and the actual
viewport-produced rectangles, Exult returns -1 and +1. The earlier +1/+1 case
used arbitrary projected rectangles and is not a result reproduced by this
runtime fixture.

The trace contains installed BG map objects in the inspected viewport chunks,
so its exact size depends on installed game data. This baseline was generated
from the local BG installation and is reproducible against the pinned Exult
revision and fixture.
