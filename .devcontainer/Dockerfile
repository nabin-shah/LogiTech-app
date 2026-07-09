FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Core build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    make \
    cmake \
    ninja-build \
    pkg-config \
    git \
    gdb \
    clangd \
    # Qt 6 development
    qt6-base-dev \
    qt6-declarative-dev \
    qt6-svg-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    qt6-l10n-tools \
    qml6-module-qtquick \
    qml6-module-qtquick-controls \
    qml6-module-qtquick-layouts \
    qml6-module-qtquick-window \
    qml6-module-qtquick-templates \
    qml6-module-qtqml-workerscript \
    qml6-module-qtquick-dialogs \
    qml6-module-qt5compat-graphicaleffects \
    qt6-qpa-plugins \
    libqt6opengl6-dev \
    libqt6svg6-dev \
    libqt6dbus6 \
    libqt6widgets6 \
    libxkbcommon-dev \
    # Qt QML language server
    qml6-module-qttest \
    # System dependencies
    libudev-dev \
    # Google Test
    libgtest-dev \
    # Shell and CLI tools
    fish \
    bat \
    eza \
    fd-find \
    ripgrep \
    fzf \
    # Fonts (emoji + icons)
    fonts-noto-color-emoji \
    unzip \
    # Utilities
    sudo \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Build and install GTest (Ubuntu ships source only)
RUN cd /usr/src/googletest && cmake -B build && cmake --build build && cmake --install build

# Install Nerd Font for prompt icons
RUN mkdir -p /usr/share/fonts/nerd && \
    curl -fLo /tmp/meslo.zip https://github.com/ryanoasis/nerd-fonts/releases/latest/download/Meslo.zip && \
    unzip -q /tmp/meslo.zip -d /usr/share/fonts/nerd && \
    rm /tmp/meslo.zip && \
    fc-cache -f

# Rename ubuntu user to match host username (empty or 'dev' in Codespaces → keep ubuntu)
ARG USERNAME
RUN ACTUAL_USER="${USERNAME:-ubuntu}" \
    && if [ "$ACTUAL_USER" != "ubuntu" ]; then \
        usermod -l $ACTUAL_USER -d /home/$ACTUAL_USER -m ubuntu 2>/dev/null && \
        groupmod -n $ACTUAL_USER ubuntu 2>/dev/null; \
    fi \
    && echo "ALL ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers.d/all-users \
    && chsh -s /usr/bin/fish $ACTUAL_USER

# Install Fisher + Pure prompt, force multi-line
RUN ACTUAL_USER="${USERNAME:-ubuntu}" \
    && su - $ACTUAL_USER -c 'fish -c "curl -sL https://raw.githubusercontent.com/jorgebucaran/fisher/main/functions/fisher.fish | source && fisher install pure-fish/pure"' || true \
    && su - $ACTUAL_USER -c 'mkdir -p ~/.config/fish/functions && echo "function _pure_is_single_line_prompt; return 1; end" > ~/.config/fish/functions/_pure_is_single_line_prompt.fish'

# Fish config with aliases matching CachyOS defaults
COPY devcontainer-fish.fish /etc/fish/conf.d/logitune-dev.fish

# Environment for headless Qt
ENV QT_QPA_PLATFORM=offscreen
ENV CMAKE_EXPORT_COMPILE_COMMANDS=ON

WORKDIR /workspaces/logitune
