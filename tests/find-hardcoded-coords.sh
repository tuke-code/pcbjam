#!/bin/bash
# Find all lines with hardcoded coordinates in test files
# Excludes waitForTimeout, timeout:, and common non-coordinate numbers

OUTPUT_FILE="hardcoded-coords-report.txt"

echo "=== Hardcoded Coordinates Report ===" > "$OUTPUT_FILE"
echo "Generated: $(date)" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

# Find lines with position/coordinate patterns
echo "=== Click position patterns ===" >> "$OUTPUT_FILE"
grep -n "position: { x:" e2e/*.spec.ts >> "$OUTPUT_FILE" 2>/dev/null

echo "" >> "$OUTPUT_FILE"
echo "=== Mouse click with coordinates ===" >> "$OUTPUT_FILE"
grep -n "mouse\.click.*[0-9]" e2e/*.spec.ts | grep -v "centerX\|centerY" >> "$OUTPUT_FILE" 2>/dev/null

echo "" >> "$OUTPUT_FILE"
echo "=== Mouse move with coordinates ===" >> "$OUTPUT_FILE"
grep -n "mouse\.move.*[0-9]" e2e/*.spec.ts | grep -v "centerX\|centerY" >> "$OUTPUT_FILE" 2>/dev/null

echo "" >> "$OUTPUT_FILE"
echo "=== Mouse dblclick with coordinates ===" >> "$OUTPUT_FILE"
grep -n "mouse\.dblclick.*[0-9]" e2e/*.spec.ts >> "$OUTPUT_FILE" 2>/dev/null

echo "" >> "$OUTPUT_FILE"
echo "=== Canvas click with position ===" >> "$OUTPUT_FILE"
grep -n "canvas\.click.*position" e2e/*.spec.ts >> "$OUTPUT_FILE" 2>/dev/null

echo "" >> "$OUTPUT_FILE"
echo "=== getImageData coordinates ===" >> "$OUTPUT_FILE"
grep -n "getImageData.*[0-9]" e2e/*.spec.ts >> "$OUTPUT_FILE" 2>/dev/null

echo "" >> "$OUTPUT_FILE"
echo "=== clickCanvas calls ===" >> "$OUTPUT_FILE"
grep -n "clickCanvas.*[0-9]" e2e/*.spec.ts >> "$OUTPUT_FILE" 2>/dev/null

echo "" >> "$OUTPUT_FILE"
echo "=== dragCanvas calls ===" >> "$OUTPUT_FILE"
grep -n "dragCanvas.*[0-9]" e2e/*.spec.ts >> "$OUTPUT_FILE" 2>/dev/null

echo "" >> "$OUTPUT_FILE"
echo "=== Summary by file ===" >> "$OUTPUT_FILE"
for file in e2e/*.spec.ts; do
    count=$(grep -c "position: { x:\|mouse\.click.*box\.\|mouse\.move.*box\.\|canvas\.click.*position\|clickCanvas\|dragCanvas" "$file" 2>/dev/null || echo "0")
    if [ "$count" -gt 0 ]; then
        echo "$file: $count potential hardcoded coordinates" >> "$OUTPUT_FILE"
    fi
done

echo ""
echo "Report written to $OUTPUT_FILE"
cat "$OUTPUT_FILE"
