# Window Manager

Linux like window manager for Windows Operating System. 

ALT+LBUTTON : MOVE

ALT+RBUTTON : RESIZE

ALT+P : PAUSE/RESUME Process 

ALT+LSHIFT+K : KILL Process

ALT+T : Set on top and vice versa

## First week (18-24 January)
- Completed 
    1. First running version
- Problems
    1. Full screen windows cannot be moved before exiting from full screen.  <span style="color:green"> [Solved]</span>
    2. Mouse clicks should be disabled while moving <span style="color:green"> [Solved]</span>
    3. When window is disabled, every click causes system beep sound <span style="color:green"> [Solved]</span>
    4. Solution2 causes some distruption for scenarios where user can use other ALT+MOUSE shortcuts e.g. File Properties shortcut is ALT + Clicking on File
- Solutions
    1. ShowWindow API with SW_RESTORE parameter exits from Full Screen
    2. EnableWindow API helps enabling/disabling window inputs
    3. System beep sound path stored on Registery at `AppEvents\\Schemes\\Apps\\.Default\\.Default\\.Current`. We remove it meanwhile ALT key is on. And restore after.

## TODO
1. Basic setup
2. Execute on startup option
3. Problem3's distruption should be fixed
4. Performance optimization
5. Prevent multiple instances
6. Build instructions for README
7. Better Handling on full Screen
8. Check whether mouse pointer on the frame
9. Deactivate if CTRL, SHIFT, TAB keys were pressed beside of ALT
10. Additional Functions: Set window on top, pause, resume process 

## PREVIEW
![DEMO](a.gif)