TEST_EXTENSIONS += .cargo
CARGO_LOG_DRIVER = env top_builddir="${abs_top_builddir}" \
                   python3 $(abs_top_srcdir)/buildsystem/cargo-test.py \
                   --host="@RUST_TARGET@" \
                   --working-directory="${abs_top_srcdir}/src/rust/"

heimdallrs-core.cargo:
heimdallrs-macros.cargo:
heimdallrs-messages.cargo:
heimdallrs-utils.cargo:
	cd $(top_srcdir)/src/rust/$(@:.cargo=) && env top_builddir="${abs_top_builddir}" \
         $(CARGO) build --target=$(RUST_TARGET)

if HAVE_RUST
TESTS += \
	heimdallrs-core.cargo \
	heimdallrs-macros.cargo \
	heimdallrs-messages.cargo \
	heimdallrs-utils.cargo
endif
