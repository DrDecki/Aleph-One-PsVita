# Aleph One Vita

This is a native port of Aleph One for the PS Vita. Aleph One is the open source engine behind Bungie's *Marathon*, *Marathon 2: Durandal* and *Marathon Infinity*, and it also runs a huge library of fan made scenarios like *Eternal*, *Rubicon X* or *Marathon Phoenix*.

The games run at the Vita's native 960x544 resolution with a stable 30 FPS. Everything is set up for the Vita automatically the first time you start a game, so you can just pick one and play.

## Installation

You need a PS Vita or PS TV with HENkaku or Enso and [VitaShell](https://github.com/TheOfficialFloW/VitaShell/releases).

1. Install `alephone.vpk` with VitaShell.
2. Download the games you want from the [Aleph One scenarios page](https://alephone.lhowon.org/scenarios.html). All three Marathon games are free.
3. Unzip them on your PC and copy every game into its own folder inside `ux0:data/AlephOne/`.
4. Copy the contents of `AlephOne.zip` into `ux0:data/AlephOne/`.
5. Start Aleph One from the LiveArea and choose a game.

When you're done, the folder should look like this:

    ux0:data/AlephOne/Lua/
    ux0:data/AlephOne/Plugins/
    ux0:data/AlephOne/Marathon/
    ux0:data/AlephOne/Marathon 2/
    ux0:data/AlephOne/Marathon Infinity/

Each game folder has to contain the game files directly, like Map, Shapes and Sounds. If you find another folder with the same name inside it, move its contents one level up.

Fan scenarios work the same way. Give each one its own folder next to the others.

## Cheats

The port ships with a built-in cheat console, available in every scenario.
It requires the `Lua/Cheats.lua` file from the data package
(`AlephOne.zip`) to be installed under `ux0:/data/AlephOne/`.

Press **Triangle** in-game, type a command and confirm with Enter:

| Command | Effect |
| --- | --- |
| `nrg()` | Recharge energy (1x -> 2x -> 3x shields) |
| `otwo()` | Refill oxygen |
| `nuke()` | Invincibility power-up (temporary) |
| `kyt()` | Toggle permanent invincibility |
| `bye()` | Invisibility power-up |
| `see()` | Infravision |
| `wow()` | Extravision |
| `mag()` | Pistol + ammo |
| `melt()` | Fusion pistol + ammo |
| `rif()` | Assault rifle + ammo and grenades |
| `pow()` | Rocket launcher + ammo |
| `toast()` | Flamethrower + fuel |
| `puff()` | Shotgun + ammo |
| `zip()` | SMG + ammo |
| `pzbxay()` | Alien weapon |
| `ammo()` | +10 ammo for all weapons |
| `shit()` | All weapons, all ammo, full shields |
| `qwe()` | Jump |
| `yourmom()` | Save the game anywhere |
| `sesame()` | Activate whatever you are pointing at |
| `wtf()` | Show the level completion state |

If the console gets stuck after closing the keyboard without input, press
**Triangle** and confirm an empty line with Enter to reset it.

## License

Aleph One and this port are released under the [GNU General Public License v3](https://www.gnu.org/licenses/gpl-3.0.html). The game data is not included and belongs to its respective owners.
