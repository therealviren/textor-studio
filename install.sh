#!/usr/bin/env bash
set -euo pipefail

REPO_URL="https://github.com/therealviren/textor-studio.git"
APP_NAME="tse"

msg() {
  printf '%s\n' "$1"
}

err() {
  printf '%s\n' "$1" >&2
  exit 1
}

have() {
  command -v "$1" >/dev/null 2>&1
}

is_termux() {
  [ -n "${PREFIX-}" ] && [ -d "${PREFIX:-/data/data/com.termux/files/usr}" ]
}

is_root() {
  [ "${EUID:-$(id -u)}" -eq 0 ]
}

run_privileged() {
  if is_root; then
    "$@"
  elif have sudo; then
    sudo "$@"
  else
    err "Administrator access is required."
  fi
}

detect_platform() {
  if is_termux; then
    printf '%s\n' "termux"
    return
  fi

  case "$(uname -s 2>/dev/null || true)" in
    Linux*) printf '%s\n' "linux" ;;
    Darwin*) printf '%s\n' "macos" ;;
    *) printf '%s\n' "unknown" ;;
  esac
}

install_dependencies() {
  platform="$1"

  if have cmake && have git && (have g++ || have clang++); then
    return
  fi

  msg "Installing dependencies..."

  case "$platform" in
    termux)
      run_privileged pkg update -y
      run_privileged pkg install -y git cmake clang make ninja
      ;;
    linux)
      if have apt-get; then
        run_privileged apt-get update -y
        run_privileged apt-get install -y git cmake g++ make ninja-build
      elif have pacman; then
        run_privileged pacman -Sy --noconfirm git cmake gcc make ninja
      elif have dnf; then
        run_privileged dnf install -y git cmake gcc-c++ make ninja-build
      elif have zypper; then
        run_privileged zypper --non-interactive install git cmake gcc-c++ make ninja
      else
        err "No supported package manager found."
      fi
      ;;
    macos)
      if have brew; then
        brew install git cmake ninja
      else
        err "Homebrew is required on macOS."
      fi
      ;;
    *)
      err "Unsupported platform."
      ;;
  esac
}

find_source_dir() {
  if [ -f "./CMakeLists.txt" ] && [ -f "./install.sh" ] && [ -d "./src" ]; then
    printf '%s\n' "$PWD"
    return
  fi
  printf '%s\n' ""
}

choose_prefix() {
  platform="$1"

  if [ "$platform" = "termux" ]; then
    printf '%s\n' "${PREFIX:-/data/data/com.termux/files/usr}"
    return
  fi

  if is_root || [ -w /usr/local ] 2>/dev/null; then
    printf '%s\n' "/usr/local"
    return
  fi

  printf '%s\n' "${HOME}/.local"
}

main() {
  msg "Textor Studio installer"

  platform="$(detect_platform)"
  install_dependencies "$platform"

  source_dir="$(find_source_dir)"
  temp_dir=""
  cleanup() {
    if [ -n "${temp_dir:-}" ] && [ -d "$temp_dir" ]; then
      rm -rf "$temp_dir"
    fi
  }
  trap cleanup EXIT INT TERM

  if [ -z "$source_dir" ]; then
    temp_dir="$(mktemp -d "${TMPDIR:-/tmp}/textor-studio.XXXXXX")"
    msg "Cloning source..."
    git clone --depth 1 "$REPO_URL" "$temp_dir"
    source_dir="$temp_dir"
  fi

  build_dir="$(mktemp -d "${TMPDIR:-/tmp}/textor-build.XXXXXX")"
  prefix="$(choose_prefix "$platform")"

  generator_args=()
  if have ninja; then
    generator_args=(-G Ninja)
  fi

  if [ "$platform" = "macos" ] && have brew && [ -x "$(brew --prefix llvm)/bin/clang++" ]; then
    export CC="$(brew --prefix llvm)/bin/clang"
    export CXX="$(brew --prefix llvm)/bin/clang++"
  else
    if have clang++; then
      export CXX="clang++"
    else
      export CXX="g++"
    fi
  fi

  msg "Configuring..."
  cmake -S "$source_dir" -B "$build_dir" \
    "${generator_args[@]}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$prefix"

  msg "Building..."
  cmake --build "$build_dir" --parallel

  msg "Installing..."
  if [ "$prefix" = "/usr/local" ] && ! is_root; then
    run_privileged cmake --install "$build_dir"
  else
    cmake --install "$build_dir"
  fi

  msg "Done."
  msg "Run: $APP_NAME"
  if [ "$prefix" = "${HOME}/.local" ]; then
    msg "If needed, add ${HOME}/.local/bin to your PATH."
  fi
}

main "$@"