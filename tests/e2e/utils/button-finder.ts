/**
 * Button Finder Utility
 *
 * Scans a wxWidgets WASM canvas app to find clickable buttons by clicking
 * across the canvas and monitoring console logs for responses.
 *
 * Usage:
 *   npx playwright test button-finder --headed
 *
 * Or programmatically:
 *   import { findButtons, scanForButtons } from './utils/button-finder';
 *   const buttons = await findButtons(page, '/standalone/clipboard/clipboard_test.html');
 */

import { Page } from '@playwright/test';

export interface ButtonInfo {
  x: number;
  y: number;
  label: string;
  logTrigger: string;
}

export interface ScanResult {
  buttons: ButtonInfo[];
  canvasBox: { x: number; y: number; width: number; height: number };
  allLogs: string[];
}

/**
 * Gets the canvas bounding box
 */
export async function getCanvasBounds(page: Page): Promise<{ x: number; y: number; width: number; height: number }> {
  const canvas = page.locator('canvas').first();
  const box = await canvas.boundingBox();
  if (!box) {
    throw new Error('Canvas not found');
  }
  return box;
}

/**
 * Scans the canvas for buttons by clicking in a grid pattern
 * and monitoring console logs for responses.
 *
 * @param page - Playwright page
 * @param stepSize - Pixels between click attempts (default 20)
 * @param waitBetweenClicks - Ms to wait between clicks (default 100)
 */
export async function scanForButtons(
  page: Page,
  stepSize: number = 20,
  waitBetweenClicks: number = 100
): Promise<ScanResult> {
  const box = await getCanvasBounds(page);
  const buttons: ButtonInfo[] = [];
  const allLogs: string[] = [];
  const seenLogTriggers = new Set<string>();

  // Collect console logs
  const logHandler = (msg: any) => {
    const text = msg.text();
    allLogs.push(text);
  };
  page.on('console', logHandler);

  console.log(`Scanning canvas: ${box.width}x${box.height} at (${box.x}, ${box.y})`);
  console.log(`Step size: ${stepSize}px, estimated clicks: ${Math.ceil(box.width / stepSize) * Math.ceil(box.height / stepSize)}`);

  // Scan the canvas in a grid pattern
  for (let y = 0; y < box.height; y += stepSize) {
    for (let x = 0; x < box.width; x += stepSize) {
      const clickX = box.x + x;
      const clickY = box.y + y;

      // Record log count before click
      const logCountBefore = allLogs.length;

      // Click at this position
      await page.mouse.click(clickX, clickY);
      await page.waitForTimeout(waitBetweenClicks);

      // Check if new logs appeared
      if (allLogs.length > logCountBefore) {
        const newLogs = allLogs.slice(logCountBefore);

        // Look for interesting patterns (button clicks, events, etc.)
        for (const log of newLogs) {
          // Skip common noise
          if (log.includes('favicon') || log.includes('DevTools')) continue;

          // Look for button-related logs
          const isButtonLog =
            log.includes('button') ||
            log.includes('Button') ||
            log.includes('clicked') ||
            log.includes('Clicked') ||
            log.includes('EVT_BUTTON') ||
            log.includes('Attempting') ||
            log.includes('SUCCESS') ||
            log.includes('ERROR') ||
            log.includes('WARNING') ||
            log.includes('Copy') ||
            log.includes('Paste') ||
            log.includes('Clear') ||
            log.includes('Check') ||
            log.includes('clipboard') ||
            log.includes('Clipboard');

          if (isButtonLog && !seenLogTriggers.has(log)) {
            seenLogTriggers.add(log);

            // Extract a label from the log
            let label = 'Unknown';
            if (log.includes('Copy')) label = 'Copy';
            else if (log.includes('Paste')) label = 'Paste';
            else if (log.includes('Clear')) label = 'Clear';
            else if (log.includes('Check')) label = 'Check';

            buttons.push({
              x: x,  // Relative to canvas
              y: y,
              label,
              logTrigger: log
            });

            console.log(`Found button at (${x}, ${y}): ${log.substring(0, 80)}`);
          }
        }
      }
    }

    // Progress indicator
    if (y % 100 === 0) {
      console.log(`Scanned row ${y}/${box.height}`);
    }
  }

  page.off('console', logHandler);

  return { buttons, canvasBox: box, allLogs };
}

/**
 * Quick scan focusing on common button locations (rows)
 * Faster than full scan - only checks horizontal strips where buttons typically appear
 */
export async function quickScanForButtons(
  page: Page,
  rows: number[] = [150, 180, 200, 220, 250, 280, 300],
  stepSize: number = 15,
  waitBetweenClicks: number = 50
): Promise<ScanResult> {
  const box = await getCanvasBounds(page);
  const buttons: ButtonInfo[] = [];
  const allLogs: string[] = [];
  const seenLogTriggers = new Set<string>();

  const logHandler = (msg: any) => {
    allLogs.push(msg.text());
  };
  page.on('console', logHandler);

  console.log(`Quick scanning rows: ${rows.join(', ')}`);

  for (const y of rows) {
    if (y >= box.height) continue;

    for (let x = 0; x < box.width; x += stepSize) {
      const clickX = box.x + x;
      const clickY = box.y + y;

      const logCountBefore = allLogs.length;

      await page.mouse.click(clickX, clickY);
      await page.waitForTimeout(waitBetweenClicks);

      if (allLogs.length > logCountBefore) {
        const newLogs = allLogs.slice(logCountBefore);

        for (const log of newLogs) {
          if (log.includes('favicon')) continue;

          const isButtonLog =
            log.includes('Attempting') ||
            log.includes('SUCCESS') ||
            log.includes('ERROR') ||
            log.includes('WARNING');

          if (isButtonLog && !seenLogTriggers.has(log)) {
            seenLogTriggers.add(log);

            let label = 'Unknown';
            if (log.includes('copy') || log.includes('Copy')) label = 'Copy';
            else if (log.includes('paste') || log.includes('Paste')) label = 'Paste';
            else if (log.includes('clear') || log.includes('Clear')) label = 'Clear';
            else if (log.includes('check') || log.includes('Check')) label = 'Check';

            buttons.push({ x, y, label, logTrigger: log });
            console.log(`Found button at (${x}, ${y}): ${label} - ${log.substring(0, 60)}`);
          }
        }
      }
    }
  }

  page.off('console', logHandler);

  return { buttons, canvasBox: box, allLogs };
}

/**
 * Find buttons in a specific test app
 */
export async function findButtons(
  page: Page,
  appUrl: string,
  options: { quick?: boolean; stepSize?: number } = {}
): Promise<ScanResult> {
  await page.goto(appUrl);

  // Wait for app to load
  await page.waitForTimeout(2000);

  if (options.quick !== false) {
    return quickScanForButtons(page, undefined, options.stepSize);
  } else {
    return scanForButtons(page, options.stepSize);
  }
}

/**
 * Generate test code snippet for found buttons
 */
export function generateTestCode(result: ScanResult): string {
  const lines: string[] = [
    '// Button positions found by button-finder utility',
    `// Canvas: ${result.canvasBox.width}x${result.canvasBox.height}`,
    '',
  ];

  for (const btn of result.buttons) {
    lines.push(`// ${btn.label} button at (${btn.x}, ${btn.y})`);
    lines.push(`// Trigger: ${btn.logTrigger.substring(0, 60)}`);
    lines.push(`await page.mouse.click(box.x + ${btn.x}, box.y + ${btn.y});`);
    lines.push('');
  }

  return lines.join('\n');
}
