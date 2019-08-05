# One Hop at a Time

A puzzle-platformer where you can only jump once per level, made in 48 hours for the GMTK game jam 2019. Programming and art was done by me, and sounds effects were generated using [jfxr](https://jfxr.frozenfractal.com).

The game can be downloaded or played in a browser on [itch.io](https://eae02.itch.io/one-hop-at-a-time).

### Build instructions
The game uses my game jam library [jamlib](https://github.com/Eae02/jamlib), and can only be compiled in Linux. To build, you need to have `fish`, `cmake` and `g++` installed. After cloning this repo, run:

```bash
git clone --recurse-submodules https://github.com/Eae02/jamlib.git JamLib
cd JamLib
./BuildAllRelease.fish --only-linux
cd ..
./JamLib/BuildGameRelease.fish
```
