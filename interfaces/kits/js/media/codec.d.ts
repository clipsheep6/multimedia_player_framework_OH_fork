import { ErrorCallback, Callback } from './basic';

function createVideoDecoderByName(name: string): VideoDecoder;
function createVideoDecoderByCodec(name: string): VideoDecoder;
function createVideoEncoderByName(name: string): VideoEncoder;
function createVideoEncoderByCodec(name: string): VideoEncoder;
function createAudioDecoderByName(name: string): AudioDecoder;
function createAudioDecoderByCodec(name: string): AudioDecoder;
function createAudioEncoderByName(name: string): AudioEncoder;
function createAudioEncoderByCodec(name: string): AudioEncoder;

enum CodecErrorCode {
    SUCCESS = 0,
}

interface CodecError extends Error {
    code: CodecErrorCode;
}

interface Format {
    [key: string] : any;
}

enum CodecBufferFlag {
    AVCODEC_BUFFER_FLAG_NONE = 0,
    AVCODEC_BUFFER_FLAG_END_OF_STREAM = 1,
}

enum CodecBufferType {
    AVCODEC_BUFFER_TYPE_NONE = 0,
    AVCODEC_BUFFER_TYPE_CODEC = 1,
    AVCODEC_BUFFER_TYPE_KEY_FRAME = 2
}

interface CodecBuffer {
    index: number;
    data: ArrayBuffer;
    offset: number;
    size: number;
}

interface VideoDecoderInput {
    buffer: CodecBuffer;
    timeStampMs: number;
    flag: CodecBufferFlag;
    type: CodecBufferType;
}

interface VideoDecoderOutput {
    buffer: CodecBuffer;
    timeStampMs: number;
    flag: CodecBufferFlag;
}

interface RenderConfig {
    discard: boolean;
}

interface VideoDecoder {
    configure(format: Format): void;
    prepare(): void;
    start(): void;
    stop(): void;
    flush(): void;
    reset(): void;

    queueInputBuffer(buffer: VideoDecoderInput): Promise<void>;
    releaseOutputBuffer(buffer: VideoDecoderOutput, config: RenderConfig): Promise<void>;
    setOutputSurface(surfaceId: string): Promise<void>;
    setParameter(format: Format): Promise<void>;

    on(type: 'configure' | 'prepare' | 'start' | 'stop' | 'flush' | 'reset', callback: () => void): void;
    on(type: 'error', callback: ErrorCallback<CodecError>): void;
    on(type: 'outputFormatChanged', callback: Callback<Format>): void;
    on(type: 'inputBufferAvailable', callback: Callback<VideoDecoderInput>): void;
    on(type: 'outputBufferAvailable', callback: Callback<VideoDecoderOutput>): void;
}

interface VideoEncoderInput {
    buffer: CodecBuffer;
    timeStampMs: number;
    flag: CodecBufferFlag;
}

interface VideoEncoderOutput {
    buffer: CodecBuffer;
    timeStampMs: number;
    flag: CodecBufferFlag;
    type: CodecBufferType;
}

interface VideoEncoder {
    configure(format: Format): void;
    prepare(): void;
    start(): void;
    stop(): void;
    flush(): void;
    reset(): void;

    queueInputBuffer(buffer: VideoEncoderInput): Promise<void>;
    releaseOutputBuffer(buffer: VideoEncoderOutput): Promise<void>;
    setParameter(format: Format): Promise<void>;

    on(type: 'configure' | 'prepare' | 'start' | 'stop' | 'flush' | 'reset', callback: () => void): void;
    on(type: 'error', callback: ErrorCallback<CodecError>): void;
    on(type: 'inputBufferAvailable', callback: Callback<VideoEncoderInput>): void;
    on(type: 'outputBufferAvailable', callback: Callback<VideoEncoderOutput>): void;
}

interface AudioDecoderInput {
    buffer: CodecBuffer;
    timeStampMs: number;
    flag: CodecBufferFlag;
    type: CodecBufferType;
}

interface AudioDecoderOutput {
    buffer: CodecBuffer;
    timeStampMs: number;
    flag: CodecBufferFlag;
}

interface AudioDecoder {
    configure(format: Format): void;
    prepare(): void;
    start(): void;
    stop(): void;
    flush(): void;
    reset(): void;

    queueInputBuffer(buffer: AudioDecoderInput): Promise<void>;
    releaseOutputBuffer(buffer: AudioDecoderOutput): Promise<void>;
    setParameter(format: Format): Promise<void>;

    on(type: 'configure' | 'prepare' | 'start' | 'stop' | 'flush' | 'reset', callback: () => void): void;
    on(type: 'error', callback: ErrorCallback<CodecError>): void;
    on(type: 'inputBufferAvailable', callback: Callback<AudioDecoderInput>): void;
    on(type: 'outputBufferAvailable', callback: Callback<AudioDecoderOutput>): void;
}

interface AudioEncoderInput {
    buffer: CodecBuffer;
    timeStampMs: number;
    flag: CodecBufferFlag;
}

interface AudioEncoderOutput {
    buffer: CodecBuffer;
    timeStampMs: number;
    flag: CodecBufferFlag;
    type: CodecBufferType;
}

interface AudioEncoder {
    configure(format: Format): void;
    prepare(): void;
    start(): void;
    stop(): void;
    flush(): void;
    reset(): void;

    queueInputBuffer(buffer: AudioEncoderInput): Promise<void>;
    releaseOutputBuffer(buffer: AudioEncoderOutput): Promise<void>;
    setParameter(format: Format): Promise<void>;

    on(type: 'configure' | 'prepare' | 'start' | 'stop' | 'flush' | 'reset', callback: () => void): void;
    on(type: 'error', callback: ErrorCallback<CodecError>): void;
    on(type: 'inputBufferAvailable', callback: Callback<AudioEncoderInput>): void;
    on(type: 'outputBufferAvailable', callback: Callback<AudioEncoderOutput>): void;
}