import * as fs from 'fs';
import * as path from 'path';

/**
 * Recursively clean all files in a directory (keeps directory structure).
 */
function cleanDirectory(dir: string): number {
  let count = 0;
  if (!fs.existsSync(dir)) return count;

  for (const entry of fs.readdirSync(dir)) {
    const fullPath = path.join(dir, entry);
    const stat = fs.statSync(fullPath);
    if (stat.isFile()) {
      fs.unlinkSync(fullPath);
      count++;
    } else if (stat.isDirectory()) {
      count += cleanDirectory(fullPath);
    }
  }
  return count;
}

/**
 * Global setup for Playwright tests.
 * Cleans the logs directory (and subdirectories) before each test run.
 */
export default async function globalSetup() {
  const logsDir = path.join(__dirname, 'logs');
  const count = cleanDirectory(logsDir);
  if (count > 0) {
    console.log(`[global-setup] Removed ${count} log files from ${logsDir}`);
  }
}
