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
          ListItem() {
//            ListItemComponent({ itemInfo: item })
            Text("视频录制")
              .width('100%').height(100).fontSize(16)
              .textAlign(TextAlign.Center).backgroundColor(0xFFFFFF)
          }
          .onClick(() => {
            router.push({
              url: 'pages/VRecorder',
//              params: {
//                data: item.title
//              }
            });
          })
        ListItem() {
          //            ListItemComponent({ itemInfo: item })
          Text("音频录制")
            .width('100%').height(100).fontSize(16)
            .textAlign(TextAlign.Center).backgroundColor(0xFFFFFF)
        }
        .onClick(() => {
          router.push({
            url: 'pages/ARecorder',
            //              params: {
            //                data: item.title
            //              }
          });
        })
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