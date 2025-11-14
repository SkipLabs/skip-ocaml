#!/bin/bash
set -e

usage() {
    cat <<'EOF'
Usage: run_tests.sh [--keep-rheaps] [--]

Options:
  --keep-rheaps   Leave generated .rheap files in place after tests finish.
                  By default they are deleted at exit so subsequent runs start
                  from a clean slate.
  --help          Show this message and exit.
EOF
}

if [ -t 1 ] && [ -z "${NO_COLOR:-}" ]; then
    GREEN="\033[32m"
    RED="\033[31m"
    YELLOW="\033[33m"
    CYAN="\033[36m"
    BOLD="\033[1m"
    RESET="\033[0m"
else
    GREEN=""
    RED=""
    YELLOW=""
    CYAN=""
    BOLD=""
    RESET=""
fi

KEEP_RHEAPS=0
while [ $# -gt 0 ]; do
    case "$1" in
        --keep-rheaps)
            KEEP_RHEAPS=1
            shift
            ;;
        --help)
            usage
            exit 0
            ;;
        --)
            shift
            break
            ;;
        -*)
            printf "%b\n" "${RED}Unknown option:${RESET} $1"
            exit 2
            ;;
        *)
            break
            ;;
    esac
done

if [ "$KEEP_RHEAPS" -eq 0 ]; then
    cleanup_rheaps() {
        find . -name '*.rheap' -delete
    }
    trap cleanup_rheaps EXIT
fi

printf "%b\n" "${BOLD}${CYAN}Running all tests...${RESET}"

for src in src/test/*.ml; do
    base=$(basename "$src" .ml)
    bin="./build/$base"
    printf "%b\n" "${CYAN}----------------------------------------${RESET}"
    printf "%b\n" "${BOLD}${CYAN}Running ${bin}...${RESET}"
    if [ -x "$bin" ]; then
        status=0
        output=$( { "$bin"; } 2>&1 ) || status=$?
        if echo "$output" | grep -q "Error in child: exited with code 2"; then
            echo "$output"
            printf "%b\n" "${YELLOW}[EXPECTED FAILURE]${RESET} ${base}: Error in child: exited with code 2"
            status=0
        elif [ "$status" -ne 0 ]; then
            echo "$output"
            printf "%b\n" "${RED}[FAIL]${RESET} ${base} exited with code ${status}"
            exit 1
        else
            echo "$output"
            printf "%b\n" "${GREEN}[OK]${RESET} ${base}"
        fi
    else
        printf "%b\n" "${RED}[MISSING]${RESET} ${bin} not found or not executable"
        exit 1
    fi
done

printf "%b\n" "${BOLD}${GREEN}All tests passed (including expected failures).${RESET}"
