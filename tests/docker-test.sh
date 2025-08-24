#!/usr/bin/env bash
set -euo pipefail

# cd to this script's dir
cd "$(cd "$(dirname "$0")" && pwd)"

# --------------------------------------------------------------------
# Args: --clang <ver> --gcc <ver>
#   Examples:
#     ./docker-test.sh --clang 4.0 --gcc 9.4
#     ./docker-test.sh --gcc 9
#     ./docker-test.sh --clang 11
# --------------------------------------------------------------------
CLANG_WANT=""
GCC_WANT=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --clang) CLANG_WANT="${2:-}"; shift 2 ;;
    --gcc)   GCC_WANT="${2:-}";   shift 2 ;;
    -h|--help)
      echo "usage: $0 [--clang X[.Y]] [--gcc X[.Y]]"
      exit 0
      ;;
    *) 
      echo "unknown arg: $1" >&2; 
      exit 2
      ;;
  esac
done

have() { command -v "$1" >/dev/null 2>&1; }

# OS info (for LTO heuristics)
if [[ -r /etc/os-release ]]; then . /etc/os-release; else PRETTY_NAME=""; ID=""; VERSION_ID=""; fi
echo "# --------------------------------------------------------------------"
echo "# Current OS: ${PRETTY_NAME:-}"
echo "# --------------------------------------------------------------------"

# ----------------------------- Clang --------------------------------
check_clang() {
  if ! have clang; then
    echo "clang: not found (skipping)"
    return 0
  fi

  echo "==> Clang toolchain:"
  clang --version | head -n1 || true
  clang++ --version | head -n1 || true

  if [[ -n "${CLANG_WANT}" ]]; then
    local line; line="$(clang --version | head -n1)"
    if [[ "$line" != *"clang version ${CLANG_WANT}"* ]]; then
      echo "Incorrect clang version:"
      echo "  got:  $line"
      echo "  need: (... clang version ${CLANG_WANT})"
      exit 1
    fi
    echo "Clang version OK."
  else
    echo "Clang version check skipped (no --clang provided)."
  fi

  echo "==> Clang: C compile/run"
  clang  docker-test.c   -o test && ./test && rm -f test

  echo "==> Clang: C++ compile/run"
  clang++ docker-test.cpp -o test && ./test && rm -f test

  echo "==> Clang ThinLTO test"
  if command -v ld.lld >/dev/null 2>&1; then
    clang++ -O2 -flto=thin -fuse-ld=lld docker-test.cpp -o test
    ./test
  elif command -v ld.gold >/dev/null 2>&1; then
    PLUGIN=$(echo /usr/lib/llvm-*/lib/LLVMgold.so | head -n1)
    if [ -f "$PLUGIN" ]; then
      clang++ -O2 -flto=thin -fuse-ld=gold -Wl,-plugin,"$PLUGIN" docker-test.cpp -o test
      ./test
    else
      echo "Skipping ThinLTO: LLVMgold.so not found."
    fi
  else
    echo "Skipping ThinLTO: neither lld nor gold present."
  fi
  rm -f test

  echo "Clang checks passed."
  echo
}

# ------------------------------ GCC ---------------------------------
check_gcc() {
  if ! have gcc; then
    echo "gcc: not found (skipping)"
    return 0
  fi

  echo "==> GCC toolchain:"
  gcc --version | head -n1 || true
  g++ --version | head -n1 || true

  if [[ -n "${GCC_WANT}" ]]; then
    # Prefer full version; fallback to -dumpversion on older gcc
    local full
    full="$(gcc -dumpfullversion 2>/dev/null || gcc -dumpversion)"
    local got_major="${full%%.*}"
    local rest="${full#*.}"
    local got_mm
    if [[ "$rest" != "$full" ]]; then
      local got_minor="${rest%%.*}"
      got_mm="${got_major}.${got_minor}"
    else
      got_mm="${got_major}"
    fi

    if [[ "${GCC_WANT}" == *.* ]]; then
      # Expect exact major.minor (e.g., 9.4)
      if [[ "$got_mm" != "$GCC_WANT" ]]; then
        echo "Incorrect gcc version (major.minor):"
        echo "  got:  $got_mm   (full: $full)"
        echo "  need: $GCC_WANT"
        exit 1
      fi
    else
      # Expect matching major only (e.g., 9)
      if [[ "$got_major" != "$GCC_WANT" ]]; then
        echo "Incorrect gcc version (major):"
        echo "  got:  $got_major   (full: $full)"
        echo "  need: $GCC_WANT"
        exit 1
      fi
    fi
    echo "GCC version OK."
  else
    echo "GCC version check skipped (no --gcc provided)."
  fi

  echo "==> GCC: C compile/run"
  gcc  docker-test.c   -o test
  ./test
  rm -f test

  echo "==> GCC: C++ compile/run"
  g++  docker-test.cpp -o test
  ./test
  rm -f test

  echo "==> GCC LTO test"
  g++ -O2 -flto docker-test.cpp -o test
  ./test
  rm -f test

  echo "GCC checks passed."
  echo
}

check_clang
check_gcc

echo "All requested compiler checks completed successfully."
