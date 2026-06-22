#!/bin/bash
# Show diff statistics between current KiCad working tree and last upstream commit
#
# This helps track how far our KiCad fork has diverged from upstream.
# We want to keep changes minimal to ease future porting.
#
# Note: Compares working tree (including uncommitted changes) vs upstream

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
KICAD_DIR="$PROJECT_ROOT/kicad"
LOGS_DIR="$PROJECT_ROOT/logs/kicad-diff"

# Create logs directory
mkdir -p "$LOGS_DIR"

# Log file with timestamp
TIMESTAMP="$(date +%Y%m%d-%H%M%S)"
LOG_FILE="$LOGS_DIR/${TIMESTAMP}.log"

# Our commit authors (add more if needed).
# NOTE: every author whose commits land on top of the upstream base must be
# listed here (or be matched by OUR_DOMAINS below). If one is missing, the
# upstream-base detection loop stops early on that author's commit and the
# divergence is drastically under-counted.
OUR_AUTHORS=(
    "viktor.vaczi@emergence-engineering.com"
    "balint.ipkovich@emergence-engineering.com"
    "torcsvari.gergo@gmail.com"
    "119620946+matejcsok-ee@users.noreply.github.com"
    "noreply@anthropic.com"
)

# Any commit whose author email ends with one of these domains is treated as
# ours too — a safety net so new emergence contributors don't silently break
# the count. Contributors using gmail/github-noreply addresses must still be
# listed explicitly in OUR_AUTHORS above.
OUR_DOMAINS=(
    "@emergence-engineering.com"
)

echo "=== KiCad Fork Diff Statistics ===" | tee "$LOG_FILE"
echo "Generated: $(date)" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

cd "$KICAD_DIR"

# Find the last commit NOT made by us
echo "Finding last upstream commit..." | tee -a "$LOG_FILE"

# Build grep pattern for our authors
AUTHOR_PATTERN=""
for author in "${OUR_AUTHORS[@]}"; do
    if [ -n "$AUTHOR_PATTERN" ]; then
        AUTHOR_PATTERN="$AUTHOR_PATTERN\|"
    fi
    AUTHOR_PATTERN="$AUTHOR_PATTERN$author"
done

# Find first commit that doesn't match our authors
UPSTREAM_COMMIT=""
while read -r commit_hash author_email; do
    is_ours=false
    for our_author in "${OUR_AUTHORS[@]}"; do
        if [[ "$author_email" == "$our_author" ]]; then
            is_ours=true
            break
        fi
    done
    if [ "$is_ours" = false ]; then
        for our_domain in "${OUR_DOMAINS[@]}"; do
            if [[ "$author_email" == *"$our_domain" ]]; then
                is_ours=true
                break
            fi
        done
    fi
    if [ "$is_ours" = false ]; then
        UPSTREAM_COMMIT="$commit_hash"
        UPSTREAM_AUTHOR="$author_email"
        break
    fi
done < <(git log --format="%H %ae" HEAD)

if [ -z "$UPSTREAM_COMMIT" ]; then
    echo "ERROR: Could not find upstream commit" | tee -a "$LOG_FILE"
    exit 1
fi

# Get commit info
UPSTREAM_SUBJECT=$(git log -1 --format="%s" "$UPSTREAM_COMMIT")
UPSTREAM_DATE=$(git log -1 --format="%ci" "$UPSTREAM_COMMIT")

