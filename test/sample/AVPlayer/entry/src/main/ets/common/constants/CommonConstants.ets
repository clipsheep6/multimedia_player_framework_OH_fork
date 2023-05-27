/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an 'AS IS' BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * Common constants for all features.
 */
export class CommonConstants {
  static readonly VIDEO_LISTS: Array<string> = ['H264_AAC.mp4', 'H264_MP3.mp4', 'MPEG2_AAC.mp4', 'MPEG2_MP3.mp4']
  static readonly VIDEO_AUDIO_LISTS: Array<string> = ['01.mp3', '02.mp3', '03.mp3', '04.mp3']
  static readonly VIDEO_HTTPS_LISTS: Array<string> = ['http://xxx/01.mp4',
    'http://xxx/02.mp4', 'https://xxx/03.mp4',
    'https://xxx/04.mp4']
  static readonly VIDEO_HLS_LISTS: Array<string> = ['http://xxx/index.m3u8',
    'http://xxx/index.m3u8',
    'http://xxx/index.m3u8',
    'http://xxx/index.m3u8']
  static readonly VIDEO_LIVE_LISTS: Array<string> = ['httpS://xxx/index.m3u8',
    'httpS://xxx/index.m3u8',
    'httpS://xxx/index.m3u8',
    'httpS://xxx/index.m3u8']
  static readonly VIDEO_HLS_NAME: Array<string> = ['测试码流1', '测试码流2', '测试码流3', '测试码流4']
  static readonly AVPLAYER_FUNCTION: Array<string> = ['本地视频播放_url', '本地视频播放_fdSrc', '本地音频播放', 'Https网络播放', 'Hls网络播放', '网络直播', '流式播放(no seek)', '流式播放(seek)']
  static readonly AVPLAYER_REMAK: Array<string> = ['播放本地视频码流_url', '播放本地视频码流_fdSrc', '播放本地音频码流',
    'http/https网络播放，需连接网络', 'hls网络播放，需连接网络', '网络直播，需连接网络', '流式播放', '流式播放']
  /**
   * Search font weight.
   */
  static readonly SEARCH_FONT_WEIGHT: number = 10;

  /**
   * The placeholder of search component.Currently, component interfaces do not support the Resource type.
   * Therefore, you need to define constants. The default prompt in the search box does not support
   * internationalization.
   */
  static readonly SEARCH_PLACEHOLDER: string = 'Search...';

  /**
   * The font family of search component.
   */
  static readonly SEARCH_FONT_FAMILY: string = 'serif';

  /**
   * The percentage of search component.
   */
  static readonly SEARCH_WIDTH_PERCENT: string = '100%';

  /**
   * Number of tab contents.
   */
  static readonly TAB_SIZE: number = 2;

  /**
   * Layout weight of the bottom tab.
   */
  static readonly TAB_LAYOUT_WEIGHT: number = 1;

  /**
   * Number of detail list contents.
   */
  static readonly DETAIL_PAGE_LIST_SIZE: number = 2;

  /**
   * Number of list contents.
   */
  static readonly LIST_SIZE: number = 4;

  /**
   * Layout weight of the list title.
   */
  static readonly LIST_TITLE_LAYOUT_WEIGHT: number = 1;

  /**
   * Max lines of the list title.
   */
  static readonly LIST_TITLE_MAX_LINES: number = 1;

  /**
   * Detail page url.
   */
  static readonly PAGE_NAME: string = 'pages/DetailPage';

  /**
   * Detail page play url.
   */
  static readonly PAGE_PLAY: string = 'pages/AVPlayer';

  /**
   * Detail page audio url.
   */
  static readonly PAGE_AUDIO: string = 'pages/AudioPlayer';

  /**
   * Detail page video url.
   */
  static readonly PAGE_VIDEO_URL: string = 'pages/VideoPlayer_url';

  /**
   * Detail page video fdSrc.
   */
  static readonly PAGE_VIDEO_FDSRC: string = 'pages/VideoPlayer_fdSrc';

  /**
   * Detail page https url.
   */
  static readonly PAGE_HTTPS: string = 'pages/HttpsPlayer';

  /**
   * Detail page https url.
   */
  static readonly PAGE_LIVE: string = 'pages/LivePlayer';

  /**
   * Detail page hls url.
   */
  static readonly PAGE_HLS: string = 'pages/HlsPlayer';

  /**
   * Detail page datasrc url.
   */
  static readonly DATASRC_PLAY: string = 'pages/DataSrc_NoSeek';

  /**
   * Detail page datasrc url.
   */
  static readonly DATASRCSEEK_PLAY: string = 'pages/DataSrc_Seek';

  /**
   * Mode of local video
   */
  static readonly MODE_VIDEO: string = 'video';

  /**
   * Mode of local video
   */
  static readonly MODE_SEEK: string = 'modeSeek';

  /**
   * Mode of local audio
   */
  static readonly MODE_AUDIO: string = 'audio';
  /**
   * Mode of local audio
   */
  static readonly MODE_NETWORK: string = 'network';

  /**
   * Key to transfer parameters when replace pages.
   */
  static readonly KEY_PARAM_DATA: string = 'data';

  /**
   * Key to transfer parameters when replace pages.
   */
  static readonly KEY_PARAM_MODE: string = 'mode';

  /**
   * The percentage of tabs component.
   */
  static readonly TABS_WIDTH_PERCENT: string = '100%';

  /**
   * The percentage of navigation title component.
   */
  static readonly NAVIGATION_TITLE_WIDTH_PERCENT: string = '100%';

  /**
   * The percentage of navigation component.
   */
  static readonly NAVIGATION_WIDTH_PERCENT: string = '100%';

  /**
   * The percentage of list component.
   */
  static readonly LIST_WIDTH_PERCENT: string = '93.3%';

  /**
   * The percentage of column component.
   */
  static readonly COLUMN_WIDTH_PERCENT: string = '100%';

  /**
   * The percentage of height in column component.
   */
  static readonly COLUMN_HEIGHT_PERCENT: string = '100%';

  /**
   * The percentage of width in row component.
   */
  static readonly ROW_WIDTH_PERCENT: string = '100%';

}