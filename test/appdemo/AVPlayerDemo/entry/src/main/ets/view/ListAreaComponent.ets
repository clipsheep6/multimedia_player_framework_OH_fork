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
import { CommonConstants } from '../common/constants/CommonConstants';
import { ListItemData } from '../common/bean/ListItemData';
import { ListItemComponent } from './ListItemComponent';
import PageViewModel from '../viewmodel/PageViewModel';

/**
 * List area of the main tab content.
 */
@Component
export struct ListAreaComponent {
  build() {
    Column() {
      List() {
        LazyForEach(PageViewModel.getListDataSource(), (item: ListItemData) => {
          ListItem() {
            ListItemComponent({ itemInfo: item })
          }
          .onClick(() => {
            router.push({
              url: CommonConstants.PAGE_NAME,
              params: {
                data: item.title
              }
            });
          })
        }, item => JSON.stringify(item))
      }
      .width(CommonConstants.LIST_WIDTH_PERCENT)
      .divider({
        strokeWidth: $r('app.float.divider_height'),
        color: $r('app.color.list_divider'),
        startMargin: $r('app.float.list_divider_start_margin'),
        endMargin: $r('app.float.list_divider_end_margin')
      })
      .margin({ bottom: $r('app.float.tab_bar_height') })
    }
    .width(CommonConstants.COLUMN_WIDTH_PERCENT)
    .height(CommonConstants.COLUMN_HEIGHT_PERCENT)
  }
}