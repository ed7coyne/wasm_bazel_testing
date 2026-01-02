/**
 * This program hermetically downloads Chromium for use with Puppeteer.
 */

const { execSync } = require('child_process');
const path = require('path');
const fs = require('fs');
const { CHROME_EXECUTABLE_PATH } = require('./chrome_executable_path');

const INSTALL_TIMEOUT_MS = 90000; // 90 seconds timeout

async function installChrome(downloadDir) {
  console.log(`Downloading Chrome to: ${downloadDir}`);

  // Create the download directory if it doesn't exist
  if (!fs.existsSync(downloadDir)) {
    fs.mkdirSync(downloadDir, { recursive: true });
  }

  // Install puppeteer with the chrome executable path
  try {
    // Use npx to run the puppeteer CLI to install the browser
    execSync(`npx puppeteer browsers install chrome-headless-shell --platform=linux_arm`, {
      stdio: 'inherit',
      env: {
        ...process.env,
        PUPPETEER_CACHE_DIR: downloadDir
      },
      timeout: INSTALL_TIMEOUT_MS
    });

    const expectedPath = path.join(downloadDir, CHROME_EXECUTABLE_PATH);
    console.log(`Expected Chrome executable path: ${expectedPath}`);

    // Check if the executable exists
    if (!fs.existsSync(expectedPath)) {
      console.error(`ERROR: Chrome executable not found at expected path: ${expectedPath}`);
      process.exit(1);
    }

    // Make the chrome executable executable
    fs.chmodSync(expectedPath, '755');
    console.log(`Chrome executable found at: ${expectedPath}`);
    
    return 0;
  } catch (error) {
    console.error(`Error installing Chrome: ${error.message}`);
    return 1;
  }
}

// The process.argv array looks like ["path/to/node", "program.js", "arg1", "arg2", ...].
if (process.argv.length !== 3) {
  console.error(`Expected exactly one command-line argument, got ${process.argv.length - 2}.`);
  process.exit(1);
}

const downloadDir = path.resolve(process.argv[2]);
installChrome(downloadDir).then(exitCode => process.exit(exitCode));