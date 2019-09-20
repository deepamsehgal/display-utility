import bindings from 'bindings';

/**
 * DisplayUtility will retrieve the information about the desktop display.
 */
// tslint:disable-next-line: no-unsafe-any
export const displayUtility: INativeDisplayUtility = bindings('display-utility');

export interface IResolution {
    width: number;
    height: number;
}

interface INativeDisplayUtility {
    getOutputName(rROutput: number): string;
    getCurrentResolution(rROutput: number): IResolution | undefined;
    setResolution(rROutput: number, resolution: IResolution): void;
    getPrimaryRROutput(): number;
}
