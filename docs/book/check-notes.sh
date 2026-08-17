#!/usr/bin/env bash
# Structural checks for the cross-repo note system.
# Usage: docs/book/check-notes.sh
set -uo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BOOK="$REPO/../xv6-riscv-book"
OSTEP="$REPO/../operating-system"
README="$REPO/docs/book/README.md"
CONC="$REPO/docs/book/00-ostep-concordance.md"

FAILS="$(mktemp)"
trap 'rm -f "$FAILS"' EXIT

bad() {
  printf '  FAIL: %s\n' "$*"
  printf '  FAIL: %s\n' "$*" >>"$FAILS"
}

# 1. Every relative markdown link resolves from the file that contains it.
#    docs/superpowers/ is excluded: specs and plans quote example links
#    that are illustrations, not navigation.
check_links() {
  echo "== relative links resolve =="
  find "$REPO/docs" -name '*.md' -not -path '*/superpowers/*' -print0 |
  while IFS= read -r -d '' f; do
    grep -oE '\]\([^)]+\)' "$f" 2>/dev/null |
      sed -E 's/^\]\(//; s/\)$//' |
    while IFS= read -r target; do
      case "$target" in http*|mailto:*|'#'*) continue ;; esac
      path="${target%%#*}"
      [ -z "$path" ] && continue
      [ -e "$(dirname "$f")/$path" ] || bad "${f#"$REPO"/} -> $target"
    done
  done
}

# 2. The README chapter table matches book.tex, in include order.
check_chapters() {
  echo "== README chapter table matches book.tex =="
  [ -f "$README" ] || { bad "missing $README"; return; }
  local n=0 stem title
  while IFS= read -r stem; do
    n=$((n + 1))
    title="$(sed -n 's/^\\chapter{\(.*\)}[[:space:]]*$/\1/p' "$BOOK/$stem.tex" | head -1)"
    [ -n "$title" ] || { bad "no \\chapter found in $stem.tex"; continue; }
    grep -qE "^\| \*\*$n\*\*[^|]*\| *$title *\|" "$README" ||
      bad "chapter $n should read '$title' (from $stem.tex)"
  done < <(sed -n 's#^\\input{latex.out/\([a-z0-9]*\)}#\1#p' "$BOOK/book.tex" | grep -v '^acks$')
}

# 3. Every file and symbol cited in the concordance actually exists.
#    Only this tree's own source is checked — tokens under kernel/ and
#    user/. The Exercise column cites ../operating-system/codes and
#    ../ostep-homework, which do not resolve against this repo and are
#    informational.
check_symbols() {
  echo "== concordance code references exist =="
  [ -f "$CONC" ] || { bad "missing $CONC"; return; }
  grep -E '^\|' "$CONC" |
  while IFS= read -r row; do
    file=""
    for tok in $(printf '%s\n' "$row" | grep -oE '`[^` ]+`' | tr -d '`'); do
      # Path tokens are checked only under kernel/ and user/; a bare
      # sym() token is checked against the last such path on the row.
      case "$tok" in
        kernel/*|user/*|*'()') ;;
        *) file=""; continue ;;
      esac
      case "$tok" in
        *.c|*.h|*.S|*.c:[0-9]*|*.h:[0-9]*|*.S:[0-9]*|*.ld)
          file="${tok%%:*}"
          [ -e "$REPO/$file" ] || bad "no such file: $file"
          ;;
        *'()')
          sym="${tok%'()'}"
          [ -n "$file" ] && [ -e "$REPO/$file" ] || continue
          grep -qE "\b${sym}[[:space:]]*\(" "$REPO/$file" ||
            bad "$sym not found in $file"
          ;;
      esac
    done
  done
}

# 4. No row claims 'linked' while its OSTEP note is still a placeholder.
check_status() {
  echo "== no 'linked' row points at a placeholder note =="
  [ -f "$CONC" ] || return
  grep -E '\| *linked *\|[[:space:]]*$' "$CONC" |
  while IFS= read -r row; do
    for tok in $(printf '%s\n' "$row" | grep -oE '`[^` ]+\.md`' | tr -d '`'); do
      if [ ! -e "$OSTEP/$tok" ]; then
        bad "no such OSTEP note: $tok"
      elif grep -q 'Placeholder — not yet written' "$OSTEP/$tok"; then
        bad "$tok is a placeholder but its row status is 'linked'"
      fi
    done
  done
}

check_links
check_chapters
check_symbols
check_status

echo
if [ -s "$FAILS" ]; then
  echo "FAILED — $(wc -l <"$FAILS") problem(s)."
  exit 1
fi
echo "All checks passed."
