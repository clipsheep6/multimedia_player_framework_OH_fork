import {ErrorCallback, Callback} from './basic';

declare namespace media {
  function createVideoDecoder(): VideoDecoder;

  enum VideoCodec {
    DEFAULT = 0,
    H264 = 1,
  }

  interface VideoDecoderConfig {
    codec: VideoCodec;
    width: number;
    height: number;
  }

  interface InputFrame {
    readonly data: ArrayBuffer;
    readonly offset: number;
    readonly size: number;
    readonly ptsMs: number;
    readonly endOfStream: boolean;
  }

  interface BufferInfo {
    readonly index: number;
    readonly ptsMs: number;
  }

  interface RenderConfig {
    index: number;
    discard?: boolean;
  }

  interface VideoDecoder {
    configure(config: VideoDecoderConfig): void;
    start(): void;
    decode(frame: InputFrame): void;
    render(config: RenderConfig): void;
    flush(): void;
    stop(): void;
    reset(): void;

    on(type: 'error', callback: ErrorCallback<Error>): void;
    on(type: 'needData', callback: ()=>{}): void;
    on(type: 'outputAvailable', callback: Callback<BufferInfo>): void;
    on(type: 'configure' | 'start' | 'flush' | 'stop' | 'reset', callback: () => void): void;
  }
}

