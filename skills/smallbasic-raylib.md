# SmallBASIC raylib Programming

Use this when writing a graphics/game program in SmallBASIC using the raylib plugin
(https://smallbasic.github.io/pages/plugins_raylib.html). raylib itself has almost no
prose documentation by design — its own docs say the way to learn it is by reading examples —
so lean on the converted sample programs at
https://github.com/smallbasic/smallbasic.plugins/tree/master/raylib/samples over guessing at
an API surface from memory. The C cheatsheet at raylib.com/cheatsheet also maps closely: most
function names carry over unchanged, just called through the `rl.` namespace.

## Setup, every program needs this

```basic
import raylib as rl
import raylibc as c
```

- `rl` is the function namespace (`rl.InitWindow`, `rl.DrawText`, ...). `c` is the constants
  namespace (`c.RAYWHITE`, `c.CAMERA_PERSPECTIVE`, ...) — pulled in via `raylibc.bas`.
- The plugin library (`libraylib.so`/`.dll`) and `raylibc.bas` should be found automatically in
  a release install. If they're not, either copy both into the program's own folder, or give the
  full path directly on the `import` line.
- **Run with the console binary, not the SDL one.** raylib owns its own window/OpenGL context,
  so it conflicts with SmallBASIC's SDL frontend (`sbasicg`). Always launch with `sbasic
  MyProgram.bas` (or the console AppImage), never `sbasicg`. This is the single most common
  "nothing happens" or crash-on-start mistake.

## The standard program shape

Every raylib program in SmallBASIC follows the same three-part structure — match it rather than
inventing a different control flow:

```basic
' 1. Initialization
const screenWidth = 800
const screenHeight = 450
rl.InitWindow(screenWidth, screenHeight, "window title")
rl.SetTargetFPS(60)

' 2. Main loop — one iteration per frame
while (!rl.WindowShouldClose())
  ' Update: read input, advance game state — no drawing calls here
  ' Draw: everything between BeginDrawing/EndDrawing, nothing else
  rl.BeginDrawing()
  rl.ClearBackground(c.RAYWHITE)
  ' ... draw calls ...
  rl.EndDrawing()
wend

' 3. De-initialization
rl.CloseWindow()
```

- `WindowShouldClose()` returns true on the window's close button or Esc by default — this is
  the whole exit condition, don't build a separate quit flag unless you need custom exit logic.
- Keep update logic and draw calls in separate sections of the loop body even though SmallBASIC
  won't enforce it — mixing them makes frame-rate-independent movement and later refactors much
  harder.
- Never allocate/load a texture, sound, or font *inside* the main loop — load once before the
  loop, reference it every frame, unload once after the loop. Loading per-frame is the most
  common cause of a raylib program that runs fine for a few seconds then stutters or leaks.

## Structures: three equivalent literal forms

raylib structs (`Vector2`, `Vector3`, `Rectangle`, `Camera2D`, `Camera3D`) can be built three
ways in SmallBASIC — pick one convention per project and stay consistent rather than mixing:

```basic
' dot-notation, field by field
vector2.x = 0
vector2.y = 0

' record literal
vector2 = {x: 0, y: 0}

' positional array literal
vector2 = [0, 0]
```

The same pattern applies to `Rectangle` (`x, y, width, height`) and `Vector3`
(`x, y, z`). `Camera2D` needs `target`, `offset`, `rotation`, `zoom`; `Camera3D` needs
`position`, `target`, `up`, `fovy`, `projection` (the last from `c.CAMERA_PERSPECTIVE` /
`c.CAMERA_ORTHOGRAPHIC`).

## Constants live under `c.`

Colors (`c.RAYWHITE`, `c.LIGHTGRAY`, `c.RED`, ...), camera modes, config flags, key codes — all
referenced through the `raylibc` namespace, never as bare identifiers or magic numbers. If a
raylib C example uses a constant you don't recognize the SmallBASIC name for, check
`raylibc.bas` itself (it's plain SmallBASIC source) rather than guessing the spelling.

## Where to check syntax before writing new code

For anything beyond basic window/shape/text calls — input handling, cameras, textures, audio,
models, shaders, physics — pull up the matching converted sample first rather than inferring
the call signature from the C cheatsheet alone:

- Input: `core_input_keys.bas`, `core_input_mouse.bas`, `core_input_gamepad.bas`,
  `core_input_multitouch.bas`
- Cameras: `core_2d_camera.bas`, `core_initialize_3d_camera_mode.bas`,
  `core_3d_camera_first_person.bas`
- Textures/sprites: `textures_sprite_button.bas`, `textures_bunnymark.bas`,
  `textures_draw_tiled.bas`
- Audio: `audio_module_playing.bas`, `audio_music_stream.bas`
- Shaders: `shaders_postprocessing.bas`, `shaders_raymarching.bas`
- Physics: `physics_demo.bas`, `physics_movement.bas`, `physics_restitution.bas`

## What NOT to do

- Don't launch with `sbasicg` — raylib's window management conflicts with the SDL frontend.
- Don't put `BeginDrawing`/`EndDrawing` anywhere but wrapping the single per-frame draw block —
  nested or repeated Begin/End pairs in one frame produce flicker or driver errors.
- Don't load textures/sounds/fonts inside the loop; load before it, unload after.
- Don't guess a function's argument order from the C cheatsheet name alone if a converted sample
  exists — SmallBASIC's struct-literal calling convention sometimes differs from the raw C
  positional-args signature.
