
export const startScreenCapture: (config:CaptureConfig) => number;
export const stopScreenCapture: () => number;
export const getFrameRate: () => frameRateArray;

interface frameRateArray {
  audioframerate: number;
  videoframerate: number;
  innerframerate: number;
}
interface CaptureConfig {
  micSampleRate: number;
  micChannel: number;
  innerSampleRate: number;
  innerChannel: number;
  width: number;
  height: number;
  isMic: boolean;
}