echo "" | tee -a "$LOG_FILE"
echo "Current HEAD: $(git rev-parse --short HEAD)" | tee -a "$LOG_FILE"
echo "Last upstream commit: $(git rev-parse --short $UPSTREAM_COMMIT)" | tee -a "$LOG_FILE"
echo "  Author: $UPSTREAM_AUTHOR" | tee -a "$LOG_FILE"
echo "  Date: $UPSTREAM_DATE" | tee -a "$LOG_FILE"
echo "  Subject: $UPSTREAM_SUBJECT" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Count our commits on top
OUR_COMMIT_COUNT=$(git rev-list --count "$UPSTREAM_COMMIT"..HEAD)
echo "Our commits on top of upstream: $OUR_COMMIT_COUNT" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Get diff statistics (comparing working tree to upstream)
echo "=== Diff Statistics ===" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Overall stats - compare working tree to upstream (not HEAD)
DIFF_STAT=$(git diff --stat "$UPSTREAM_COMMIT")
DIFF_SHORTSTAT=$(git diff --shortstat "$UPSTREAM_COMMIT")

echo "Summary: $DIFF_SHORTSTAT" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Detailed breakdown
DIFF_NUMSTAT=$(git diff --numstat "$UPSTREAM_COMMIT")

# Count files by type
TOTAL_FILES=0
TOTAL_INSERTIONS=0
TOTAL_DELETIONS=0
MODIFIED_FILES=0
ADDED_FILES=0
DELETED_FILES=0

while IFS=$'\t' read -r added deleted filename; do
    # Skip empty lines
    [ -z "$filename" ] && continue

    TOTAL_FILES=$((TOTAL_FILES + 1))

    # Handle binary files (shown as -)
    if [ "$added" = "-" ]; then
        added=0
    fi
    if [ "$deleted" = "-" ]; then
        deleted=0
    fi

    TOTAL_INSERTIONS=$((TOTAL_INSERTIONS + added))
    TOTAL_DELETIONS=$((TOTAL_DELETIONS + deleted))

    # Categorize file changes
    if [ "$added" -gt 0 ] && [ "$deleted" -eq 0 ]; then
        # Check if file exists in upstream
        if git cat-file -e "$UPSTREAM_COMMIT:$filename" 2>/dev/null; then
            MODIFIED_FILES=$((MODIFIED_FILES + 1))
        else
            ADDED_FILES=$((ADDED_FILES + 1))
        fi
    elif [ "$added" -eq 0 ] && [ "$deleted" -gt 0 ]; then
        # Check if file exists in HEAD
        if git cat-file -e "HEAD:$filename" 2>/dev/null; then
            MODIFIED_FILES=$((MODIFIED_FILES + 1))
        else
            DELETED_FILES=$((DELETED_FILES + 1))
        fi
    else
        MODIFIED_FILES=$((MODIFIED_FILES + 1))
    fi
done <<< "$DIFF_NUMSTAT"

echo "Files changed:    $TOTAL_FILES" | tee -a "$LOG_FILE"
echo "  Added:          $ADDED_FILES" | tee -a "$LOG_FILE"
echo "  Modified:       $MODIFIED_FILES" | tee -a "$LOG_FILE"
echo "  Deleted:        $DELETED_FILES" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"
echo "Lines changed:" | tee -a "$LOG_FILE"
echo "  Insertions:     +$TOTAL_INSERTIONS" | tee -a "$LOG_FILE"
echo "  Deletions:      -$TOTAL_DELETIONS" | tee -a "$LOG_FILE"
echo "  Net change:     $((TOTAL_INSERTIONS - TOTAL_DELETIONS))" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# List changed files
echo "=== Changed Files ===" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Get list with change type (working tree vs upstream)
git diff --name-status "$UPSTREAM_COMMIT" | while IFS=$'\t' read -r status filename; do
    case "$status" in
        A) status_text="[ADDED]    " ;;
        M) status_text="[MODIFIED] " ;;
        D) status_text="[DELETED]  " ;;
        R*) status_text="[RENAMED]  " ;;
        C*) status_text="[COPIED]   " ;;
        *) status_text="[$status]      " ;;
    esac
    echo "$status_text $filename" | tee -a "$LOG_FILE"
done

echo "" | tee -a "$LOG_FILE"
echo "=== End of Report ===" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"
echo "Full log saved to: $LOG_FILE"
