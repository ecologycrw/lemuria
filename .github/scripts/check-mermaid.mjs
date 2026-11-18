// Renders every ```mermaid block of the repository's Markdown files with mermaid-cli (mmdc).
// A syntax error in any diagram fails the CI (docs/CONVENTIONS.md, "Documentação").
import { execFileSync } from 'node:child_process';
import { mkdtempSync, readdirSync, readFileSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join, relative } from 'node:path';

const ignored = new Set(['.git', 'node_modules', 'build', 'out', 'vcpkg_installed']);
const root = process.cwd();

function markdownFiles(directory) {
  return readdirSync(directory, { withFileTypes: true }).flatMap((entry) => {
    if (ignored.has(entry.name)) {
      return [];
    }
    const path = join(directory, entry.name);
    if (entry.isDirectory()) {
      return markdownFiles(path);
    }
    return entry.name.endsWith('.md') ? [path] : [];
  });
}

const work = mkdtempSync(join(tmpdir(), 'mermaid-'));
const puppeteerConfig = join(work, 'puppeteer.json');
writeFileSync(puppeteerConfig, JSON.stringify({ args: ['--no-sandbox'] }));

let diagrams = 0;
let failures = 0;
for (const file of markdownFiles(root)) {
  const blocks = [...readFileSync(file, 'utf8').matchAll(/^```mermaid\r?\n([\s\S]*?)^```/gm)];
  blocks.forEach((block, index) => {
    diagrams++;
    const input = join(work, `diagram-${diagrams}.mmd`);
    writeFileSync(input, block[1]);
    try {
      execFileSync('mmdc', ['--quiet', '-p', puppeteerConfig, '-i', input, '-o', join(work, `diagram-${diagrams}.svg`)], { stdio: 'pipe' });
    } catch (error) {
      failures++;
      console.log(`FAIL ${relative(root, file)} (diagram ${index + 1}):\n${error.stderr?.toString() || error.message}`);
    }
  });
}

console.log(`${diagrams} diagram(s) checked, ${failures} failure(s).`);
process.exit(failures === 0 ? 0 : 1);
