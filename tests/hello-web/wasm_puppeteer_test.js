/**
 * Simplified Puppeteer test for WebAssembly modules
 */
const puppeteer = require('puppeteer');
const http = require('http');
const path = require('path');
const fs = require('fs');

// Get configuration from environment variables
const PORT = process.env.TEST_SERVER_PORT || 8099;
const FIREFOX_PATH = process.env.FIREFOX_PATH;
const WASM_BIN_PATH = process.env.WASM_BIN_PATH;
const TEST_HTML_PATH = process.env.TEST_HTML_PATH;

console.log('Using Firefox path:', FIREFOX_PATH);
console.log('Using WASM binary path:', WASM_BIN_PATH);
console.log('Using test HTML path:', TEST_HTML_PATH);

let server;
let browser;

async function startServer() {
  // Create a simple HTTP server to serve our test page
  const testDir = path.resolve(__dirname);
  
  // Verify that the required files exist
  if (!fs.existsSync(TEST_HTML_PATH)) {
    console.error(`Test HTML file not found at: ${TEST_HTML_PATH}`);
    process.exit(1);
  }
  
  if (!fs.existsSync(WASM_BIN_PATH)) {
    console.error(`WASM binary not found at: ${WASM_BIN_PATH}`);
    process.exit(1);
  }

  server = http.createServer((req, res) => {
    let filePath;
    
    if (req.url === '/') {
      filePath = TEST_HTML_PATH;
    } else if (req.url === '/hello_web.wasm') {
      filePath = WASM_BIN_PATH;
    } else if (req.url === '/tests/hello-web/hello_web_bin') {
      filePath = WASM_BIN_PATH;
    } else {
      filePath = path.join(testDir, req.url);
    }

    const extname = path.extname(filePath);
    let contentType = 'text/html';

    switch (extname) {
      case '.js':
        contentType = 'text/javascript';
        break;
      case '.css':
        contentType = 'text/css';
        break;
      case '.json':
        contentType = 'application/json';
        break;
      case '.wasm':
        contentType = 'application/wasm';
        break;
    }

    fs.readFile(filePath, (err, content) => {
      if (err) {
        if (err.code === 'ENOENT') {
          console.error(`File not found: ${filePath}`);
          res.writeHead(404);
          res.end('File not found');
        } else {
          console.error(`Server error: ${err.code}`);
          res.writeHead(500);
          res.end(`Server Error: ${err.code}`);
        }
      } else {
        res.writeHead(200, { 'Content-Type': contentType });
        res.end(content, 'utf-8');
      }
    });
  });

  return new Promise((resolve) => {
    server.listen(PORT, () => {
      console.log(`Test server running at http://localhost:${PORT}/`);
      resolve();
    });
  });
}

async function stopServer() {
  return new Promise((resolve) => {
    server.close(() => {
      console.log('Test server stopped');
      resolve();
    });
  });
}

async function runTest() {
  try {
    // Start the test server
    await startServer();

    // Launch a headless browser using Firefox
    console.log('Launching Firefox browser...');
    browser = await puppeteer.launch({
      headless: true,
      browser: 'firefox',
      executablePath: FIREFOX_PATH,
    });
    console.log('Firefox browser launched successfully');

    // Open a new page
    const page = await browser.newPage();

    // Navigate to our test page
    await page.goto(`http://localhost:${PORT}/`);

    // Wait for the test to complete (look for the test result element)
    console.log('Waiting for test result element...');
    await page.waitForSelector('#test-result', { visible: true, timeout: 60000 });
    console.log('Test result element found');

    // Get the test result
    const testPassed = await page.evaluate(() => {
      const resultElement = document.getElementById('test-result');
      return resultElement.classList.contains('pass');
    });

    // Get the exit code
    const exitCode = await page.evaluate(() => {
      const outputText = document.getElementById('output').textContent;
      const match = outputText.match(/Exit code: (\d+)/);
      return match ? parseInt(match[1]) : null;
    });

    // Log the results
    if (testPassed && exitCode === 101) {
      console.log('✅ Test passed! Exit code is 101 as expected.');
      return 0; // Success
    } else {
      console.error(`❌ Test failed! Exit code: ${exitCode}`);

      // Get and log the output for debugging
      const output = await page.evaluate(() => document.getElementById('output').textContent);
      console.error(`Output: ${output}`);

      return 1; // Failure
    }
  } catch (error) {
    console.error(`❌ Test error: ${error.message}`);
    return 1; // Failure
  } finally {
    // Clean up
    if (browser) {
      await browser.close();
    }
    await stopServer();
  }
}

// Run the test and exit with the appropriate code
runTest().then(exitCode => {
  process.exit(exitCode);
});