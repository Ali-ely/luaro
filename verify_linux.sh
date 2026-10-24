#!/bin/sh
set -eu
cd "$(dirname "$0")"

command -v g++ >/dev/null || { echo 'g++ is required.' >&2; exit 1; }
command -v valgrind >/dev/null || { echo 'Valgrind is required (or add its bin directory to PATH).' >&2; exit 1; }
mkdir -p build/linux
core='Player.cpp Teams.cpp Roles.cpp World.cpp Event.cpp Command.cpp Capabilities.cpp Console.cpp'
for milestone in 1 2 3 4 5 6 7 8 9; do
    sources="$core tests/m${milestone}_tests.cpp"
    if [ "$milestone" = 2 ]; then sources='tests/m2_tests.cpp'; fi
    # Source names are fixed project paths without spaces; split this list deliberately.
    g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. -g -fsanitize=address,undefined -fno-omit-frame-pointer $sources -o "build/linux/m${milestone}_asan"
    ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "build/linux/m${milestone}_asan"
    g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. -g $sources -o "build/linux/m${milestone}_tests"
    valgrind --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --error-exitcode=1 "build/linux/m${milestone}_tests"
done
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror $core main.cpp -o build/linux/simworld
build/linux/simworld --demo >build/linux/demo.txt
echo 'All Linux sanitizer and Valgrind checks passed.'
