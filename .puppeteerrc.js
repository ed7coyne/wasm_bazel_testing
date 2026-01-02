/**
 * @type {import("puppeteer").Configuration}
 */
module.exports = {
  // Disable Puppeteer's automatic Chrome download
  // We'll handle this with our chrome_downloader
  skipDownload: true,
};