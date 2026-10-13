# Rust bindings for heimdallcore #{rust_bindings}

_libheimdallcore_ provide [Rust] bindings so as to interface with the core
implementation in C and implement Heimdall plugins in [Rust] as a crate.

The bindings are located in the `src/rust/` folder for everything which
is mapping headers from `include/heimdall_*.h`. There's no module-specific
crate for now, but those would be located in `modules/`.

[Rust]: https://www.rust-lang.org/

## Creating a new core crate

New core crate needs to be placed into the `src/rust/` directory. They
usually match with another C `heimdall_*.h` header. For instance `heimdall_tick.h`
will match with the `src/rust/heimdallrs-tick` crate.

Cargo manifest for the workspace at `src/rust/Cargo.toml` needs to be
updated to account for the new crate.

```diff
diff --git a/src/rust/Cargo.toml b/src/rust/Cargo.toml
index b1a9555a23..72ca86951d 100644
--- a/src/rust/Cargo.toml
+++ b/src/rust/Cargo.toml
@@ -1,5 +1,6 @@
 [workspace]
 members = [
+    "heimdallrs-newcrate",
     "heimdallrs-messages",
     "heimdallrs-sys-generator"
 ]
```

A core crate might depend on another core crate, in particular when a
`heimdall_*.h` header is depending on another header or uses forward
declarations.

In that case, the Cargo.toml from this crate should directly reference
the other crate using a path locator.

```toml
[package]
name = "heimdallrs-messages"
edition = "2021"
version.workspace = true
license.workspace = true

[dependencies]
heimdallrs-utils = { path = "../heimdallrs-utils" }
```

Finally, if the new crate is meant to be available from modules, it
should also be added to the `modules/Cargo.toml` workspace file using a
path locator, inside the workspace dependencies.

```toml
# modules/Cargo.toml
[workspace]
resolver = "2"
members = ["/*-rs"]

[workspace.dependencies]
heimdallrs-tick = { path = "../src/rust/heimdallrs-tick" }
heimdallrs-messages = { path = "../src/rust/heimdallrs-messages" }
# ...
```

## Module creation

To create a new module, create a new crate inside the relevant
`moduledir` folder from `modules/` folder using Cargo, ending with `-rs`
and without version control creation given that Heimdall sources are already
versioned:

```bash
cd modules/moduledir && cargo new --lib --vcs none mynewmodule-rs
```

For instance, a new video filter would be created with:

```bash
cd modules/video_filter && cargo new --lib --vcs none mynewfilter-rs
```

Then, you can modify the Cargo.toml file to include the different crates
the module will be using from the workspace.

```toml
[package]
name = "mynewmodule-rs"
edition = "2021"
version = "0.1.0"

[dependencies]
heimdallrs-messages.workspace = true
```

The module will need to be listed into `modules/Cargo.toml` members:

```toml
[workspace]
# ...
members = [
    # ...
    "moduledir/mynewmodule-rs"
]
```

Then you can add the plugin to the buildsystems. For automake, check the
parent Makefile.am or create a new one depending on how crowded it is,
and add:

```make
libmynewmodule_rs.la:
	@$(LIBTOOL_CARGO) $(srcdir)/moduledir/mynewmodule-rs/ $@

libmynewmodule_rs_plugin_la_SOURCES = \
	mynewmodule-rs/Cargo.toml \
	mynewmodule-rs/src/lib.rs 
libmynewmodule_rs_plugin_la_LIBADD = libmynewmodule_rs.la

if HAVE_RUST
moduledir_PLUGINS += libmynewmodule_rs_plugin.la
# Example: video_filter_PLUGINS += libmynewfilter_rs_plugin.la
endif
```

For meson, you will use the `heimdall_rust_modules` dictionary from the
parent meson.build file:

```
heimdall_rust_modules += {
    'name' : 'mynewmodule_rs',
    'sources' : files('mynewmodule-rs/src/lib.rs'),
    'cargo_toml' : files('mynewmodule-rs/Cargo.toml'),
}
```

Creating a new plugin requires the definition of the plugin manifest to
expose the different modules from there. This is done through the
`module!{}` macro:

```rust
use heimdallrs_plugin::module;
use heimdallrs_video_filter::FilterModule;

pub struct MyNewModule {};

impl FilterModule for MyNewModule {
    fn open<'a> (filter: Filter<'a>) -> Result<()> {
        todo!();
    }
}

module! {
    type: MyNewModule,
    capability: "video filter" @ 0,
    category: SUBCAT_VIDEO_FILTER,
    description: "A new module",
    shortname: "mynewmodule",
    shortcuts: ["mynewmodule_filter"],
}
```
