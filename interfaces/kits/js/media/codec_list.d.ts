import { ErrorCallback, Callback } from './basic';

// 和mux demux公用一个查询类

interface XXX {
    findVideoDecoder(format: Format): string;
    findVideoEncoder(format: Format): string;
    findAudioDecoder(format: Format): string;
    findAudioEncoder(format: Format): string;
    getCodecInfo(): Array<AVCodecInfo>;
}

enum AVCodecType{
    AVCODEC_TYPE_VIDEO_ENCODER = 0,
    AVCODEC_TYPE_VIDEO_DECODER = 1,
    AVCODEC_TYPE_AUDIO_ENCODER = 2,
    AVCODEC_TYPE_AUDIO_DECODER = 3,
}

interface AVCodecInfo {
    name: string;
    type: AVCodecType;
    isHardwareAccelerated: boolean;
    isSoftwareOnly: boolean;
    isVendor: boolean;
    capability: AVCodecCapability;
}

interface AVCodecCapability {
    audioCapability: AudioCapability;
    videoCapability: VideoCapability;
    encoderCapability: EncoderCapability;
    mimeType: string;
    maxSupportedInstances: number;
}

interface AudioCapability {
    supportedBitrate: Array<number>;
    supportedChannel: Array<number>;
    supportedSampleRate: Array<number>;
}

interface VideoCapability {
    heightAlignment: number;
    widthAlignment: number;
    supportedBitrate: Array<number>;
    supportedFrameRate: Array<number>;
    supportedHeight: Array<number>;
    supportedWidth: Array<number>;
    isSizeSupported(width: number, height: number): boolean;
}

interface EncoderCapability {
    supportedComplexity: Array<number>;
    supportedQuality: Array<number>;
    supportedBitrateMode: Array<number>;
}