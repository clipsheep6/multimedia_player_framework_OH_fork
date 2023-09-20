import UIAbility from '@ohos.app.ability.UIAbility';
import hilog from '@ohos.hilog';
import window from '@ohos.window';
import abilityAccessCtrl from '@ohos.abilityAccessCtrl';
const TAG: string = '[EntryAbility]'

export default class EntryAbility extends UIAbility {
  onCreate(want, launchParam) {
    console.info(TAG, '[Demo] EntryAbility onCreate')
    globalThis.abilityWant = want;
    globalThis.shotScreenContext = this.context;
    var context = this.context
    globalThis.pathDir = context.filesDir;
    globalThis.abilityContext=context
    let atManager = abilityAccessCtrl.createAtManager();
    try {
      atManager.requestPermissionsFromUser(this.context, ["ohos.permission.MICROPHONE","ohos.permission.CAPTURE_SCREEN","ohos.permission.READ_MEDIA","ohos.permission.WRITE_MEDIA","ohos.permission.SYSTEM_FLOAT_WINDOW"]).then((data) => {
        console.info("data:" + JSON.stringify(data));
        console.info("data permissions:" + data.permissions);
        console.info("data authResults:" + data.authResults);
      }).catch((err) => {
        console.info("data:" + JSON.stringify(err));
      })
    } catch(err) {
      console.log(`catch err->${JSON.stringify(err)}`);
    }
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onCreate');
  }

  onDestroy() {
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onDestroy');
  }

  onWindowStageCreate(windowStage: window.WindowStage) {
    // Main window is created, set main page for this ability
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onWindowStageCreate');

    windowStage.loadContent('pages/index', (err, data) => {
      if (err.code) {
        hilog.error(0x0000, 'testTag', 'Failed to load the content. Cause: %{public}s', JSON.stringify(err) ?? '');
        return;
      }
      hilog.info(0x0000, 'testTag', 'Succeeded in loading the content. Data: %{public}s', JSON.stringify(data) ?? '');
    });

    windowStage.getMainWindow((err, data) => {
      if (err.code) {
        console.error(TAG, 'Failed to obtain the main window. Cause: ' + JSON.stringify(err));
        return;
      }
      globalThis.mainWindow = data;
      console.info(TAG, 'Succeeded in obtaining the main window. Data: ' + JSON.stringify(data));
    });

  }

  onWindowStageDestroy() {
    // Main window is destroyed, release UI related resources
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onWindowStageDestroy');
  }

  onForeground() {
    // Ability has brought to foreground
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onForeground');
  }

  onBackground() {
    // Ability has back to background
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onBackground');
  }
};
