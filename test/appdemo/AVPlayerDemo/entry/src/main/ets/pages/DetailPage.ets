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
import router from '@ohos.router';
import { DetailListComponent } from '../view/DetailListComponent';
import { HLSDetailListComponent } from '../view/HLSDetailListComponent';
import { HTTPDetailListComponent } from '../view/HTTPDetailListComponent';
import { HTTPSDetailListComponent } from '../view/HTTPSDetailListComponent';
import { CommonConstants } from '../common/constants/CommonConstants';

/**
 * Detail page to display more information.
 */
@Entry
@Component
struct DetailPage {
  private titleParam: string = 'default title'

  aboutToAppear() {
    if (router.getParams()) {
      this.titleParam = router.getParams()[CommonConstants.KEY_PARAM_DATA];
      if (this.titleParam == CommonConstants.AVPLAYER_FUNCTION[0]) {

      }
    }
  }

  @Builder NavigationTitle() {
    Column() {
      Text(this.titleParam)
        .width(CommonConstants.NAVIGATION_TITLE_WIDTH_PERCENT)
        .fontColor($r('app.color.navigation_title'))
        .fontSize($r('app.float.detail_navigation_title'))
    }
  }

  build() {
    Column() {
      Navigation()
        .title(this.NavigationTitle())
        .width(CommonConstants.NAVIGATION_WIDTH_PERCENT)
        .height($r('app.float.detail_navigation_height'))
        .backgroundColor($r('app.color.detail_page_background'))
      if (this.titleParam == CommonConstants.AVPLAYER_FUNCTION[0]) {
        DetailListComponent()
      } else if (this.titleParam == CommonConstants.AVPLAYER_FUNCTION[1]) {
        HTTPDetailListComponent()
      } else if (this.titleParam == CommonConstants.AVPLAYER_FUNCTION[2]) {
        HTTPSDetailListComponent()
      } else if (this.titleParam == CommonConstants.AVPLAYER_FUNCTION[3]) {
        HLSDetailListComponent()
      }

    }
    .width(CommonConstants.COLUMN_WIDTH_PERCENT)
    .height(CommonConstants.COLUMN_HEIGHT_PERCENT)
    .backgroundColor($r('app.color.detail_page_background'))
  }
}