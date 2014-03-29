#!/bin/sh -e
clang-format -i \
  -style="{BasedOnStyle: llvm, AllowShortIfStatementsOnASingleLine: true, BreakBeforeBraces: Stroustrup, BreakConstructorInitializersBeforeComma: true, NamespaceIndentation: All, DerivePointerBinding: true}" \
  "$@"
