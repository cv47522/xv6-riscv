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

# Every link target in a markdown file, one per line, in both spellings:
# inline `](target)` and reference definitions `[label]: target`. The
# reference form is what every chapter stub uses for its single most
# load-bearing link, the `[ostep]:` line at the bottom. Fenced code blocks
# are skipped, because README.md's shape template quotes an illustrative
# `[ostep-x]: .../<Note>.md#<anchor>` that is not meant to resolve.
link_targets() {
  awk '
    /^[[:space:]]*```/ { fence = !fence; next }
    fence { next }
    {
      if ($0 ~ /^\[[^]]+\]:[[:space:]]*[^[:space:]]+/) {
        t = $0
        sub(/^\[[^]]+\]:[[:space:]]*/, "", t)
        sub(/[[:space:]].*$/, "", t)
        print t
      }
      line = $0
      while (match(line, /\]\([^)]+\)/)) {
        print substr(line, RSTART + 2, RLENGTH - 3)
        line = substr(line, RSTART + RLENGTH)
      }
    }
  ' "$1"
}

# Every GitHub-style anchor a markdown file generates, one per line:
# lowercase the heading text, drop anything that is not alphanumeric,
# space, or hyphen, then turn spaces into hyphens. Headings inside fenced
# code blocks are skipped for the same reason as above.
anchors_of() {
  awk '
    /^[[:space:]]*```/ { fence = !fence; next }
    fence { next }
    /^#{1,6}[[:space:]]/ {
      sub(/^#{1,6}[[:space:]]+/, "")
      sub(/[[:space:]]+$/, "")
      print
    }
  ' "$1" |
    tr '[:upper:]' '[:lower:]' |
    sed -E 's/[^a-z0-9 -]//g; s/ /-/g'
}

# 1. Every relative markdown link resolves from the file that contains it,
#    and any '#fragment' on a target inside this repository names a heading
#    that actually exists.
#    docs/superpowers/ is excluded: specs and plans quote example links
#    that are illustrations, not navigation.
check_links() {
  echo "== relative links resolve =="
  find "$REPO/docs" -name '*.md' -not -path '*/superpowers/*' -print0 |
  while IFS= read -r -d '' f; do
    link_targets "$f" |
    while IFS= read -r target; do
      case "$target" in http*|mailto:*) continue ;; esac
      path="${target%%#*}"
      case "$target" in *#*) frag="${target#*#}" ;; *) frag="" ;; esac

      if [ -n "$path" ]; then
        resolved="$(dirname "$f")/$path"
        [ -e "$resolved" ] || { bad "${f#"$REPO"/} -> $target"; continue; }
      else
        resolved="$f"  # bare '#fragment' points into the containing file
      fi

      # Fragments are only verifiable against .md files in this repository.
      [ -n "$frag" ] || continue
      case "$resolved" in *.md) ;; *) continue ;; esac
      abs="$(cd "$(dirname "$resolved")" && pwd)/$(basename "$resolved")"
      case "$abs" in "$REPO"/*) ;; *) continue ;; esac

      anchors_of "$abs" | grep -qxF "$frag" ||
        bad "${f#"$REPO"/} -> $target (no heading generates #$frag)"
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

# 4. Concordance statuses match the state of the OSTEP notes, in both
#    directions: no 'linked' row may point at a placeholder, and no 'loan'
#    row may point at a note that has since been written — that second case
#    is the moment the debt-repayment procedure is supposed to fire.
check_status() {
  echo "== concordance status matches the OSTEP notes =="
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
  grep -E '\| *loan *\|[[:space:]]*$' "$CONC" |
  while IFS= read -r row; do
    for tok in $(printf '%s\n' "$row" | grep -oE '`[^` ]+\.md`' | tr -d '`'); do
      if [ ! -e "$OSTEP/$tok" ]; then
        bad "no such OSTEP note: $tok"
      elif ! grep -q 'Placeholder — not yet written' "$OSTEP/$tok"; then
        bad "$tok is written but its row status is still 'loan' — move the loaned material into it, link to it, and flip the row to 'linked'"
      fi
    done
  done
}

# 5. The README's per-chapter Case column is derived from the concordance's
#    per-concept Status column, so the two can drift. A chapter whose
#    concordance rows all agree must carry that value; one whose rows
#    disagree must read 'mixed'.
check_cases() {
  echo "== README Case column agrees with concordance Status =="
  [ -f "$README" ] || { bad "missing $README"; return; }
  [ -f "$CONC" ] || { bad "missing $CONC"; return; }
  local num rcase statuses count expected
  while read -r num rcase; do
    statuses="$(awk -F'|' -v ch="$num" 'NF >= 7 {
      c = $4; gsub(/[[:space:]]/, "", c)
      s = $7; gsub(/[[:space:]]/, "", s)
      if (c == ch && (s == "linked" || s == "loan" || s == "xv6-only")) print s
    }' "$CONC" | sort -u)"
    if [ -z "$statuses" ]; then
      bad "README chapter $num has no concordance rows"
      continue
    fi
    count="$(printf '%s\n' "$statuses" | wc -l)"
    if [ "$count" -eq 1 ]; then expected="$statuses"; else expected="mixed"; fi
    [ "$rcase" = "$expected" ] ||
      bad "README chapter $num Case is '$rcase' but the concordance says '$expected'"
  done < <(awk -F'|' 'NF >= 6 && $2 ~ /^[[:space:]]*\*\*[0-9]+\*\*[[:space:]]*$/ {
    num = $2; gsub(/[^0-9]/, "", num)
    c = $5; gsub(/^[[:space:]]+/, "", c); gsub(/[[:space:]]+$/, "", c)
    print num, c
  }' "$README")
}

# 6. The reverse direction: the backlinks in ../operating-system/*.md that
#    point back into this repository. Renaming a chapter stub breaks them
#    all with no signal from this side. Skipped, not failed, when that
#    repository is not checked out — this script must work standalone.
check_backlinks() {
  echo "== OSTEP backlinks into this repo resolve =="
  if [ ! -d "$OSTEP" ]; then
    echo "  SKIP: $OSTEP is not present"
    return
  fi
  for f in "$OSTEP"/*.md; do
    [ -e "$f" ] || continue
    grep -oE '\]\(\.\./xv6-riscv/[^)]+\)' "$f" 2>/dev/null |
      sed -E 's/^\]\(//; s/\)$//' |
    while IFS= read -r target; do
      path="${target%%#*}"
      [ -z "$path" ] && continue
      [ -e "$OSTEP/$path" ] || bad "backlink ${f##*/} -> $target"
    done
  done
}

