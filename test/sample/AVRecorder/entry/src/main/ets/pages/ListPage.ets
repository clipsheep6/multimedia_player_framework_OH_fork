/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import router from '@ohos.router';

@Entry
@Component
struct ListPage {
  @State currentTabIndex: number = 0;

  @Builder NavigationTitle() {
    Column() {
      Text($r('app.string.page_title'))
        .width('100%')
        .fontColor($r('app.color.navigation_title'))
        .fontSize($r('app.float.title_size'))
    }
  }

  build() {
    Column() {
      Navigation()
        .backgroundColor($r('app.color.theme_background'))
        .hideBackButton(true)
        .title(this.NavigationTitle())
        .margin({
          top: $r('app.float.navigation_margin_top'),
          left: $r('app.float.navigation_margin_left'),
          right: $r('app.float.navigation_margin_right'),
          bottom: $r('app.float.navigation_margin_bottom')
        })
        .width('100%')
        .height($r('app.float.navigation_height'))
      Column() {
        List() {
          ListItem() {
            Text($r('app.string.page_title_video'))
              .width('100%')
              .height(100)
              .fontSize(16)
              .textAlign(TextAlign.Center)
              .backgroundColor(0xFFFFFF)
          }
          .onClick(() => {
            router.push({
              url: 'recorder/VRecorder',
              params: {}
            });
          })

          ListItem() {
            Text($r('app.string.page_title_audio'))
              .width('100%')
              .height(100)
              .fontSize(16)
              .textAlign(TextAlign.Center)
              .backgroundColor(0xFFFFFF)
          }
          .onClick(() => {
            router.push({
              url: 'recorder/ARecorder',
              params: {}
            });
          })
        }
        .width('100%')
        .divider({
          strokeWidth: $r('app.float.divider_height'),
          color: $r('app.color.list_divider'),
          startMargin: $r('app.float.list_divider_start_margin'),
          endMargin: $r('app.float.list_divider_end_margin')
        })
        .margin({ bottom: $r('app.float.tab_bar_height') })
      }
      .width('100%')
      .height('100%')
    }
    .backgroundColor($r('app.color.theme_background'))
  }
}