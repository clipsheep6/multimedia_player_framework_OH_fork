/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * Common constants for all features.
 */
export class CommonConstants {
  static readonly VIDEO_LISTS: Array<string> = ['H264_AAC.mp4', 'H264_MP3.mp4', 'MPEG2_AAC.mp4', 'MPEG2_MP3.mp4']
  static readonly VIDEO_HTTP_LISTS: Array<string> = ['HTTP://01', 'HTTP://02', 'HTTP://03', 'HTTP://04']
  static readonly VIDEO_HTTPS_LISTS: Array<string> = ['HTTPS://01', 'HTTPS://02', 'HTTPS://03', 'HTTPS://04']
  static readonly VIDEO_HLS_LISTS: Array<string> = ['HLS://01', 'HLS://02', 'HLS://03', 'HLS://04']
  static readonly AVPLAYER_FUNCTION: Array<string> = ['本地播放', 'HTTP播放', 'HTTPS播放', 'HLS播放']
  static readonly AVPLAYER_REMAK: Array<string> = ['播放本地码流', '播放HTTP协议码流，需连接网络', '播放HTTP协议码流，需连接网络', '播放HTTP协议码流，需连接网络']
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
   * Key to transfer parameters when replace pages.
   */
  static readonly KEY_PARAM_DATA: string = 'data';

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