# Window Manager

Linux like window manager for Windows Operating System. 

ALT+LBUTTON : MOVE

ALT+RBUTTON : RESIZE

ALT+T : Set on Top and Vice Versa

ALT+P : PAUSE/RESUME Process 

ALT+CTRL+K : KILL Process

ALT+CTRL+I : KILL Window Manager Itself

ALT+CTRL+L : Show Logs



## First week (18-24 January)
- Completed 
    1. First running version
- Problems
    1. Full screen windows cannot be moved before exiting from full screen.  [Solved]
    2. Mouse clicks should be disabled while moving [Solved]
    3. When window is disabled, every click causes system beep sound [Solved]
    4. Solution2 causes some distruption for scenarios where user can use other ALT+MOUSE shortcuts e.g. File Properties shortcut is ALT + Clicking on File [Solved]
- Solutions
    1. ShowWindow API with SW_RESTORE parameter exits from Full Screen
    2. EnableWindow API helps enabling/disabling window inputs
    3. System beep sound path stored on Registery at `AppEvents\\Schemes\\Apps\\.Default\\.Default\\.Current`. We remove it meanwhile ALT key is on. And restore after.

## TODO
1. Basic setup
2. Execute on startup option
3. Performance optimization
4. Prevent multiple instances
5. Build instructions for README
6. Better Handling on full Screen
7. Check whether mouse pointer on the frame
8. Deactivate if CTRL, SHIFT, TAB keys were pressed beside of ALT
9. Additional Functions: Set window on top, pause, resume process 
10. Consider using callback via SetWindowsHookExA instead of infinite while loop

## PREVIEW
![DEMO](a.gif)