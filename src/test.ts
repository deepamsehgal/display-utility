import fs from 'fs';
import * as path from 'path';
import { displayUtility, lockUtility, screenCaptureUtility } from './index';

// tslint:disable-next-line:no-console
console.log(displayUtility.getConnectedOutputs());
// lockUtility.lockScreen();

// tslint:disable-next-line:no-console
console.log(displayUtility.getPrimaryRROutput());
// displayUtility.setResolution(65, {
//     width: 1440,
//     height: 900
// });

async function testScreenCapturer(): Promise<void> {
    let numberOfFrames: number = 10;
    while (numberOfFrames !== 0) {
        await writeToFile(screenCaptureUtility.getNextFrame());
        numberOfFrames = numberOfFrames - 1;
    }
}

async function writeToFile(input: ArrayBuffer): Promise<void> {
    fs.appendFileSync('/tmp/output.h264', new Buffer(input));
}

// tslint:disable-next-line: no-floating-promises
// testScreenCapturer();
