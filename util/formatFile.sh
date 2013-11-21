#!/bin/sh -e
clang-format -i \
  -style="{BasedOnStyle: llvm, BreakConstructorInitializersBeforeComma: true, NamespaceIndentation: All, PointerBindsToType: true}" \
  "$@"
