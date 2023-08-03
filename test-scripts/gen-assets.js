const zlib = require('node:zlib');
const { createReadStream, createWriteStream } = require('node:fs');
const { readFile, writeFile } = require('node:fs/promises');
const { pipeline } = require('node:stream/promises');

const fn = async (sourceFiles, destPath) => {
    let allContent = '';

    for (let f =0; f < sourceFiles.length; f++) {
        const interPath = 'public/intermediate.bin';
        const sourcePath = sourceFiles[f].path;
        const symbolName = sourceFiles[f].symbol;

        const source = createReadStream(sourcePath);
        const dest = createWriteStream(interPath);
        await pipeline(source, zlib.createGzip(), dest);
    
        const content = await readFile(interPath);
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
