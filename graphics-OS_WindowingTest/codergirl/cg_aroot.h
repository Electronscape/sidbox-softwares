/*///////////////////////////////////////////////////////////////

    CODER-GIRL V1.0  (CG)

    Windowing + gadget management system.
    Provides:
        - Windows (create, destroy, z-order, focus, paint, hit-test)
        - Gadgets:
            * Button
            * RadioButton
            * Checkbox
            * BitmapView
            * ListBox

    cgroot.h is the shared root include:
        - common types/structs used across CG modules
        - shared constants and compile-time configuration
        - extern declarations for cross-module globals (sparingly)

    Rules:
        - Keep cgroot.h small and stable. If it grows teeth, split it.
        - Do NOT include heavy module headers here unless required.
        - Anything declared extern here must have exactly one definition
          in a .c/.cpp file (and be documented).

///////////////////////////////////////////////////////////////*/

