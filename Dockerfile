# PCB CAD Development Environment
# Based on Ubuntu 22.04 LTS
FROM ubuntu:22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Set locale
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

# Install build dependencies
RUN apt-get update && apt-get install -y \
    # Build essentials
    build-essential \
    autoconf \
    automake \
    autopoint \
    libtool \
    pkg-config \
    # Parser generators
    flex \
    bison \
    # Internationalization
    gettext \
    intltool \
    # Text processing
    m4 \
    gawk \
    # Version control
    git \
    # Core libraries
    libglib2.0-dev \
    # GTK GUI dependencies
    libgtk2.0-dev \
    libgtkglext1-dev \
    # Graphics libraries
    libgd-dev \
    libcairo2-dev \
    # IPC
    libdbus-1-dev \
    # Lesstif GUI dependencies (optional)
    lesstif2-dev \
    libxmu-dev \
    libxt-dev \
    libxrender-dev \
    libxinerama-dev \
    libxpm-dev \
    # Testing tools
    gerbv \
    imagemagick \
    # Utilities
    wish \
    tcl \
    vim \
    nano \
    less \
    # Development tools
    gdb \
    valgrind \
    strace \
    # Code quality tools
    cppcheck \
    clang-tidy \
    # Documentation tools (optional, can be disabled with --disable-doc)
    texinfo \
    texlive-latex-base \
    texlive-latex-extra \
    && rm -rf /var/lib/apt/lists/*

# Create a non-root user for development
RUN useradd -m -s /bin/bash developer && \
    echo "developer ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Set working directory
WORKDIR /workspace

# Switch to non-root user
USER developer

# Set up git configuration (user should override)
RUN git config --global user.name "PCB Developer" && \
    git config --global user.email "dev@example.com" && \
    git config --global init.defaultBranch main

# Add helpful aliases
RUN echo 'alias ll="ls -la"' >> ~/.bashrc && \
    echo 'alias gs="git status"' >> ~/.bashrc && \
    echo 'alias gl="git log --oneline --graph --decorate -20"' >> ~/.bashrc

# Create a startup message
RUN echo 'echo "PCB CAD Development Environment"' >> ~/.bashrc && \
    echo 'echo "================================"' >> ~/.bashrc && \
    echo 'echo ""' >> ~/.bashrc && \
    echo 'echo "Quick start:"' >> ~/.bashrc && \
    echo 'echo "  ./autogen.sh                    # Generate configure script"' >> ~/.bashrc && \
    echo 'echo "  ./configure --disable-doc       # Configure build"' >> ~/.bashrc && \
    echo 'echo "  make -j\$(nproc)                 # Build"' >> ~/.bashrc && \
    echo 'echo "  make check                      # Run tests"' >> ~/.bashrc && \
    echo 'echo ""' >> ~/.bashrc

# Default command
CMD ["/bin/bash"]