# 7. The system call reference stays in step with the tree, in both
#    directions. Every stub generated by user/usys.pl must have a row, so a
#    system call added during a lab cannot go undocumented; and every call
#    number quoted in the reference must be the number kernel/syscall.h
#    actually defines, so a renumbering cannot leave a stale table behind.
check_syscalls() {
  echo "== syscall reference matches the tree =="
  local ref="$REPO/docs/05-syscall-reference.md"
  [ -f "$ref" ] || { bad "missing $ref"; return; }

  # Forward: every generated stub is documented.
  sed -n 's/^entry("\([a-z_]*\)").*/\1/p' "$REPO/user/usys.pl" |
  while IFS= read -r name; do
    grep -qF "\`$name()\`" "$ref" ||
      bad "05-syscall-reference.md has no row for \`$name()\`"
  done

  # Numbers: what the reference quotes is what syscall.h defines.
  sed -n 's/^#define[[:space:]]\{1,\}SYS_\([a-z]\{1,\}\)[[:space:]]\{1,\}\([0-9]\{1,\}\).*/\1 \2/p' \
    "$REPO/kernel/syscall.h" |
  while read -r name num; do
    grep -qF "\`SYS_$name\` = $num" "$ref" ||
      bad "05-syscall-reference.md: SYS_$name should read '= $num' (kernel/syscall.h)"
  done

  # Reverse: the reference names no call that has since been removed.
  grep -oE '`SYS_[a-z]+`' "$ref" | tr -d '`' | sort -u |
  while IFS= read -r sym; do
    grep -qE "^#define[[:space:]]+$sym[[:space:]]" "$REPO/kernel/syscall.h" ||
      bad "05-syscall-reference.md names $sym, absent from kernel/syscall.h"
  done
}

check_links
check_chapters
check_symbols
check_status
check_cases
check_backlinks
check_syscalls

echo
if [ -s "$FAILS" ]; then
  echo "FAILED — $(wc -l <"$FAILS") problem(s)."
  exit 1
fi
echo "All checks passed."
