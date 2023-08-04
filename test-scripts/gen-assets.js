const zlib = require('node:zlib');
const { createReadStream, createWriteStream } = require('node:fs');
const { readFile, writeFile } = require('node:fs/promises');
const { pipeline } = require('node:stream/promises');
const minify = require('html-minifier').minify;

const fn = async (sourceFiles, destPath) => {
    let allContent = '';

    for (let f =0; f < sourceFiles.length; f++) {
        const interPath1 = 'public/intermediate1.bin';
        const interPath2 = 'public/intermediate2.bin';
        let sourcePath = sourceFiles[f].path;
        const symbolName = sourceFiles[f].symbol;

        // preprocess html
        let content = await readFile(sourcePath, 'utf-8');
        content = minify(content, {
            removeAttributeQuotes: true,
            collapseWhitespace: true,
            minifyCSS: true
        });
        await writeFile(interPath1, content);
        sourcePath = interPath1;

        const source = createReadStream(sourcePath);
        const dest = createWriteStream(interPath2);
        await pipeline(source, zlib.createGzip(), dest);
    
        content = await readFile(interPath2);
        const hexText = content.toString('hex').toUpperCase();
        
        const nibbles = [];
        for(let i=0; i < hexText.length / 2; i++) {
            const breakLine = (i % 8 == 0) ? '\n\t' : '';
            nibbles.push(breakLine + '0x' + hexText.substring(2*i, 2*i+2));
        }
        
        allContent += `int ${symbolName}_length = ${content.length};\n`;
        allContent += `char ${symbolName}[] = \{` + nibbles.join(', ') + '\n};\n';
    }

    writeFile(destPath, allContent);
}

const filesToArchive = [
    { symbol: 'index_html', path: 'public/index.html' }
]
fn(filesToArchive, 'public/output.c');
