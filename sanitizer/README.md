All `.a` files in this directory are empty shims.

They are required because linker will unconditionally add them when sanitizer is enabled.

But due to QMK's build system, generating `lib<name>.a` from our source is not feasible.
