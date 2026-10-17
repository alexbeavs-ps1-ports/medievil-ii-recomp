# Modern controls

**Right-stick Camera** is enabled by default under Mods → Controls. It presents
a DualShock even when the launcher was set to digital, so a controller's right
stick can rotate the native camera. Its horizontal and vertical reversal options
are independent of mouse settings. L2/R2 retain the original button actions.

Enable **Mouse and Keyboard** under Mods → Controls. Use the game's native
**button layout A**. The mod changes P1's keyboard bindings for that session;
your saved bindings return when it is disabled. Gamepad buttons remain usable.

| Input | Action |
| --- | --- |
| WASD | Move; navigate menus |
| Mouse movement | Native camera look |
| Left click | Attack / interact / confirm |
| Right click | Secondary attack; hold to charge |
| Middle click | Inventory / items |
| Side button 1 / 2 | Switch equipped weapon / change target |
| Space | Jump |
| Left Shift | Defend / duck; dash when running |
| E | Switch between the two equipped weapons |
| F | Interact / confirm |
| Q | Change target |
| Left Ctrl + WASD | Strafe / walk relative to Dan |
| Tab | Inventory; select items with WASD and F / left click |
| Enter | Pause |
| 1, 2, 3 | Primary attack, secondary attack, defend / dash |
| 4 | Switch equipped weapon |
| 5 | Switch Dan / Dan-Hand after unlocking that ability |
| 6 | Inventory / item selection |
| Z / C | Native camera left / right |
| Left Alt + mouse | Native first-person free look while stationary |
| F4 | Mark a playtest incident (L3 + R3) |

Click once in gameplay to capture the mouse. That first click is consumed;
subsequent clicks perform their assigned actions. Escape releases the cursor.
Pause, inventory, cutscenes and room loading keep the cursor captured while
camera input is suspended. Escape, focus loss, host overlays and right-stick
takeover release it; click again in gameplay to capture. Movement from a
transition is discarded. Mouse look follows mouse direction by default, with
velocity smoothing and a short release tail. Sensitivity and separate mouse
axis reversal options are available in the feature's options. Old v0.2.2
`invert-x`/`invert-y` settings reset to the corrected defaults.

All five mouse buttons have action choices in **Mouse and Keyboard → Options**:
attack/interact, secondary/charge, jump, defend/dash, weapon switch, Dan-Hand,
target, strafe, inventory, pause or unbound. The first left click always captures
the cursor and is consumed regardless of its assigned action. Mouse controls
use a temporary session profile, so adjust these choices in the mod options.

WASD uses only the left stick in gameplay, with normalized diagonal movement;
menus use only the D-pad. Walking and camera look can run together. Game-specific
action meanings are listed here and in the mod's launcher description.

The shortcuts use the original game's actions and its existing inventory,
weapon pair and unlock checks. Camera collision and authored camera restrictions
remain in effect. These are local offline controls; diagnostic replay supplies
its recorded pad state through the normal input boundary.

Native mappings were checked against the [original press-kit controls](https://medievil.wiki/w/MediEvil_2_Press_Kit)
and [instruction manual](https://medievil.wiki/w/MediEvil_2_manual), including
Circle for jump, Cross for attack and L1 + Square to switch weapons.
