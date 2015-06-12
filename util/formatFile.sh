#!/bin/sh -e
CLANG_FORMAT=${CLANG_FORMAT-$(which clang-format || which clang-format-3.6 || which clang-format-3.5 || which clang-format-3.4)}
"${CLANG_FORMAT}" -i \
  -style="file" \
  "$@"
