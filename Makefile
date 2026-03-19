.PHONY: build install uninstall run clean

# Build debug (fast, for development)
build:
	cargo build

# Build release + install as Linux app
install:
	chmod +x install.sh
	./install.sh

# Uninstall
uninstall:
	./install.sh --uninstall

# Run debug build directly
run:
	cargo run

# Run release build
run-release:
	cargo build --release && ./target/release/hyprland-settings

# Clean build artifacts
clean:
	cargo clean
