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

import { CommonConstants } from '../common/constants/CommonConstants';
import { TabItemData } from '../common/bean/TabItemData';
import router from '@ohos.router';
import { ListAreaComponent } from '../view/ListAreaComponent';
import PageViewModel from '../viewmodel/PageViewModel';

/**
 * ListPage is the entry of application and shows how to develop a list page.The page consists of the search component,
 * list, and tabs. The list component is ListAreaComponent, and each item in the list corresponds to ListItemComponent.
 * The LazyForEach interface is used to implement high performance. Developers can expand
 * ListItemData, TabItemData, and PageViewModel, modify or add your own text, pictures, etc.,
 * and quickly develop functions that meet project requirements.
 */
@Entry
@Component
struct ListPage {
  @State currentTabIndex: number = 0;
  @Builder BuildTabs(item: TabItemData, index: number) {
    Column() {
      Image(this.currentTabIndex === index ? item.imageActivated : item.imageOriginal)
        .width($r('app.float.image_star_width'))
        .height($r('app.float.image_star_height'))
      Text(item.title)
        .margin({ top: $r('app.float.tab_text_margin_top') })
        .fontSize($r('app.float.tab_text_size'))
        .fontColor(this.currentTabIndex === index ? $r('app.color.tab_text_activated') : $r('app.color.tab_text_normal'))
    }
    .justifyContent(FlexAlign.Center)
    .width(CommonConstants.TABS_WIDTH_PERCENT)
    .height($r('app.float.tab_bar_height'))
  }

  @Builder NavigationTitle() {
    Column() {
      Text($r('app.string.page_title'))
        .width(CommonConstants.NAVIGATION_TITLE_WIDTH_PERCENT)
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
        .width(CommonConstants.NAVIGATION_WIDTH_PERCENT)
        .height($r('app.float.navigation_height'))
        ListAreaComponent();
    }
    .backgroundColor($r('app.color.theme_background'))
  }
}